#include <Wire.h>
#define LCD_ADRS 0x3E  // AQM1602XA I2Cアドレス
#define LCD_COLS 16

// 表示状態
enum DispState : uint8_t {
  STATE_OFF = 0,
  STATE_TAKENOKO = 1,   // 旧 HALF_ON
  STATE_KINOKO  = 2,    // 旧 FULL_ON
  STATE_CHAR_SAMPLE = 3
};
DispState g_state = STATE_OFF;

int g_contrast = 0;    // 0..63, 'c'で+7

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
  writeCommand(0x80 | (row ? (0x40 + col) : col)); // row: 0=上,1=下
}
static inline void writeAsciiStr(const char* s) {
  while (*s) writeData((uint8_t)*s++);
}
static inline void writeSpaces(uint8_t n) {
  for (uint8_t i = 0; i < n; ++i) writeData(0x20);
}
static inline void putCGRAM(uint8_t code) { // 0..7
  writeData(code & 0x07);
}

// ---- 初期化 ----
void initLCD() {
  delay(50);
  writeCommand(0x38);     // Function set
  writeCommand(0x39);     // IS=1
  writeCommand(0x14);     // Bias/OSC
  writeCommand(0x70 | (g_contrast & 0x0F));          // Contrast low
  writeCommand(0x5C | ((g_contrast >> 4) & 0x03));   // Contrast high, Icon off, Booster on
  writeCommand(0x6C);     // Follower ON
  delay(200);
  writeCommand(0x38);     // IS=0
  writeCommand(0x0C);     // Display ON
  writeCommand(0x01);     // Clear
  delay(2);
  writeCommand(0x06);     // Entry mode
}

// ---- CGRAM定義（0〜7） ----
void defineCGRAMAll() {
  const uint8_t CGRAMs[8][8] = {
    { 0x03, 0x0F, 0x0C, 0x18, 0x18, 0x18, 0x1C, 0x00 }, // 0
    { 0x18, 0x0C, 0x02, 0x01, 0x01, 0x02, 0x0C, 0x00 }, // 1
    { 0x1F, 0x1F, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 2
    { 0x18, 0x04, 0x12, 0x11, 0x09, 0x09, 0x06, 0x00 }, // 3
    { 0x01, 0x03, 0x06, 0x05, 0x0B, 0x0E, 0x1D, 0x00 }, // 4
    { 0x10, 0x18, 0x0C, 0x14, 0x0A, 0x1E, 0x1B, 0x00 }, // 5
    { 0x1D, 0x1D, 0x1B, 0x1F, 0x04, 0x04, 0x07, 0x00 }, // 6
    { 0x17, 0x0F, 0x0F, 0x1F, 0x04, 0x04, 0x1C, 0x00 }  // 7
  };
  for (uint8_t ch = 0; ch < 8; ++ch) {
    writeCommand(0x40 | ((ch & 0x07) << 3));   // CGRAM base addr
    for (uint8_t r = 0; r < 8; ++r) writeData(CGRAMs[ch][r] & 0x1F);
  }
}

// ---- 画面クリア ----
void lcdClearAll() {
  // 空白で埋めてから Clear でチラつき抑制
  setCursor(0, 0); for (int i=0; i<LCD_COLS; ++i) writeData(0x20);
  setCursor(0, 1); for (int i=0; i<LCD_COLS; ++i) writeData(0x20);
  writeCommand(0x01);
  delay(2);
}

// ---- PATTERN: KINOKO（旧 FULL_ON） ----
// 上段：CGRAM0,1 , 空白×3 , "Kinoko"(6) , 空白×3 , CGRAM0,1
// 下段：CGRAM2,3 , 空白×12 , CGRAM2,3
void drawKinokoPattern() {
  lcdClearAll();

  // 上段
  setCursor(0, 0);
  putCGRAM(0); putCGRAM(1);         // 左端 CGRAM0,1
  writeSpaces(3);
  writeAsciiStr("Kinoko");          // 6
  writeSpaces(3);
  putCGRAM(0); putCGRAM(1);         // 右端 CGRAM0,1

  // 下段
  setCursor(0, 1);
  putCGRAM(2); putCGRAM(3);         // 左端 CGRAM2,3
  writeSpaces(12);
  putCGRAM(2); putCGRAM(3);         // 右端 CGRAM2,3
}

// ---- PATTERN: TAKENOKO（旧 HALF_ON） ----
// 上段：CGRAM4,5 , 空白×2 , "Takenoko"(8) , 空白×2 , CGRAM4,5
// 下段：CGRAM6,7 , 空白×12 , CGRAM6,7
void drawTakenokoPattern() {
  lcdClearAll();

  // 上段
  setCursor(0, 0);
  putCGRAM(4); putCGRAM(5);         // 左端 CGRAM4,5
  writeSpaces(2);
  writeAsciiStr("Takenoko");        // 8
  writeSpaces(2);
  putCGRAM(4); putCGRAM(5);         // 右端 CGRAM4,5

  // 下段
  setCursor(0, 1);
  putCGRAM(6); putCGRAM(7);         // 左端 CGRAM6,7
  writeSpaces(12);
  putCGRAM(6); putCGRAM(7);         // 右端 CGRAM6,7
}

// ---- サンプル（変更なし） ----
void drawCharSample() {
  lcdClearAll();
  const char line1[17] = "ABCDEFGHIJKLMNOP";
  const char line2[17] = "QRSTUVWXYZabcdef";
  setCursor(0, 0); for (int i=0;i<LCD_COLS;i++) writeData(line1[i]);
  setCursor(0, 1); for (int i=0;i<LCD_COLS;i++) writeData(line2[i]);
}

// ---- コントラスト ----
void setContrast(int val) {
  val = constrain(val, 0, 63);
  writeCommand(0x39); // IS=1
  writeCommand(0x70 | (val & 0x0F));
  writeCommand(0x5C | ((val >> 4) & 0x03));
  writeCommand(0x38); // IS=0
}

// ---- 状態適用 ----
void applyState() {
  switch (g_state) {
    case STATE_OFF:
      lcdClearAll();        Serial.println(F("STATE_OFF")); break;
    case STATE_TAKENOKO:
      drawTakenokoPattern();Serial.println(F("STATE_TAKENOKO: [4,5]..Takenoko..[4,5] / [6,7]....[6,7]")); break;
    case STATE_KINOKO:
      drawKinokoPattern();  Serial.println(F("STATE_KINOKO: [0,1]..Kinoko..[0,1] / [2,3]....[2,3]")); break;
    case STATE_CHAR_SAMPLE:
      drawCharSample();     Serial.println(F("STATE_CHAR_SAMPLE")); break;
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();             // Due: SDA=20, SCL=21
  initLCD();
  defineCGRAMAll();
  applyState();    // 初期 STATE_OFF
  Serial.println(F("Ready. 'r': OFF -> TAKENOKO -> KINOKO -> SAMPLE,  'c': Contrast +7"));
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'r' || c == 'R') {
      g_state = static_cast<DispState>((g_state + 1) % 4);
      applyState();
    }
    else if (c == 'c' || c == 'C') {
      g_contrast += 7;
      if (g_contrast > 63) g_contrast = 0;
      setContrast(g_contrast);
      Serial.print(F("Contrast = "));
      Serial.println(g_contrast);
    }
  }
}
