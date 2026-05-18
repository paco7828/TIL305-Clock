#include <Wire.h>
#include <WiFi.h>
#include <RTClib.h>
#include "Better-Joystick.h"

// ── Pins ──────────────────────────────────────────────────────────────
#define SDA_PIN 6
#define SCL_PIN 7
#define BUZZER 20

#define COLON_HH_A 3
#define COLON_HH_B 4
#define COLON_MM_A 5
#define COLON_MM_B 8

#define JS_X 0
#define JS_SW 1
#define JS_Y 2

// ── WiFi ──────────────────────────────────────────────────────────────
const char* WIFI_SSID = "UPCD679A2A_24GHz";
const char* WIFI_PASS = "Koszegi1963";

// ── HT16K33 ───────────────────────────────────────────────────────────
const uint8_t HT16K33_ADDR[3] = { 0x71, 0x72, 0x73 };

uint16_t displaybuf[3][8];

// ── Number font ───────────────────────────────────────────────────────
const uint8_t font[10][5] = {
  { 0x3E, 0x41, 0x41, 0x41, 0x3E },
  { 0x00, 0x42, 0x7F, 0x40, 0x00 },
  { 0x62, 0x51, 0x49, 0x49, 0x46 },
  { 0x00, 0x41, 0x49, 0x49, 0x36 },
  { 0x18, 0x14, 0x12, 0x7F, 0x10 },
  { 0x27, 0x45, 0x45, 0x45, 0x39 },
  { 0x3E, 0x49, 0x49, 0x49, 0x30 },
  { 0x01, 0x01, 0x79, 0x05, 0x03 },
  { 0x36, 0x49, 0x49, 0x49, 0x36 },
  { 0x06, 0x49, 0x49, 0x49, 0x3E },
};

// ── Character font ────────────────────────────────────────────────────
const uint8_t font_T[5] = { 0x01, 0x01, 0x7F, 0x01, 0x01 };
const uint8_t font_I[5] = { 0x00, 0x41, 0x7F, 0x41, 0x00 };
const uint8_t font_L[5] = { 0x7F, 0x40, 0x40, 0x40, 0x40 };

const uint8_t font_W[5] = { 0x3F, 0x40, 0x38, 0x40, 0x3F };
const uint8_t font_F[5] = { 0x7F, 0x09, 0x09, 0x09, 0x01 };
const uint8_t font_i[5] = { 0x00, 0x40, 0x7D, 0x44, 0x00 };

const uint8_t font_C[5] = { 0x3E, 0x41, 0x41, 0x41, 0x22 };
const uint8_t font_deg[5] = { 0x00, 0x06, 0x09, 0x06, 0x00 };

// ── RTC / Joystick ────────────────────────────────────────────────────
RTC_DS3231 rtc;
BetterJoystick joystick;

uint32_t tempUntil = 0;

// ── HT16K33 ───────────────────────────────────────────────────────────
void ht16k33_cmd(uint8_t dev, uint8_t cmd) {
  Wire.beginTransmission(HT16K33_ADDR[dev]);
  Wire.write(cmd);
  Wire.endTransmission();
}

void ht16k33_init_all() {
  for (uint8_t d = 0; d < 3; d++) {
    ht16k33_cmd(d, 0x21);  // oscillator on
    ht16k33_cmd(d, 0x81);  // display on
    ht16k33_cmd(d, 0xEF);  // brightness max
  }
}

void ht16k33_flush(uint8_t dev) {

  Wire.beginTransmission(HT16K33_ADDR[dev]);
  Wire.write(0x00);

  for (uint8_t i = 0; i < 8; i++) {
    Wire.write(displaybuf[dev][i] & 0xFF);
    Wire.write((displaybuf[dev][i] >> 8) & 0xFF);
  }

  Wire.endTransmission();
}

void ht16k33_flush_all() {
  for (uint8_t d = 0; d < 3; d++) {
    ht16k33_flush(d);
  }
}

void clearDisplay() {
  memset(displaybuf, 0, sizeof(displaybuf));
}

// ── Draw digit ────────────────────────────────────────────────────────
void setDigit(uint8_t digit, uint8_t dev, uint8_t pos) {

  if (digit > 9) return;

  uint8_t col_offset = (pos == 0) ? 3 : 9;

  for (uint8_t fc = 0; fc < 5; fc++) {

    uint8_t col_data = font[digit][4 - fc];

    for (uint8_t r = 0; r < 7; r++) {

      if (col_data & (1 << r)) {
        displaybuf[dev][r + 1] |= (1u << (col_offset + fc));
      }
    }
  }
}

