#include <Wire.h>
#define LCD_ADRS 0x3E  // AQM1602XA I2Cアドレス

// 表示状態：消灯 → 半分点灯(下) → 全点灯 → 文字サンプル
enum DispState : uint8_t { ALL_OFF = 0, HALF_ON = 1, FULL_ON = 2, CHAR_SAMPLE = 3 };
DispState state = ALL_OFF;

int  contrastVal = 0;    // 0..63, 'c'で+7

// ---- 基本I2C送信 ----
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

// ---- 初期化 ----
void init_LCD() {
  delay(50);
  writeCommand(0x38);     // Function set
  writeCommand(0x39);     // IS=1
  writeCommand(0x14);     // Bias/OSC (初期は1/5 bias)
  writeCommand(0x70 | (contrastVal & 0x0F));          // Contrast low
  writeCommand(0x5C | ((contrastVal >> 4) & 0x03));   // Contrast high, Icon off, Booster on
  writeCommand(0x6C);     // Follower ON
  delay(200);
  writeCommand(0x38);     // IS=0
  writeCommand(0x0C);     // Display ON
  writeCommand(0x01);     // Clear
  delay(2);
  writeCommand(0x06);     // Entry mode
}

// ---- CGRAM定義 ----
// コード0: 5x8 全点灯
void defineFullOnChar() {
  writeCommand(0x40);     // CGRAM addr = 0（キャラ0）
  for (int i = 0; i < 8; i++) writeData(0x1F);
}

// コード1: 下半分のみ点灯（行4〜7を0x1F、行0〜3は0x00）
void defineHalfOnCharBottom() {
  writeCommand(0x40 | 0x08); // CGRAM addr = 8（キャラ1の先頭）
  for (int i = 0; i < 4; i++) writeData(0x00); // 上半分 消灯
  for (int i = 0; i < 4; i++) writeData(0x1F); // 下半分 点灯
}

// ---- DDRAM全域を指定コードで埋める（16x2） ----
void lcdFillAll(uint8_t code) {
  writeCommand(0x80 | 0x00);
  for (int i = 0; i < 16; i++) writeData(code);
  writeCommand(0x80 | 0x40);
  for (int i = 0; i < 16; i++) writeData(code);
}

// ---- 全消灯（空白で埋め→Clear） ----
void lcdAllOff() {
  lcdFillAll(0x20);   // 空白で埋める（0b0010_0000）
  writeCommand(0x01); // Clear Display（DDRAM全消去＋ホーム）
  delay(2);
}

// ---- 半分点灯（コード1） ----
void lcdHalfOn() {
  lcdAllOff();
  lcdFillAll(0x01); // CGRAM[1]
}

// ---- 全点灯（コード0） ----
void lcdAllOn() {
  lcdAllOff();
  lcdFillAll(0x00); // CGRAM[0]
}

// ---- キャラクタサンプル表示（"ABCDEFGHIJKLMNOP" / "QRSTUVWXYZabcdef"） ----
void lcdCharSample() {
  lcdAllOff();
  const char line1[17] = "ABCDEFGHIJKLMNOP";     // 16文字
  const char line2[17] = "QRSTUVWXYZabcdef";     // 16文字（A..Zの続きに a..f）
  writeCommand(0x80 | 0x00);                     // 1行目先頭
  for (int i = 0; i < 16; i++) writeData(line1[i]);
  writeCommand(0x80 | 0x40);                     // 2行目先頭
  for (int i = 0; i < 16; i++) writeData(line2[i]);
}

// ---- コントラスト設定 ----
void setContrast(int val) {
  val = constrain(val, 0, 63);
  writeCommand(0x39); // IS=1
  writeCommand(0x70 | (val & 0x0F));
  writeCommand(0x5C | ((val >> 4) & 0x03));
  writeCommand(0x38); // IS=0
}

// ---- 状態適用：再描画 ----
void applyState() {
  switch (state) {
    case ALL_OFF:    lcdAllOff();     Serial.println(F("ALL-OFF")); break;
    case HALF_ON:    lcdHalfOn();     Serial.println(F("HALF-ON (bottom half)")); break;
    case FULL_ON:    lcdAllOn();      Serial.println(F("FULL-ON")); break;
    case CHAR_SAMPLE:lcdCharSample(); Serial.println(F("CHAR-SAMPLE (A..Z then a..f)")); break;
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();             // Due: SDA=20, SCL=21
  init_LCD();
  defineFullOnChar();
  defineHalfOnCharBottom();
  applyState();    // 初期 ALL_OFF
  Serial.println(F("Ready. 'r': OFF -> HALF -> FULL -> SAMPLE,  'c': Contrast +7"));
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'r' || c == 'R') {
      state = static_cast<DispState>((state + 1) % 4); // OFF→HALF→FULL→SAMPLE→OFF…
      applyState();
    }
    else if (c == 'c' || c == 'C') {
      contrastVal += 7;
      if (contrastVal > 63) contrastVal = 0;
      setContrast(contrastVal);
      Serial.print(F("Contrast = "));
      Serial.println(contrastVal);
    }
  }
}
