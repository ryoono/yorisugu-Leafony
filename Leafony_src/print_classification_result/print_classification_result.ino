// チップ：ATmega328P 3.3V 8MHz 
// パッケージ：Leafony

#include <Wire.h>
#define LCD_ADRS 0x3E  // AQM1602XA I2Cアドレス
#define LCD_COLS 16

// ★ 省電力用：スリープ＆割り込み
#include <avr/sleep.h>
#include <avr/interrupt.h>

// ★ デバッグ用：演算時間計測ピン（D5）
// 必要ならスケッチの先頭などで #define DEBUG を有効化する
// #define DEBUG
#ifdef DEBUG
  #define DEBUG_PIN 5
#endif

// ★ ロジスティック回帰のパラメータ（標準化吸収済み, 生ADC値用）
#include "logistic_params_raw.h"

// ---- ピン定義 ----
static const uint8_t OUTPUT_PINS[] = {6, 7, 8, A3, 9, 10};
static const uint8_t INPUT_PINS[]  = {A0, A1, A2};
static const uint8_t NUM_OUT = sizeof(OUTPUT_PINS) / sizeof(OUTPUT_PINS[0]);
static const uint8_t NUM_IN  = sizeof(INPUT_PINS)  / sizeof(INPUT_PINS[0]);

// 3(入力) x 6(出力) の読み取り結果バッファ
static uint16_t adc[3][6];  // adc[row(in), col(out)]

// OUTPUT_PINS の各要素に対応するポートレジスタとビット
static volatile uint8_t* const OUTPUT_PORTS[NUM_OUT] = {
  &PORTD, &PORTD, &PORTB, &PORTC, &PORTB, &PORTB
};
static const uint8_t OUTPUT_BITS[NUM_OUT] = {
  _BV(PD6), _BV(PD7), _BV(PB0), _BV(PC3), _BV(PB1), _BV(PB2)
};

// 高速版 digitalWrite 相当
static inline void fastWriteHigh(uint8_t idx) {
  *OUTPUT_PORTS[idx] |= OUTPUT_BITS[idx];
}
static inline void fastWriteLow(uint8_t idx) {
  *OUTPUT_PORTS[idx] &= ~OUTPUT_BITS[idx];
}

// 表示状態
enum DispState : uint8_t {
  STATE_OFF = 0,
  STATE_TAKENOKO = 1,
  STATE_KINOKO  = 2,
  STATE_NOT_FOUND = 3
};
DispState g_state = STATE_OFF;
DispState g_state_buf = STATE_OFF;

// ---- 固定コントラスト ----
static const uint8_t FIXED_CONTRAST = 35; // 0..63

// ---- 低レベル送信 ----
static inline void writeCommand(uint8_t cmd) {
  Wire.beginTransmission(LCD_ADRS);
  Wire.write(0x00);  // Co=0, RS=0
  Wire.write(cmd);
  Wire.endTransmission();
  delayMicroseconds(500);
}
static inline void writeData(uint8_t data) {
  Wire.beginTransmission(LCD_ADRS);
  Wire.write(0x40);  // Co=0, RS=1
  Wire.write(data);
  Wire.endTransmission();
  delayMicroseconds(500);
}

// ---- 便利ヘルパー ----
static inline void setCursor(uint8_t col, uint8_t row) {
  writeCommand(0x80 | (row ? (0x40 + col) : col));
}
static inline void writeAsciiStr(const char* s) { while (*s) writeData((uint8_t)*s++); }
static inline void writeSpaces(uint8_t n) { for (uint8_t i = 0; i < n; ++i) writeData(0x20); }
static inline void putCGRAM(uint8_t code) { writeData(code & 0x07); }

// ---- ピン初期化 ----
void initPins() {
  for (uint8_t i = 0; i < NUM_OUT; ++i) {
    pinMode(OUTPUT_PINS[i], OUTPUT);
    fastWriteHigh(i);
  }
  for (uint8_t i = 0; i < NUM_IN; ++i) {
    pinMode(INPUT_PINS[i], INPUT); // 外付けプルダウン
  }

#ifdef DEBUG
  // D5 を演算時間計測用の出力にする
  pinMode(DEBUG_PIN, OUTPUT);
  digitalWrite(DEBUG_PIN, LOW);
#endif
}