// ── Draw char ─────────────────────────────────────────────────────────
void setChar(const uint8_t ch[5], uint8_t dev, uint8_t pos) {

  uint8_t col_offset = (pos == 0) ? 3 : 9;

  for (uint8_t fc = 0; fc < 5; fc++) {

    uint8_t col_data = ch[4 - fc];

    for (uint8_t r = 0; r < 7; r++) {

      if (col_data & (1 << r)) {
        displaybuf[dev][r + 1] |= (1u << (col_offset + fc));
      }
    }
  }
}

// ── Colons ────────────────────────────────────────────────────────────
void setColons(bool state) {

  digitalWrite(COLON_HH_A, state);
  digitalWrite(COLON_HH_B, state);

  digitalWrite(COLON_MM_A, state);
  digitalWrite(COLON_MM_B, state);
}

// ── Pixel helper (animációhoz) ────────────────────────────────────────
// globalCol 0-29: dev0=0-9, dev1=10-19, dev2=20-29
// localCol 0-4 → bitek 3-7, localCol 5-9 → bitek 9-13
// row 0-6 → displaybuf sor 1-7
void setPixel(uint8_t globalCol, uint8_t row, bool on) {
  uint8_t dev = globalCol / 10;
  uint8_t localCol = globalCol % 10;
  uint8_t bit = (localCol < 5) ? (localCol + 3) : (localCol + 4);
  if (on)
    displaybuf[dev][row + 1] |= (1u << bit);
  else
    displaybuf[dev][row + 1] &= ~(1u << bit);
}

// ── Három független panel-animáció ────────────────────────────────────
void displayWiFiAnimations(uint32_t ms) {
  clearDisplay();
  float t = ms * 0.001f;

  // Panel 0: szinusz-hullám (sparse, fluid)
  for (uint8_t col = 0; col < 10; col++) {
    float h = 3.0f + 2.8f * sinf(col * 0.55f + t * 2.3f);
    uint8_t row = (uint8_t)constrain(h, 0.0f, 6.0f);
    setPixel(col, row, true);
    if (row > 0) setPixel(col, row - 1, true);  // trail
  }

  // Panel 1: interference mező
  for (uint8_t row = 0; row < 7; row++) {
    for (uint8_t lc = 0; lc < 10; lc++) {
      float v = sinf(lc * 0.7f + t * 1.8f)
                + sinf(row * 1.1f - t * 1.3f);
      if (v > 0.3f) setPixel(10 + lc, row, true);
    }
  }

  // Panel 2: radiális gyűrűk (saját középpontból)
  for (uint8_t row = 0; row < 7; row++) {
    for (uint8_t lc = 0; lc < 10; lc++) {
      float cx = lc - 4.5f, cy = row - 3.0f;
      float v = sinf(sqrtf(cx * cx + cy * cy) * 0.85f - t * 2.2f)
                + sinf(lc * 0.4f + t * 0.9f);
      if (v > 0.2f) setPixel(20 + lc, row, true);
    }
  }

  ht16k33_flush_all();
  setColons(false);
}

// ── Liquid fill startup animáció ──────────────────────────────────────
// ── Plasma → sötét átmenet (HT16K33 hardware brightness) ─────────────
void playDimOut() {
  for (int8_t b = 14; b >= 0; b--) {
    for (uint8_t d = 0; d < 3; d++)
      ht16k33_cmd(d, 0xE0 | (uint8_t)b);
    delay(22);
  }
  clearDisplay();
  ht16k33_flush_all();
  for (uint8_t d = 0; d < 3; d++)
    ht16k33_cmd(d, 0xEF);  // max brightness vissza
}

// ── Liquid fill + drain reveal ────────────────────────────────────────
// ── Per-display liquid fill ───────────────────────────────────────────
void fillOneDisplay(uint8_t dev) {
  const uint16_t FULL_ROW = (0x1F << 3) | (0x1F << 9);
  const uint8_t FRAMES = 38;
  const float AMP = 1.8f, FREQ = 0.38f, SLOSH = 0.35f;
  uint8_t colStart = dev * 10;

  for (uint8_t f = 0; f < FRAMES; f++) {
    for (uint8_t r = 0; r < 8; r++) displaybuf[dev][r] = 0;

    float norm = f / (float)(FRAMES - 1);
    float level = 7.0f * (1.0f - cosf(norm * 3.14159f)) * 0.5f;
    float phase = f * SLOSH;

    for (uint8_t col = 0; col < 10; col++) {
      float h = level
                + AMP * sinf((colStart + col) * FREQ + phase)
                + AMP * 0.4f * sinf((colStart + col) * FREQ * 2.1f - phase * 0.7f);
      h = constrain(h, 0.0f, 7.0f);
      uint8_t top = (h >= 7.0f) ? 0 : (uint8_t)(7.0f - h);
      for (uint8_t row = top; row < 7; row++)
        setPixel(colStart + col, row, true);
    }
    ht16k33_flush(dev);
    delay(35);
  }

  // Teljesen tele a végén
  for (uint8_t r = 1; r <= 7; r++) displaybuf[dev][r] = FULL_ROW;
  ht16k33_flush(dev);
}

