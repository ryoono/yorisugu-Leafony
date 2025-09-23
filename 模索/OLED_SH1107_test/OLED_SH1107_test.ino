#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

// --- UNO R4 専用（フルバッファ） ---
U8G2_SH1107_128X128_F_HW_I2C u8g2(U8G2_R0, /*reset=*/ U8X8_PIN_NONE);

// 必要なら横オフセットを調整
uint8_t xoff = 32;
inline uint8_t X(int x){ return (uint8_t)((x + xoff) & 127); }
inline void drawPixelWrap(int x,int y){ u8g2.drawPixel(X(x), y); }
inline void hline_wrap(int x,int y,int w){ for(int i=0;i<w;++i) drawPixelWrap(x+i,y); }

// コントラスト候補（生値 0..255）
const uint8_t CONTR_LIST[5] = { 0, 5, 10, 15, 20 };        // ★指定の通り
int idx_contrast = 0; // 初期: 0

// 点灯率候補（%）
const uint8_t ONPCT_LIST[5] = { 0, 5, 10, 15, 20 };        // ★指定の通り
int idx_pct = 0; // 初期: 0%

// 送信/描画ペース
uint32_t i2c_khz = 50;   // 既定 50kHz（安定優先）
uint8_t  chunk_h = 8;    // 一度に描く行数（ゆっくり描画）
uint8_t  chunk_wait_ms = 3;
uint32_t dwell_ms = 3000;

void applyI2C(){ Wire.setClock(constrain(i2c_khz,20u,100u)*1000u); }
void applyContrastRaw(uint8_t v){ u8g2.setContrast(v); delay(2); }

// 20行周期の縞：周期中の先頭 N 行のみ点灯（N = round(20*pct/100)）
void drawStripeChunkToBuffer(uint8_t pct, int y0, int y1){
  const int W = u8g2.getDisplayWidth();   // 128
  const int H = u8g2.getDisplayHeight();  // 128
  y0 = max(0, y0); y1 = min(H, y1);
  int on_rows = (int)round(20.0 * pct / 100.0); // 0..4（pct<=20%想定）
  if (on_rows > 20) on_rows = 20;

  for(int y = y0; y < y1; ++y){
    if ((y % 20) < on_rows){
      hline_wrap(0, y, W);
    }
  }
}

// フルバッファに累積→部分送信でゆっくり描画（前回分は残る）
void renderStripeSlow(uint8_t pct){
  const int H = u8g2.getDisplayHeight();
  const int ch = constrain((int)chunk_h, 1, 32);

  u8g2.clearBuffer(); // 一度だけ全消去
  for(int y0=0; y0<H; y0+=ch){
    int y1 = min(y0+ch, H);
    drawStripeChunkToBuffer(pct, y0, y1); // 該当チャンクを描き足し
    u8g2.sendBuffer();                    // 逐次送信（負荷分散）
    delay(chunk_wait_ms);
  }
}

void applyAndRender(){
  applyContrastRaw(CONTR_LIST[idx_contrast]);
  renderStripeSlow(ONPCT_LIST[idx_pct]);
}

// 自動巡回（5×5=25通り）
void runGrid(){
  Serial.println(F("Start grid sweep (contrast x pct)"));
  for(int ci=0; ci<5; ++ci){
    for(int pi=0; pi<5; ++pi){
      idx_contrast = ci;
      idx_pct      = pi;
      Serial.print(F("  contrast=")); Serial.print(CONTR_LIST[ci]);
      Serial.print(F("  pct="));      Serial.print(ONPCT_LIST[pi]); Serial.print(F("%"));
      Serial.print(F("  i2c="));      Serial.print(i2c_khz);        Serial.print(F("kHz"));
      Serial.print(F("  chunk="));    Serial.print(chunk_h);
      Serial.print(F("  wait="));     Serial.print((int)chunk_wait_ms); Serial.println(F("ms"));

      applyAndRender();
      uint32_t t0 = millis();
      while (millis() - t0 < dwell_ms) delay(10);
    }
  }
  Serial.println(F("Grid sweep done."));
}

void printHelp(){
  Serial.println(F("Commands:"));
  Serial.println(F("  c/C : next/prev contrast (0,5,10,15,20) [raw 0..255]"));
  Serial.println(F("  p/P : next/prev percent  (0,5,10,15,20 %)"));
  Serial.println(F("  g   : run all combinations (grid sweep)"));
  Serial.println(F("  tNNN: set dwell ms (>=100), e.g., t2000"));
  Serial.println(F("  iNN : set I2C clock kHz (20..100), e.g., i50"));
  Serial.println(F("  bNN : set chunk height lines (1..32), e.g., b8"));
  Serial.println(F("  wNN : set chunk wait ms (0..20), e.g., w3"));
  Serial.println(F("  s   : show current settings"));
  Serial.println(F("  h   : help"));
}
void printState(){
  Serial.print(F("contrast=")); Serial.print(CONTR_LIST[idx_contrast]); Serial.print(F("  "));
  Serial.print(F("pct="));      Serial.print(ONPCT_LIST[idx_pct]);      Serial.print(F("%  "));
  Serial.print(F("i2c="));      Serial.print(i2c_khz);                  Serial.print(F("kHz  "));
  Serial.print(F("chunk="));    Serial.print(chunk_h);                   Serial.print(F("  "));
  Serial.print(F("wait="));     Serial.print((int)chunk_wait_ms);        Serial.println(F("ms"));
}

void setup(){
  Serial.begin(115200);
  while(!Serial){;}
  Serial.println(F("\nUNO R4 + SH1107 Low-Load Power Test (0..20% dots, raw contrast 0..20)"));
  printHelp();

  Wire.begin();
  applyI2C();          // 50kHz
  u8g2.begin();
  u8g2.setPowerSave(0);

  // 初期：contrast=0, pct=0%
  applyAndRender();
  printState();
}

void loop(){
  if (Serial.available()){
    String cmd = Serial.readStringUntil('\n'); cmd.trim();

    if (cmd=="c"){ idx_contrast=(idx_contrast+1)%5; applyAndRender(); printState(); }
    else if (cmd=="C"){ idx_contrast=(idx_contrast+4)%5; applyAndRender(); printState(); }
    else if (cmd=="p"){ idx_pct=(idx_pct+1)%5; applyAndRender(); printState(); }
    else if (cmd=="P"){ idx_pct=(idx_pct+4)%5; applyAndRender(); printState(); }
    else if (cmd=="g"){ runGrid(); }
    else if (cmd.length()>=2 && cmd[0]=='t'){
      long v = cmd.substring(1).toInt(); if (v>=100) dwell_ms = (uint32_t)v;
      Serial.print(F("dwell_ms=")); Serial.println(dwell_ms);
    }
    else if (cmd.length()>=2 && cmd[0]=='i'){
      long v = cmd.substring(1).toInt(); if (v>=20 && v<=100){ i2c_khz = (uint32_t)v; applyI2C(); }
      printState();
    }
    else if (cmd.length()>=2 && cmd[0]=='b'){
      long v = cmd.substring(1).toInt(); if (v>=1 && v<=32) chunk_h = (uint8_t)v;
      printState();
    }
    else if (cmd.length()>=2 && cmd[0]=='w'){
      long v = cmd.substring(1).toInt(); if (v>=0 && v<=20) chunk_wait_ms = (uint8_t)v;
      printState();
    }
    else if (cmd=="s"){ printState(); }
    else if (cmd=="h" || cmd=="?"){ printHelp(); }
  }
}