// ---- LCD初期化 ----
void initLCD() {
  delay(50);
  writeCommand(0x38);
  writeCommand(0x39);
  writeCommand(0x14);
  writeCommand(0x70 | (FIXED_CONTRAST & 0x0F));
  writeCommand(0x5C | ((FIXED_CONTRAST >> 4) & 0x03));
  writeCommand(0x6C);
  delay(200);
  writeCommand(0x38);
  writeCommand(0x0C);
  writeCommand(0x01);
  delay(2);
  writeCommand(0x06);
}

// ---- CGRAM定義 ----
void defineCGRAMAll() {
  const uint8_t CGRAMs[8][8] = {
    { 0x03, 0x0F, 0x0C, 0x18, 0x18, 0x18, 0x1C, 0x00 },
    { 0x18, 0x0C, 0x02, 0x01, 0x01, 0x02, 0x0C, 0x00 },
    { 0x1F, 0x1F, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x18, 0x04, 0x12, 0x11, 0x09, 0x09, 0x06, 0x00 },
    { 0x01, 0x03, 0x06, 0x05, 0x0B, 0x0E, 0x1D, 0x00 },
    { 0x10, 0x18, 0x0C, 0x14, 0x0A, 0x1E, 0x1B, 0x00 },
    { 0x1D, 0x1D, 0x1B, 0x1F, 0x04, 0x04, 0x07, 0x00 },
    { 0x17, 0x0F, 0x0F, 0x1F, 0x04, 0x04, 0x1C, 0x00 }
  };
  for (uint8_t ch = 0; ch < 8; ++ch) {
    writeCommand(0x40 | ((ch & 0x07) << 3));
    for (uint8_t r = 0; r < 8; ++r) writeData(CGRAMs[ch][r] & 0x1F);
  }
}

// ---- 画面クリア ----
void lcdClearAll() {
  setCursor(0, 0); for (int i = 0; i < LCD_COLS; ++i) writeData(0x20);
  setCursor(0, 1); for (int i = 0; i < LCD_COLS; ++i) writeData(0x20);
  writeCommand(0x01);
  delayMicroseconds(500);
}

// ---- パターン ----
void drawKinokoPattern() {
  lcdClearAll();
  setCursor(0, 0);
  putCGRAM(0); putCGRAM(1);
  writeSpaces(3);
  writeAsciiStr("Kinoko");
  writeSpaces(3);
  putCGRAM(0); putCGRAM(1);
  setCursor(0, 1);
  putCGRAM(2); putCGRAM(3);
  writeSpaces(12);
  putCGRAM(2); putCGRAM(3);
}
void drawTakenokoPattern() {
  lcdClearAll();
  setCursor(0, 0);
  putCGRAM(4); putCGRAM(5);
  writeSpaces(2);
  writeAsciiStr("Takenoko");
  writeSpaces(2);
  putCGRAM(4); putCGRAM(5);
  setCursor(0, 1);
  putCGRAM(6); putCGRAM(7);
  writeSpaces(12);
  putCGRAM(6); putCGRAM(7);
}
void drawNotFoundPattern() {
  lcdClearAll();
  setCursor(0, 0);

  const char* msg = "Not Found";
  const uint8_t len = 9;                 // "Not Found" の文字数
  uint8_t left = (LCD_COLS - len) >> 1;  // ÷2（中央寄せ用）

  // 1行目中央に "Not Found" を表示
  writeSpaces(left);
  writeAsciiStr(msg);
  // 残りは lcdClearAll() 済みなので特に何もしなくてOK
}

// ---- 状態適用 ----
void printResult() {
  switch (g_state) {
    case STATE_OFF:       lcdClearAll();         break;
    case STATE_TAKENOKO:  drawTakenokoPattern(); break;
    case STATE_KINOKO:    drawKinokoPattern();   break;
    case STATE_NOT_FOUND: drawNotFoundPattern(); break;
  }
}

// 何もない場合の判定 学習データ最大(2977)より少し大きい値にしておく
const uint16_t NO_OBJECT_THRESHOLD = 3200;

// ★ 1秒ごとに計測処理を実行するためのフラグ（Timer1割り込みでセット）
volatile bool g_do_measure = false;