// ── Per-display liquid drain (digit reveal) ───────────────────────────
void drainOneDisplay(uint8_t dev, uint16_t* target) {
  const uint16_t FULL_ROW = (0x1F << 3) | (0x1F << 9);
  const uint8_t FRAMES = 33;

  for (uint8_t f = 0; f < FRAMES; f++) {
    float prog = f / (float)(FRAMES - 1);
    float dLevel = 7.0f * (1.0f - (1.0f - prog) * (1.0f - prog));
    uint8_t dRow = (uint8_t)dLevel;

    for (uint8_t row = 0; row < 7; row++)
      displaybuf[dev][row + 1] = (row < dRow) ? target[row + 1] : FULL_ROW;

    ht16k33_flush(dev);
    delay(30);
  }

  memcpy(displaybuf[dev], target, 8 * sizeof(uint16_t));
  ht16k33_flush(dev);
}

// ── Szekvenciális fill + reveal ───────────────────────────────────────
void playFillReveal() {
  setColons(false);

  // Óra + perc target buffer előszámítás
  DateTime dt = rtc.now();
  uint16_t targetBuf[3][8];
  memset(targetBuf, 0, sizeof(targetBuf));

  clearDisplay();
  setDigit(dt.hour() / 10, 0, 0);
  setDigit(dt.hour() % 10, 0, 1);
  memcpy(targetBuf[0], displaybuf[0], 8 * sizeof(uint16_t));

  clearDisplay();
  setDigit(dt.minute() / 10, 1, 0);
  setDigit(dt.minute() % 10, 1, 1);
  memcpy(targetBuf[1], displaybuf[1], 8 * sizeof(uint16_t));

  clearDisplay();
  ht16k33_flush_all();

  // ── Display 0: fill → drain → óra ──
  fillOneDisplay(0);
  drainOneDisplay(0, targetBuf[0]);

  // ── Display 1: fill → drain → perc ──
  fillOneDisplay(1);
  drainOneDisplay(1, targetBuf[1]);

  // ── Display 2: fill → tele marad → gyors fade → másodperc ──
  fillOneDisplay(2);
  delay(200);

  // Másodperc újraolvasás közvetlenül a váltás előtt
  dt = rtc.now();
  clearDisplay();  // csak targetBuf[2] számításhoz
  setDigit(dt.second() / 10, 2, 0);
  setDigit(dt.second() % 10, 2, 1);
  memcpy(targetBuf[2], displaybuf[2], 8 * sizeof(uint16_t));

  // clearDisplay() törölte a RAM-ot, hardware még jó → visszaállítás
  memcpy(displaybuf[0], targetBuf[0], 8 * sizeof(uint16_t));
  memcpy(displaybuf[1], targetBuf[1], 8 * sizeof(uint16_t));

  // Csak display 2 brightness-fade (per-device HT16K33 parancs)
  for (int8_t b = 12; b >= 0; b -= 4) {
    ht16k33_cmd(2, 0xE0 | (uint8_t)b);
    delay(7);
  }
  memcpy(displaybuf[2], targetBuf[2], 8 * sizeof(uint16_t));
  ht16k33_flush(2);
  for (uint8_t b = 4; b <= 15; b += 4) {
    ht16k33_cmd(2, 0xE0 | b);
    delay(7);
  }
  ht16k33_cmd(2, 0xEF);

  setColons(true);
}

// ── Boot logo ─────────────────────────────────────────────────────────
void displayBootLogo() {

  clearDisplay();

  setChar(font_T, 0, 0);
  setChar(font_I, 0, 1);

  setChar(font_L, 1, 0);
  setDigit(3, 1, 1);

  setDigit(0, 2, 0);
  setDigit(5, 2, 1);

  ht16k33_flush_all();

  setColons(false);
}

// ── Time display ──────────────────────────────────────────────────────
void displayTime(DateTime t, bool colon) {

  clearDisplay();

  setDigit(t.hour() / 10, 0, 0);
  setDigit(t.hour() % 10, 0, 1);

  setDigit(t.minute() / 10, 1, 0);
  setDigit(t.minute() % 10, 1, 1);

  setDigit(t.second() / 10, 2, 0);
  setDigit(t.second() % 10, 2, 1);

  ht16k33_flush_all();

  setColons(colon);
}

