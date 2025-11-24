// チップ：ATmega328P 3.3V 8MHz 
// パッケージ：Leafony

#include <Wire.h>
#define LCD_ADRS 0x3E  // AQM1602XA I2Cアドレス
#define LCD_COLS 16

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
  STATE_CHAR_SAMPLE = 3
};
DispState g_state = STATE_OFF;

// ---- 固定コントラスト ----
static const uint8_t FIXED_CONTRAST = 35; // 0..63

// ---- 低レベル送信 ----
static inline void writeCommand(uint8_t cmd) {
  Wire.beginTransmission(LCD_ADRS);
  Wire.write(0x00);  // Co=0, RS=0
  Wire.write(cmd);
  Wire.endTransmission();
  delay(2);
}
static inline void writeData(uint8_t data) {
  Wire.beginTransmission(LCD_ADRS);
  Wire.write(0x40);  // Co=0, RS=1
  Wire.write(data);
  Wire.endTransmission();
  delay(1);
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
    digitalWrite(OUTPUT_PINS[i], HIGH);
  }
  for (uint8_t i = 0; i < NUM_IN; ++i) {
    pinMode(INPUT_PINS[i], INPUT); // 外付けプルダウン
  }
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
  delay(2);
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

// ---- 状態適用 ----
void applyState() {
  switch (g_state) {
    case STATE_OFF:       lcdClearAll();        break;
    case STATE_TAKENOKO:  drawTakenokoPattern();break;
    case STATE_KINOKO:    drawKinokoPattern();  break;
    case STATE_CHAR_SAMPLE:                     break;
  }
}

// ---- メイン処理（LCDシーケンス）----
const DispState SEQ[4] = { STATE_OFF, STATE_TAKENOKO, STATE_OFF, STATE_KINOKO };
uint8_t seq_idx = 0;
unsigned long last_sw = 0;
const unsigned long INTERVAL_MS = 5000UL;

// ---- 出力の順次LOW & 入力サンプリング ----
unsigned long last_out_time = 0;
const unsigned long OUT_INTERVAL = 500; // 0.5秒
uint8_t out_index = 0;

void printAdcMatrix() {
  // 3x6 = 18要素を1次元CSVとして出力
  bool first = true;
  for (uint8_t r = 0; r < NUM_IN; ++r) {
    for (uint8_t c = 0; c < NUM_OUT; ++c) {
      if (!first) {
        Serial.print(',');          // 2個目以降はカンマを付ける
      }
      Serial.print(adc[r][c]);
      first = false;
    }
  }
  Serial.println();                 // 行終端（1サンプル=1行）
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  initPins();
  initLCD();
  defineCGRAMAll();

  g_state = SEQ[seq_idx];
  applyState();

  last_sw = millis();
  last_out_time = last_sw;
}

void loop() {
  // ==== LCD更新処理（既存） ====
  unsigned long now = millis();
  if (now - last_sw >= INTERVAL_MS) {
    last_sw = now;
    seq_idx = (seq_idx + 1) & 0x03;
    g_state = SEQ[seq_idx];
    applyState();
  }

  // ==== 0.5秒おきにscan ====
  if (now - last_out_time >= OUT_INTERVAL) {
    last_out_time = now;

    for( uint8_t j=0; j<NUM_OUT;++j){
  
      // 今の列だけ LOW
      fastWriteLow(out_index);   // ★ digitalWrite → レジスタ直叩き
  
      // （必要ならRC安定待ち）
      delayMicroseconds(2750);
  
      // このタイミングで INPUT を1回ずつ読む（列 out_index に格納）
      for (uint8_t r = 0; r < NUM_IN; ++r) {
        (void)analogRead(INPUT_PINS[r]);           // 捨て読み（ADC内部の前回値を追従させる）
        adc[r][out_index] = analogRead(INPUT_PINS[r]);
      }

      fastWriteHigh(out_index);  // ★ digitalWrite → レジスタ直叩き
  
      // 最後の列（index=5）を読んだ直後に、3x6 行列をシリアル送信
      if (out_index == (NUM_OUT - 1)) {
        printAdcMatrix();
      }
  
      // 次の列へ
      out_index++;
      if (out_index >= NUM_OUT) out_index = 0;
    }
  }
}