// ★ 光センサ値(adc[3][6])からラベル(0〜3)を推論
int predictLabelFromAdc() {
  float x[LOGI_NUM_FEATURES];
  uint8_t idx = 0;

  // 学習時と同じ並び順（行優先: r→c）で18要素にフラット化
  for (uint8_t r = 0; r < NUM_IN; ++r) {
    for (uint8_t c = 0; c < NUM_OUT; ++c) {
      // NUM_IN * NUM_OUT == LOGI_NUM_FEATURES(18) 固定なので境界チェックは不要
      x[idx++] = (float)adc[r][c];   // 生のanalogRead値をそのまま使う
    }
  }

  float best_score = -1.0e30f;
  int   best_k     = 0;

  for (uint8_t k = 0; k < LOGI_NUM_CLASSES; ++k) {
    float z = LOGI_B[k];
    for (uint8_t i = 0; i < LOGI_NUM_FEATURES; ++i) {
      z += LOGI_W[k][i] * x[i];
    }
    if (k == 0 || z > best_score) {
      best_score = z;
      best_k     = (int)k;
    }
  }
  return best_k;
}

// ★ Timer1 を 1Hz（1秒ごと）CTC割り込みに設定
void initTimer1_1Hz() {
  cli();              // 割り込み一時禁止

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  // CTCモード (WGM12 = 1)
  TCCR1B |= (1 << WGM12);

  // 8MHz / 1024 = 7812.5 Hz → 約1秒ごとに割り込み
  OCR1A = 7812;  // 1秒弱だが展示用途なら十分

  // プリスケーラ 1024 設定 (CS12=1, CS10=1)
  TCCR1B |= (1 << CS12) | (1 << CS10);

  // 比較一致A割り込み許可
  TIMSK1 |= (1 << OCIE1A);

  sei();              // 割り込み許可
}

// ★ Timer1 比較一致A割り込みハンドラ：フラグを立てるだけ
ISR(TIMER1_COMPA_vect) {
  g_do_measure = true;
}

// 物体認識と結果表示関数
void runSensorScanAndInference(){


  uint16_t adc_sum = 0; // 何もない場合を判定するための、ADC合計値

  // FETを順にONし、光センサを3つずつ読み込む(6列分)
  for (uint8_t j = 0; j < NUM_OUT; ++j) {

    // 対象の列だけ LOW(FET ON)
    fastWriteLow(j);   // ★ digitalWrite → レジスタ直叩き

    // （必要ならRC安定待ち）
    delayMicroseconds(2500);

    // このタイミングで INPUT を1回ずつ読む（列 j に格納）
    for (uint8_t r = 0; r < NUM_IN; ++r) {
      (void)analogRead(INPUT_PINS[r]);           // 捨て読み（ADC内部の前回値を追従させる）
      adc[r][j] = analogRead(INPUT_PINS[r]);
      adc_sum += adc[r][j];
    }

    // 対象の列だけ HIGH(FET OFF)
    fastWriteHigh(j);  // ★ digitalWrite → レジスタ直叩き
  }

  // 最後の列（index=5）を読んだ直後に、推論 & LCD出力処理
  // 物体が何もない場合は、推論をしない
  // LCD表示が前回と変更ない場合は、出力しない
  if (adc_sum >= NO_OBJECT_THRESHOLD) { // 物体が何もない場合
    g_state = STATE_NOT_FOUND;
  }
  else { // 物体がある場合
#ifdef DEBUG
  // ★ 計測開始：D5(HIGH)
  PORTD |= _BV(PD5);
#endif
    int label = predictLabelFromAdc();
#ifdef DEBUG
  // ★ 計測終了：D5(LOW)
  PORTD &= ~_BV(PD5);
#endif
    if ((label == 0) || (label == 1))  g_state = STATE_KINOKO;
    else                               g_state = STATE_TAKENOKO;
  }

  if (g_state != g_state_buf)  printResult();
  g_state_buf = g_state;
}

void setup() {
  // Serial.begin(115200);
  Wire.begin();
  initPins();
  initLCD();
  defineCGRAMAll();

  g_state = STATE_OFF;
  printResult();

  // ★ 1秒タイマ割り込みの初期化
  initTimer1_1Hz();
}

void loop() {
  // ===== スリープ =====
  if (!g_do_measure) {
    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_enable();
    sleep_cpu();    // ← Timer1で起きる
    sleep_disable();
  }

  // ===== ここへ来たら、Timer1が1秒ごとに起こした証拠 =====
  if (!g_do_measure) {
    return;         // 念のため
  }
  g_do_measure = false;

  // ===== 1秒ごとの測定処理（元の処理をそのまま置く） =====
  runSensorScanAndInference();   // ← あなたのADC読み込み＋推論＋LCD更新処理

  // ===== 処理が終わったので、次の割り込みまでスリープへ =====
}