// ── Temperature display ───────────────────────────────────────────────
void displayTemperature(float temp) {

  clearDisplay();
  digitalWrite(COLON_HH_A, LOW);
  digitalWrite(COLON_MM_A, LOW);
  digitalWrite(COLON_MM_B, LOW);
  digitalWrite(COLON_HH_B, HIGH);

  int t_int = (int)fabsf(temp);
  int t_frac = ((int)(fabsf(temp) * 100)) % 100;

  // 26
  setDigit((t_int / 10) % 10, 0, 0);
  setDigit(t_int % 10, 0, 1);

  // 25
  setDigit(t_frac / 10, 1, 0);
  setDigit(t_frac % 10, 1, 1);

  // °C
  setChar(font_deg, 2, 0);
  setChar(font_C, 2, 1);

  ht16k33_flush_all();
}

// ── Buzzer ────────────────────────────────────────────────────────────
void beep(uint16_t freq, uint16_t dur) {

  tone(BUZZER, freq, dur);
  delay(dur + 30);
}

void startupSound() {
  beep(262, 50);    // C4
  beep(330, 50);    // E4
  beep(392, 50);    // G4
  beep(523, 50);    // C5
  beep(659, 50);    // E5
  beep(784, 50);    // G5
  beep(1047, 120);  // C6
}

void ntpOkSound() {

  beep(784, 80);
  beep(880, 80);
  beep(1047, 180);
}

void ntpFailSound() {

  beep(300, 200);
  beep(200, 300);
}

// ── RTC sync ──────────────────────────────────────────────────────────
void writeRTCFromSystem() {

  time_t now = time(nullptr);

  struct tm* t = localtime(&now);

  rtc.adjust(DateTime(
    t->tm_year + 1900,
    t->tm_mon + 1,
    t->tm_mday,
    t->tm_hour,
    t->tm_min,
    t->tm_sec));
}

// ── WiFi + NTP ────────────────────────────────────────────────────────
bool syncNTP() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("[WiFi] Connecting");

  uint32_t t0 = millis();
  uint32_t lastFrame = 0;

  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
    uint32_t now = millis();
    if (now - lastFrame >= 50) {
      displayWiFiAnimations(now);
      lastFrame = now;
    }
    delay(5);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\n[WiFi] FAILED");
    return false;
  }

  Serial.printf("\n[WiFi] OK  IP: %s\n", WiFi.localIP().toString().c_str());

  configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org", "time.nist.gov");
  Serial.print("[NTP] Syncing");

  t0 = millis();

  // ← animáció megy NTP sync alatt is, nem fagy be
  while (time(nullptr) < 1700000000UL && millis() - t0 < 10000) {
    uint32_t now = millis();
    if (now - lastFrame >= 50) {
      displayWiFiAnimations(now);
      lastFrame = now;
    }
    delay(5);
  }

  if (time(nullptr) < 1700000000UL) {
    Serial.println("\n[NTP] FAILED");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    return false;
  }

  Serial.println("\n[NTP] OK");
  writeRTCFromSystem();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  return true;
}

// ── Setup ─────────────────────────────────────────────────────────────
void setup() {

  Serial.begin(115200);

  pinMode(COLON_HH_A, OUTPUT);
  pinMode(COLON_HH_B, OUTPUT);

  pinMode(COLON_MM_A, OUTPUT);
  pinMode(COLON_MM_B, OUTPUT);

  pinMode(BUZZER, OUTPUT);

  joystick.begin(JS_SW, JS_X, JS_Y);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  if (!rtc.begin()) {

    Serial.println("[RTC] NOT FOUND");

    while (1)
      ;
  }

  ht16k33_init_all();

  displayBootLogo();
  startupSound();
  delay(1000);

  bool ntpOk = syncNTP();

  playDimOut();

  if (ntpOk) {
    ntpOkSound();
  } else {
    ntpFailSound();
  }

  DateTime t = rtc.now();
  playFillReveal();

  Serial.printf(
    "[RTC] Boot time: %02d:%02d:%02d\n",
    t.hour(),
    t.minute(),
    t.second());

  Serial.printf(
    "[RTC] Temp: %.2f°C\n",
    rtc.getTemperature());
}

// ── Loop ──────────────────────────────────────────────────────────────
void loop() {

  static uint8_t lastSec = 255;

  if (joystick.getDirection() != 0) {
    tempUntil = millis() + 3000;
  }

  DateTime t = rtc.now();

  float temp = rtc.getTemperature();

  if (t.second() != lastSec) {
    lastSec = t.second();

    Serial.printf(
      "[RTC] %02d:%02d:%02d  %.2f°C\n",
      t.hour(), t.minute(), t.second(), temp);
  }

  if (millis() < tempUntil) {
    displayTemperature(temp);
  } else {
    displayTime(t, true);
  }

  delay(100);
}