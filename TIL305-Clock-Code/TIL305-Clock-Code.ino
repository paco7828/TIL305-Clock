#include <Wire.h>
#include <RTClib.h>
#include <Preferences.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include "Better-JoyStick.h"

// Pins
#define PIN_SDA 6
#define PIN_SCL 7
#define PIN_BUZZ 20
#define PIN_COL_HH_TOP 3
#define PIN_COL_HH_BOT 4
#define PIN_COL_MM_TOP 5
#define PIN_COL_MM_BOT 8
#define PIN_JS_X 0
#define PIN_JS_SW 1
#define PIN_JS_Y 2
#define PIN_GPS_RX 10
#define PIN_GPS_TX 9

// Joystick directions
#define DIR_LEFT 1
#define DIR_RIGHT 2
#define DIR_DOWN 3
#define DIR_UP 4

// Timing
#define GPS_SYNC_INTERVAL 60000UL
#define AUTO_CYCLE_IDLE 60000UL
#define AUTO_CYCLE_DUR 20000UL

enum class Mode : uint8_t { BOOT,
                            TIME,
                            DATE,
                            DAY,
                            TEMP,
                            SPEED,
                            ALARM };

// HT16K33 I2C addresses
const uint8_t HT_ADDR[3] = { 0x71, 0x72, 0x73 };

// --- Font ---
const uint8_t FONT_DIGITS[10][5] = {
  { 0x3E, 0x41, 0x41, 0x41, 0x3E },  // 0
  { 0x00, 0x42, 0x7F, 0x40, 0x00 },  // 1
  { 0x62, 0x51, 0x49, 0x49, 0x46 },  // 2
  { 0x00, 0x41, 0x49, 0x49, 0x36 },  // 3
  { 0x18, 0x14, 0x12, 0x7F, 0x10 },  // 4
  { 0x27, 0x45, 0x45, 0x45, 0x39 },  // 5
  { 0x3E, 0x49, 0x49, 0x49, 0x30 },  // 6
  { 0x01, 0x01, 0x79, 0x05, 0x03 },  // 7
  { 0x36, 0x49, 0x49, 0x49, 0x36 },  // 8
  { 0x06, 0x49, 0x49, 0x49, 0x3E },  // 9
};

const uint8_t FONT_ALPHA[26][5] = {
  { 0x7E, 0x09, 0x09, 0x09, 0x7E },  // A
  { 0x7F, 0x49, 0x49, 0x49, 0x36 },  // B
  { 0x3E, 0x41, 0x41, 0x41, 0x00 },  // C
  { 0x7F, 0x41, 0x41, 0x41, 0x3E },  // D
  { 0x7F, 0x49, 0x49, 0x49, 0x41 },  // E
  { 0x7F, 0x09, 0x09, 0x09, 0x01 },  // F
  { 0x3E, 0x41, 0x49, 0x49, 0x30 },  // G
  { 0x7F, 0x08, 0x08, 0x08, 0x7F },  // H
  { 0x00, 0x41, 0x7F, 0x41, 0x00 },  // I
  { 0x20, 0x40, 0x40, 0x40, 0x3F },  // J
  { 0x7F, 0x08, 0x14, 0x22, 0x41 },  // K
  { 0x7F, 0x40, 0x40, 0x40, 0x40 },  // L
  { 0x7F, 0x02, 0x04, 0x02, 0x7F },  // M
  { 0x7F, 0x03, 0x0C, 0x30, 0x7F },  // N
  { 0x3E, 0x41, 0x41, 0x41, 0x3E },  // O
  { 0x7F, 0x09, 0x09, 0x09, 0x06 },  // P
  { 0x1E, 0x21, 0x29, 0x31, 0x7E },  // Q
  { 0x7F, 0x09, 0x19, 0x29, 0x46 },  // R
  { 0x06, 0x49, 0x49, 0x49, 0x30 },  // S
  { 0x01, 0x01, 0x7F, 0x01, 0x01 },  // T
  { 0x3F, 0x40, 0x40, 0x40, 0x3F },  // U
  { 0x07, 0x18, 0x60, 0x18, 0x07 },  // V
  { 0x3F, 0x40, 0x38, 0x40, 0x3F },  // W
  { 0x63, 0x14, 0x08, 0x14, 0x63 },  // X
  { 0x03, 0x04, 0x7C, 0x04, 0x03 },  // Y
  { 0x61, 0x51, 0x49, 0x45, 0x43 },  // Z
};

const uint8_t F_lo_a[5] = { 0x30, 0x4A, 0x4A, 0x4A, 0x7C };
const uint8_t F_lo_e[5] = { 0x3C, 0x4A, 0x4A, 0x4A, 0x0C };
const uint8_t F_lo_i[5] = { 0x00, 0x40, 0x7A, 0x40, 0x00 };
const uint8_t F_lo_n[5] = { 0x7C, 0x04, 0x04, 0x04, 0x78 };
const uint8_t F_lo_r[5] = { 0x7C, 0x04, 0x04, 0x04, 0x00 };
const uint8_t F_lo_v[5] = { 0x1C, 0x20, 0x40, 0x20, 0x1C };

const uint8_t F_SPC[5] = { 0x00, 0x00, 0x00, 0x00, 0x00 };
const uint8_t F_DEG[5] = { 0x00, 0x06, 0x09, 0x06, 0x00 };
const uint8_t F_EXC[5] = { 0x00, 0x00, 0x5F, 0x00, 0x00 };
const uint8_t F_QST[5] = { 0x02, 0x01, 0x51, 0x09, 0x06 };
const uint8_t F_DOT[5] = { 0x00, 0x00, 0x60, 0x00, 0x00 };
const uint8_t F_SQ[5] = { 0x60, 0x60, 0x00, 0x00, 0x00 };
const uint8_t F_COL[5] = { 0x00, 0x00, 0x22, 0x00, 0x00 };
const uint8_t F_MIN[5] = { 0x00, 0x08, 0x08, 0x08, 0x00 };

const uint8_t* glyphFor(char c) {
  if (c >= '0' && c <= '9') return FONT_DIGITS[c - '0'];
  if (c >= 'A' && c <= 'Z') return FONT_ALPHA[c - 'A'];
  switch (c) {
    case 'a': return F_lo_a;
    case 'e': return F_lo_e;
    case 'i': return F_lo_i;
    case 'n': return F_lo_n;
    case 'r': return F_lo_r;
    case 'v': return F_lo_v;
    case 'k': return FONT_ALPHA['K' - 'A'];
    case 'p': return FONT_ALPHA['P' - 'A'];
    case 'b': return FONT_ALPHA[1];
    case 'c': return FONT_ALPHA[2];
    case 'd': return FONT_ALPHA[3];
    case 'f': return FONT_ALPHA[5];
    case 'g': return FONT_ALPHA[6];
    case 'h': return FONT_ALPHA[7];
    case 'j': return FONT_ALPHA[9];
    case 'l': return FONT_ALPHA[11];
    case 'm': return FONT_ALPHA[12];
    case 'o': return FONT_ALPHA[14];
    case 'q': return FONT_ALPHA[16];
    case 's': return FONT_ALPHA[18];
    case 't': return FONT_ALPHA[19];
    case 'u': return FONT_ALPHA[20];
    case 'w': return FONT_ALPHA[22];
    case 'x': return FONT_ALPHA[23];
    case 'y': return FONT_ALPHA[24];
    case 'z': return FONT_ALPHA[25];
    case ' ': return F_SPC;
    case '-': return F_MIN;
    case '.': return F_DOT;
    case ':': return F_COL;
    case '!': return F_EXC;
    case '?': return F_QST;
    case 0xB0: return F_DEG;
    default: return F_SPC;
  }
}

// --- HT16K33 driver ---
uint16_t frameBuf[3][8];
uint8_t colonLevel = 100;

void htWrite(uint8_t dev, uint8_t cmd) {
  Wire.beginTransmission(HT_ADDR[dev]);
  Wire.write(cmd);
  Wire.endTransmission();
}

void htSetBrightness(uint8_t lvl) {
  uint8_t cmd = 0xE0 | (uint8_t)(lvl > 7 ? 14 : lvl * 2);
  for (uint8_t d = 0; d < 3; d++) htWrite(d, cmd);
  colonLevel = map(lvl, 0, 7, 16, 255);
}

void initDisplays() {
  for (uint8_t d = 0; d < 3; d++) {
    htWrite(d, 0x21);  // oscillator on
    htWrite(d, 0x81);  // display on, no blink
    htWrite(d, 0xEF);  // max brightness
  }
}

void htFlush(uint8_t dev) {
  Wire.beginTransmission(HT_ADDR[dev]);
  Wire.write(0x00);
  for (int i = 0; i < 8; i++) {
    Wire.write(frameBuf[dev][i] & 0xFF);
    Wire.write((frameBuf[dev][i] >> 8) & 0xFF);
  }
  Wire.endTransmission();
}

void htFlushAll() {
  for (uint8_t d = 0; d < 3; d++) htFlush(d);
}

void clearBuf() {
  memset(frameBuf, 0, sizeof(frameBuf));
}

// --- Display helpers ---
void putGlyph(const uint8_t* glyph, uint8_t dev, uint8_t pos) {
  uint8_t col = (pos == 0) ? 3 : 9;
  for (uint8_t fc = 0; fc < 5; fc++) {
    uint8_t colData = glyph[4 - fc];
    for (uint8_t r = 0; r < 7; r++)
      if (colData & (1 << r)) frameBuf[dev][r + 1] |= (1u << (col + fc));
  }
}

void putChar(char c, uint8_t slot) {
  putGlyph(glyphFor(c), slot / 2, slot % 2);
}

void putDigit(uint8_t d, uint8_t slot) {
  putGlyph(FONT_DIGITS[d % 10], slot / 2, slot % 2);
}

void showStr(const char* s) {
  clearBuf();
  for (uint8_t i = 0; i < 6; i++) putChar(s[i] ? s[i] : ' ', i);
  htFlushAll();
}

void setPixel(uint8_t gc, uint8_t row, bool on) {
  uint8_t dev = gc / 10, lc = gc % 10;
  uint8_t bit = (lc < 5) ? (lc + 3) : (lc + 4);
  if (on) frameBuf[dev][row + 1] |= (1u << bit);
  else frameBuf[dev][row + 1] &= ~(1u << bit);
}

// --- Colons ---
void colonHH(bool v) {
  analogWrite(PIN_COL_HH_TOP, v ? colonLevel : 0);
  analogWrite(PIN_COL_HH_BOT, v ? colonLevel : 0);
}

void colonMM(bool v) {
  analogWrite(PIN_COL_MM_TOP, v ? colonLevel : 0);
  analogWrite(PIN_COL_MM_BOT, v ? colonLevel : 0);
}

void colonAll(bool v) {
  colonHH(v);
  colonMM(v);
}
void colonOff() {
  colonAll(false);
}

void colonHHBot(bool v) {
  analogWrite(PIN_COL_HH_TOP, 0);
  analogWrite(PIN_COL_HH_BOT, v ? colonLevel : 0);
}

void colonMMBot(bool v) {
  analogWrite(PIN_COL_MM_TOP, 0);
  analogWrite(PIN_COL_MM_BOT, v ? colonLevel : 0);
}

void showAndWait(const char* s, uint32_t ms) {
  showStr(s);
  colonOff();
  delay(ms);
}

// --- Buzzer ---
void beep(uint16_t freq, uint16_t dur) {
  tone(PIN_BUZZ, freq, dur);
  delay(dur + 20);
}

void playStartupMelody() {
  beep(262, 50);
  beep(330, 50);
  beep(392, 50);
  beep(523, 50);
  beep(659, 50);
  beep(784, 50);
  beep(1047, 120);
}

void beepClick() {
  tone(PIN_BUZZ, 1200, 30);
}
void beepMode() {
  tone(PIN_BUZZ, 900, 60);
}

void alarmBeepTick() {
  uint32_t phase = millis() % 2000;
  if (phase < 100) tone(PIN_BUZZ, 1000);
  else if (phase < 200) noTone(PIN_BUZZ);
  else if (phase < 300) tone(PIN_BUZZ, 1000);
  else if (phase < 400) noTone(PIN_BUZZ);
  else if (phase < 500) tone(PIN_BUZZ, 1000);
  else noTone(PIN_BUZZ);
}

uint32_t lastSpeedBeep = 0;
void speedBeepTick(double speed, uint8_t threshold) {
  if (speed <= threshold) return;
  double over = speed - threshold;
  int interval = max(80, (int)(1000 - over * 25));
  if ((int32_t)(millis() - lastSpeedBeep) > interval) {
    tone(PIN_BUZZ, 800 + (int)min(over * 10, 500.0), 40);
    lastSpeedBeep = millis();
  }
}

// --- Joystick input ---
BetterJoystick joy;

struct JsState {
  byte lastDir = 0;
  bool lastBtn = false;
  bool holdFired = false;
  uint32_t btnDownAt = 0;

  byte clickDir = 0;
  bool clickBtn = false;
  bool holdBtn = false;

  void update() {
    byte dir = joy.getDirection();
    bool btn = joy.getButtonPress();
    clickDir = 0;
    clickBtn = false;
    holdBtn = false;

    if (dir != 0 && lastDir == 0) clickDir = dir;
    lastDir = dir;

    if (btn && !lastBtn) {
      btnDownAt = millis();
      holdFired = false;
    }
    if (btn && !holdFired && millis() - btnDownAt >= 3000) {
      holdBtn = true;
      holdFired = true;
    }
    if (!btn && lastBtn && !holdFired) clickBtn = true;
    lastBtn = btn;
  }

  bool any() {
    return clickDir || clickBtn || holdBtn;
  }

  void consume() {
    clickDir = 0;
    clickBtn = false;
    holdBtn = false;
  }
} js;

// --- Objects ---
RTC_DS3231 rtc;
TinyGPSPlus gps;
HardwareSerial gpsSerial(1);
Preferences prefs;

void gpsSetRate(uint16_t ms) {
  uint8_t msg[] = {
    0xB5, 0x62, 0x06, 0x08, 0x06, 0x00,
    (uint8_t)(ms & 0xFF), (uint8_t)(ms >> 8),
    0x01, 0x00, 0x01, 0x00, 0x00, 0x00
  };
  uint8_t ckA = 0, ckB = 0;
  for (int i = 2; i < 12; i++) {
    ckA += msg[i];
    ckB += ckA;
  }
  msg[12] = ckA;
  msg[13] = ckB;
  gpsSerial.write(msg, 14);
}

// --- State ---
bool rtcOk = false;
bool gpsAvailable = false;
bool gpsHighRate = false;
uint8_t brightness = 5;
uint8_t lastSecond = 255;
bool displayReversed[4] = {};  // per-mode alt layout toggle (TIME, DATE, DAY, TEMP)

uint8_t speedThreshIdx = 0;
const uint8_t SPEED_LIMITS[] = { 0, 50, 70, 90, 110, 130 };

bool showingModeLabel = false;
uint32_t modeLabelEnd = 0;

struct AlarmCfg {
  uint8_t h = 0, m = 0, s = 0;
  bool enabled = false;
} alarmCfg;

bool alarmTriggered = false;
bool alarmRinging = false;
uint32_t alarmStartTime = 0;
uint8_t alarmPhase = 0;
uint8_t alarmSetH = 0, alarmSetM = 0, alarmSetS = 0;
uint32_t alarmPhaseEnd = 0;

bool showingMessage = false;
uint32_t messageEnd = 0;
Mode messageReturnMode = Mode::TIME;
char messageBuf[7] = "      ";

uint32_t lastGpsSync = 0;
float lastTemp = 20.0f;
uint32_t lastTempRead = 0;

bool autoScroll = false;
uint8_t autoScrollIdx = 255;
uint32_t autoScrollStart = 0;
uint32_t lastActivityMs = 0;

Mode currentMode = Mode::BOOT;

const char* DAY_NAMES[7] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };

static const Mode MODE_ORDER[] = {
  Mode::TIME, Mode::DATE, Mode::DAY, Mode::TEMP, Mode::SPEED, Mode::ALARM
};
static const uint8_t MODE_COUNT = 6;

// --- Time helpers ---
int calcDayOfWeek(int y, int m, int d) {
  if (m < 3) {
    m += 12;
    y--;
  }
  int k = y % 100, j = y / 100;
  return ((d + 13 * (m + 1) / 5 + k + k / 4 + j / 4 + 5 * j) + 1) % 7;
}

bool isDST(int y, int mo, int d, int h) {
  if (mo < 3 || mo > 10) return false;
  if (mo > 3 && mo < 10) return true;
  int lastSun = 31;
  while (calcDayOfWeek(y, mo, lastSun) != 0) lastSun--;
  if (mo == 3) return (d > lastSun) || (d == lastSun && h >= 1);
  if (mo == 10) return (d < lastSun) || (d == lastSun && h < 1);
  return false;
}

bool gpsHasTime(int& y, int& mo, int& d, int& dow, int& h, int& mi, int& s) {
  if (!gps.location.isValid() || !gps.date.isValid() || !gps.time.isValid()) return false;
  y = gps.date.year();
  mo = gps.date.month();
  d = gps.date.day();
  h = gps.time.hour();
  mi = gps.time.minute();
  s = gps.time.second();
  int offset = isDST(y, mo, d, h) ? 2 : 1;
  h += offset;
  if (h >= 24) {
    h -= 24;
    d++;
  }
  dow = calcDayOfWeek(y, mo, d);
  return true;
}

void gpsSyncRTC() {
  int y, mo, d, dow, h, mi, s;
  if (gpsHasTime(y, mo, d, dow, h, mi, s) && y >= 2024)
    rtc.adjust(DateTime(y, mo, d, h, mi, s));
}

// --- Animations ---
void fillOneDisplay(uint8_t dev) {
  const uint16_t FULL = (0x1F << 3) | (0x1F << 9);
  const uint8_t FRAMES = 38;
  const float AMP = 1.8f, FREQ = 0.38f, SLOSH = 0.35f;
  uint8_t cs = dev * 10;

  for (uint8_t f = 0; f < FRAMES; f++) {
    for (uint8_t r = 0; r < 8; r++) frameBuf[dev][r] = 0;
    float norm = f / (float)(FRAMES - 1);
    float level = 7.0f * (1.0f - cosf(norm * 3.14159f)) * 0.5f;
    float phase = f * SLOSH;
    for (uint8_t col = 0; col < 10; col++) {
      float h = level
                + AMP * sinf((cs + col) * FREQ + phase)
                + AMP * 0.4f * sinf((cs + col) * FREQ * 2.1f - phase * 0.7f);
      h = constrain(h, 0.0f, 7.0f);
      uint8_t top = (h >= 7.0f) ? 0 : (uint8_t)(7.0f - h);
      for (uint8_t row = top; row < 7; row++) setPixel(cs + col, row, true);
    }
    htFlush(dev);
    delay(35);
  }
  for (uint8_t r = 1; r <= 7; r++) frameBuf[dev][r] = FULL;
  htFlush(dev);
}

void drainOneDisplay(uint8_t dev, uint16_t* tgt) {
  const uint16_t FULL = (0x1F << 3) | (0x1F << 9);
  const uint8_t FRAMES = 33;

  for (uint8_t f = 0; f < FRAMES; f++) {
    float prog = f / (float)(FRAMES - 1);
    float dLevel = 7.0f * (1.0f - (1.0f - prog) * (1.0f - prog));
    uint8_t dRow = (uint8_t)dLevel;
    for (uint8_t row = 0; row < 7; row++)
      frameBuf[dev][row + 1] = (row < dRow) ? tgt[row + 1] : FULL;
    htFlush(dev);
    delay(30);
  }
  memcpy(frameBuf[dev], tgt, 8 * sizeof(uint16_t));
  htFlush(dev);
}

void fadeInColon(uint8_t pinT, uint8_t pinB) {
  for (int p = 0; p <= (int)colonLevel; p += 3) {
    analogWrite(pinT, p);
    analogWrite(pinB, p);
    delay(6);
  }
  analogWrite(pinT, colonLevel);
  analogWrite(pinB, colonLevel);
}

void playFillReveal() {
  colonOff();
  DateTime dt = rtc.now();
  uint16_t tgt[3][8];
  memset(tgt, 0, sizeof(tgt));

  clearBuf();
  putGlyph(FONT_DIGITS[dt.hour() / 10], 0, 0);
  putGlyph(FONT_DIGITS[dt.hour() % 10], 0, 1);
  memcpy(tgt[0], frameBuf[0], 8 * sizeof(uint16_t));

  clearBuf();
  putGlyph(FONT_DIGITS[dt.minute() / 10], 1, 0);
  putGlyph(FONT_DIGITS[dt.minute() % 10], 1, 1);
  memcpy(tgt[1], frameBuf[1], 8 * sizeof(uint16_t));

  clearBuf();
  htFlushAll();

  fillOneDisplay(0);
  drainOneDisplay(0, tgt[0]);
  fadeInColon(PIN_COL_HH_TOP, PIN_COL_HH_BOT);

  fillOneDisplay(1);
  drainOneDisplay(1, tgt[1]);
  fadeInColon(PIN_COL_MM_TOP, PIN_COL_MM_BOT);

  dt = rtc.now();
  clearBuf();
  putGlyph(FONT_DIGITS[dt.second() / 10], 2, 0);
  putGlyph(FONT_DIGITS[dt.second() % 10], 2, 1);
  memcpy(frameBuf[0], tgt[0], 8 * sizeof(uint16_t));
  memcpy(frameBuf[1], tgt[1], 8 * sizeof(uint16_t));
  htWrite(2, 0xE0);
  htFlushAll();
  for (uint8_t b = 0; b <= brightness * 2; b++) {
    htWrite(2, 0xE0 | b);
    delay(12);
  }
  htWrite(2, 0xE0 | (uint8_t)(brightness * 2));
  colonAll(true);
}

void runStartupTest() {
  uint8_t savedBrightness = brightness;
  htSetBrightness(7);
  colonOff();

  const char* testChars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ -.:!?";
  for (const char* p = testChars; *p; p++) {
    clearBuf();
    for (int slot = 0; slot < 6; slot++) putChar(*p, slot);
    htFlushAll();
    delay(250);
  }

  clearBuf();
  for (int slot = 0; slot < 6; slot++) putGlyph(F_DEG, slot / 2, slot % 2);
  htFlushAll();
  delay(250);

  clearBuf();
  const uint16_t ALL_ON = (0x1F << 3) | (0x1F << 9);
  for (uint8_t dev = 0; dev < 3; dev++)
    for (uint8_t row = 1; row <= 7; row++) frameBuf[dev][row] = ALL_ON;
  htFlushAll();

  for (uint8_t cycle = 0; cycle < 4; cycle++) {
    for (int8_t b = 7; b >= 0; b--) {
      htSetBrightness(b);
      delay(50);
    }
    for (int8_t b = 0; b <= 7; b++) {
      htSetBrightness(b);
      delay(50);
    }
  }
  htSetBrightness(7);
  showStr("  OK  ");
  delay(2000);

  showStr("BRIGHT");
  int steps = abs(7 - (int)savedBrightness);
  if (steps > 0) {
    int stepDelay = max(20, 1500 / steps);
    int dir = (savedBrightness < 7) ? -1 : 1;
    for (int b = 7; b != (int)savedBrightness; b += dir) {
      htSetBrightness(b);
      delay(stepDelay);
    }
  } else {
    delay(2000);
  }
  htSetBrightness(savedBrightness);
}

// --- Alarm ---
void loadAlarm() {
  alarmCfg.h = prefs.getUChar("alh", 0);
  alarmCfg.m = prefs.getUChar("alm", 0);
  alarmCfg.s = prefs.getUChar("als", 0);
  alarmCfg.enabled = prefs.getBool("ale", false);
}

void saveAlarm() {
  prefs.putUChar("alh", alarmCfg.h);
  prefs.putUChar("alm", alarmCfg.m);
  prefs.putUChar("als", alarmCfg.s);
  prefs.putBool("ale", alarmCfg.enabled);
}

void stopAlarm() {
  alarmRinging = false;
  noTone(PIN_BUZZ);
}

void checkAlarm(uint8_t h, uint8_t m, uint8_t s) {
  if (h == 0 && m == 0 && s == 0) alarmTriggered = false;
  if (!alarmCfg.enabled || alarmTriggered || alarmRinging) return;
  if (h == alarmCfg.h && m == alarmCfg.m && s == alarmCfg.s) {
    alarmRinging = true;
    alarmTriggered = true;
    alarmStartTime = millis();
  }
}

// --- Mode management ---
void showMessage(const char* msg, uint32_t dur, Mode returnTo) {
  strncpy(messageBuf, msg, 6);
  messageBuf[6] = 0;
  showStr(messageBuf);
  colonOff();
  messageEnd = millis() + dur;
  messageReturnMode = returnTo;
  showingMessage = true;
}

void adjustBrightness(int8_t delta) {
  int16_t nb = (int16_t)brightness + delta;
  if (nb > 7) {
    showMessage("MAX BR", 2000, currentMode);
    beep(1000, 50);
    beep(1000, 50);
    brightness = 7;
  } else if (nb < 0) {
    showMessage("MIN BR", 2000, currentMode);
    beep(1000, 50);
    beep(1000, 50);
    brightness = 0;
  } else {
    brightness = (uint8_t)nb;
  }
  htSetBrightness(brightness);
  prefs.putUChar("br", brightness);
}

static const char* MODE_ENTRY_MSG[] = {
  "HHMMSS",  // TIME
  "YYYYMM",  // DATE
  "DD DAY",  // DAY
  " TEMP ",  // TEMP
  "Drivin",  // SPEED
  "WakeUp",  // ALARM
};

uint8_t modeIndex(Mode m) {
  for (uint8_t i = 0; i < MODE_COUNT; i++)
    if (MODE_ORDER[i] == m) return i;
  return 0;
}

void enterMode(Mode m, bool showLabel = true) {
  if (m == Mode::SPEED && currentMode != Mode::SPEED) {
    gpsSetRate(200);
    gpsHighRate = true;
  } else if (m != Mode::SPEED && currentMode == Mode::SPEED) {
    gpsSetRate(1000);
    gpsHighRate = false;
  }
  currentMode = m;
  noTone(PIN_BUZZ);

  if (showLabel) {
    showStr(MODE_ENTRY_MSG[modeIndex(m)]);
    if (m == Mode::TIME) {
      colonHH(true);
      colonMM(true);
    } else if (m == Mode::DATE) {
      colonHH(false);
      colonMMBot(true);
    } else if (m == Mode::DAY) {
      colonHHBot(true);
      colonMM(false);
    } else colonOff();
    beepMode();
    showingModeLabel = true;
    modeLabelEnd = millis() + 3000;
  }

  if (m == Mode::ALARM) {
    alarmPhase = 0;
    alarmSetH = alarmCfg.h;
    alarmSetM = alarmCfg.m;
    alarmSetS = alarmCfg.s;
    alarmPhaseEnd = millis() + 3000;
  }
}

void nextMode() {
  enterMode(MODE_ORDER[(modeIndex(currentMode) + 1) % MODE_COUNT]);
}

void prevMode() {
  enterMode(MODE_ORDER[(modeIndex(currentMode) + MODE_COUNT - 1) % MODE_COUNT]);
}

bool handleCommon() {
  if (js.clickDir == DIR_UP) {
    adjustBrightness(+1);
    js.consume();
    return true;
  }
  if (js.clickDir == DIR_DOWN) {
    adjustBrightness(-1);
    js.consume();
    return true;
  }
  if (js.clickDir == DIR_RIGHT) {
    nextMode();
    js.consume();
    return true;
  }
  if (js.clickDir == DIR_LEFT) {
    prevMode();
    js.consume();
    return true;
  }
  return false;
}

// --- Mode: Time (HH MM SS) ---
void updateTime(const DateTime& now) {
  uint8_t h = now.hour(), m = now.minute(), s = now.second();
  clearBuf();
  if (!displayReversed[0]) {
    putDigit(h / 10, 0);
    putDigit(h % 10, 1);
    putDigit(m / 10, 2);
    putDigit(m % 10, 3);
    putDigit(s / 10, 4);
    putDigit(s % 10, 5);
  } else {
    putDigit(s / 10, 0);
    putDigit(s % 10, 1);
    putDigit(m / 10, 2);
    putDigit(m % 10, 3);
    putDigit(h / 10, 4);
    putDigit(h % 10, 5);
  }
  colonHH(true);
  colonMM(true);
  htFlushAll();

  if (js.clickBtn) {
    displayReversed[0] = !displayReversed[0];
    beepClick();
    js.consume();
  }
}

// --- Mode: Date (YYYY MM) ---
void updateDate(const DateTime& now) {
  uint16_t y = now.year();
  uint8_t mo = now.month();
  clearBuf();
  if (!displayReversed[1]) {
    putDigit((y / 1000) % 10, 0);
    putDigit((y / 100) % 10, 1);
    putDigit((y / 10) % 10, 2);
    putDigit(y % 10, 3);
    putDigit(mo / 10, 4);
    putDigit(mo % 10, 5);
    colonHH(false);
    colonMMBot(true);
  } else {
    putDigit(mo / 10, 0);
    putDigit(mo % 10, 1);
    putDigit((y / 1000) % 10, 2);
    putDigit((y / 100) % 10, 3);
    putDigit((y / 10) % 10, 4);
    putDigit(y % 10, 5);
    colonHHBot(true);
    colonMM(false);
  }
  htFlushAll();
  if (js.clickBtn) {
    displayReversed[1] = !displayReversed[1];
    beepClick();
    js.consume();
  }
}

// --- Mode: Day of week + date ---
void updateDay(const DateTime& now) {
  uint8_t dd = now.day();
  const char* dn = DAY_NAMES[now.dayOfTheWeek()];
  clearBuf();
  if (!displayReversed[2]) {
    putDigit(dd / 10, 0);
    putDigit(dd % 10, 1);
    putChar(' ', 2);
    putChar(dn[0], 3);
    putChar(dn[1], 4);
    putChar(dn[2], 5);
    colonHHBot(true);
    colonMM(false);
  } else {
    putChar(dn[0], 0);
    putChar(dn[1], 1);
    putChar(dn[2], 2);
    putChar(' ', 3);
    putDigit(dd / 10, 4);
    putDigit(dd % 10, 5);
    colonOff();
  }
  htFlushAll();
  if (js.clickBtn) {
    displayReversed[2] = !displayReversed[2];
    beepClick();
    js.consume();
  }
}

// --- Mode: Temperature ---
void updateTemp() {
  if (millis() - lastTempRead > 5000) {
    lastTemp = rtc.getTemperature();
    lastTempRead = millis();
  }
  float t = displayReversed[3] ? lastTemp * 9.0f / 5.0f + 32.0f : lastTemp;
  char u = displayReversed[3] ? 'F' : 'C';
  bool neg = (t < 0);
  float at = fabsf(t);
  int ti = (int)at;
  int tf = (int)roundf((at - ti) * 10.0f);
  if (tf >= 10) {
    ti++;
    tf = 0;
  }

  clearBuf();
  if (neg && ti < 10) {
    putChar('-', 0);
    putDigit(ti, 1);
    putDigit(tf, 2);
  } else {
    putDigit((ti / 10) % 10, 0);
    putDigit(ti % 10, 1);
    putDigit(tf, 2);
  }
  putChar(' ', 3);
  putGlyph(F_DEG, 2, 0);
  putChar(u, 5);
  colonHHBot(true);
  colonMM(false);
  htFlushAll();

  if (js.clickBtn) {
    displayReversed[3] = !displayReversed[3];
    beepClick();
    js.consume();
  }
}

// --- Mode: Speed (GPS) ---
void updateSpeed() {
  if (!gps.location.isValid() || !gps.speed.isValid()) {
    showStr("NO GPS");
    colonOff();
    return;
  }

  double spd = gps.speed.kmph();
  uint16_t sv = (uint16_t)min(spd, 999.0);
  uint8_t th = SPEED_LIMITS[speedThreshIdx];

  char buf[7];
  if (sv < 10) snprintf(buf, 7, "   %d  ", sv);
  else if (sv < 100) snprintf(buf, 7, "  %d  ", sv);
  else snprintf(buf, 7, "  %d ", sv);
  showStr(buf);
  colonOff();

  speedBeepTick(spd, th);

  if (js.clickBtn) {
    speedThreshIdx = (speedThreshIdx + 1) % 6;
    th = SPEED_LIMITS[speedThreshIdx];
    char tmsg[7];
    snprintf(tmsg, 7, th == 0 ? "NO LIM" : "%d OK", th);
    showMessage(tmsg, 2000, Mode::SPEED);
    beepClick();
    js.consume();
  }
}

// --- Mode: Alarm ---
void updateAlarm() {
  if (alarmRinging) {
    showStr("!WAKE!");
    colonOff();
    alarmBeepTick();
    if (millis() - alarmStartTime >= 60000) {
      stopAlarm();
      enterMode(Mode::TIME);
      js.consume();
      return;
    }
    if (js.any()) {
      stopAlarm();
      js.consume();
      enterMode(Mode::TIME);
    }
    return;
  }

  switch (alarmPhase) {
    case 0:
      if (millis() >= alarmPhaseEnd) {
        alarmPhase = 1;
        showStr(" SET? ");
        colonOff();
      }
      if (js.clickDir) {
        js.consume();
        enterMode(Mode::TIME);
      }
      break;

    case 1:
      showStr(" SET? ");
      colonOff();
      if (js.clickDir) {
        js.consume();
        enterMode(Mode::TIME);
        return;
      }
      if (js.clickBtn) {
        alarmPhase = 2;
        beepClick();
        js.consume();
      }
      break;

    case 2:
      {
        char buf[7];
        snprintf(buf, 7, "HOUR%02d", alarmSetH);
        showStr(buf);
        colonOff();
        if (js.clickDir == DIR_UP) {
          alarmSetH = (alarmSetH + 1) % 24;
          beepClick();
          js.consume();
        }
        if (js.clickDir == DIR_DOWN) {
          alarmSetH = (alarmSetH + 23) % 24;
          beepClick();
          js.consume();
        }
        if (js.clickBtn) {
          alarmPhase = 3;
          beepClick();
          js.consume();
        }
        break;
      }

    case 3:
      {
        char buf[7];
        snprintf(buf, 7, "MIN %02d", alarmSetM);
        showStr(buf);
        colonOff();
        if (js.clickDir == DIR_UP) {
          alarmSetM = (alarmSetM + 1) % 60;
          beepClick();
          js.consume();
        }
        if (js.clickDir == DIR_DOWN) {
          alarmSetM = (alarmSetM + 59) % 60;
          beepClick();
          js.consume();
        }
        if (js.clickBtn) {
          alarmPhase = 4;
          beepClick();
          js.consume();
        }
        break;
      }

    case 4:
      {
        char buf[7];
        snprintf(buf, 7, "SEC %02d", alarmSetS);
        showStr(buf);
        colonOff();
        if (js.clickDir == DIR_UP) {
          alarmSetS = (alarmSetS + 1) % 60;
          beepClick();
          js.consume();
        }
        if (js.clickDir == DIR_DOWN) {
          alarmSetS = (alarmSetS + 59) % 60;
          beepClick();
          js.consume();
        }
        if (js.clickBtn) {
          alarmCfg.h = alarmSetH;
          alarmCfg.m = alarmSetM;
          alarmCfg.s = alarmSetS;
          alarmCfg.enabled = true;
          saveAlarm();
          alarmTriggered = false;
          alarmPhase = 5;
          alarmPhaseEnd = millis() + 3000;
          showStr(" SET! ");
          colonOff();
          beepClick();
          delay(80);
          beepClick();
          js.consume();
        }
        break;
      }

    case 5:
      showStr(" SET! ");
      colonOff();
      if (millis() >= alarmPhaseEnd) enterMode(Mode::TIME);
      break;
  }
}

// --- Setup ---
void setup() {
  pinMode(PIN_COL_HH_TOP, OUTPUT);
  pinMode(PIN_COL_HH_BOT, OUTPUT);
  pinMode(PIN_COL_MM_TOP, OUTPUT);
  pinMode(PIN_COL_MM_BOT, OUTPUT);
  pinMode(PIN_BUZZ, OUTPUT);
  colonOff();

  joy.begin(PIN_JS_SW, PIN_JS_Y, PIN_JS_X);
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(100000);
  rtcOk = rtc.begin();

  prefs.begin("tilclock", false);
  loadAlarm();
  brightness = prefs.getUChar("br", 5);
  initDisplays();
  htSetBrightness(brightness);

  clearBuf();
  putGlyph(FONT_ALPHA['T' - 'A'], 0, 0);
  putGlyph(FONT_ALPHA['I' - 'A'], 0, 1);
  putGlyph(FONT_ALPHA['L' - 'A'], 1, 0);
  putGlyph(FONT_DIGITS[3], 1, 1);
  putGlyph(FONT_DIGITS[0], 2, 0);
  putGlyph(FONT_DIGITS[5], 2, 1);
  htFlushAll();
  colonOff();
  playStartupMelody();
  delay(1000);

  runStartupTest();

  gpsSerial.begin(9600, SERIAL_8N1, PIN_GPS_RX, PIN_GPS_TX);
  colonOff();
  uint32_t gpsWaitStart = millis();
  uint32_t lastDotUpdate = 0;
  int dotCount = 0, dotDir = 1;
  uint32_t checksumBefore = gps.passedChecksum();

  while (millis() - gpsWaitStart < 4000) {
    while (gpsSerial.available()) gps.encode(gpsSerial.read());
    if (gps.location.isValid() && gps.date.isValid() && gps.time.isValid()) {
      gpsAvailable = true;
      break;
    }
    if (millis() - lastDotUpdate > 300) {
      lastDotUpdate = millis();
      clearBuf();
      putChar('G', 0);
      putChar('P', 1);
      putChar('S', 2);
      putChar(' ', 3);
      putChar(' ', 4);
      putChar(' ', 5);
      if (dotCount >= 1) putGlyph(F_SQ, 1, 1);
      if (dotCount >= 2) putGlyph(F_SQ, 2, 0);
      if (dotCount >= 3) putGlyph(F_SQ, 2, 1);
      htFlushAll();
      colonOff();
      dotCount += dotDir;
      if (dotCount >= 3) {
        dotCount = 3;
        dotDir = -1;
      } else if (dotCount <= 0) {
        dotCount = 0;
        dotDir = 1;
      }
    }
    delay(10);
  }

  bool nmeaDetected = gps.passedChecksum() > checksumBefore;

  if (gpsAvailable) {
    showAndWait("GPS OK", 2000);
    gpsSyncRTC();
    lastGpsSync = millis();
  } else if (nmeaDetected) {
    showAndWait("NO FIX", 2000);
  } else {
    showAndWait("NO GPS", 2000);
  }

  if (!rtcOk && !gpsAvailable) {
    showAndWait("NOTIME", 3000);
    while (true) delay(100);
  } else if (rtcOk && !gpsAvailable) {
    showAndWait("RTC OK", 2000);
  }

  if (rtcOk) playFillReveal();
  enterMode(Mode::TIME, false);
  showingModeLabel = false;
  lastActivityMs = millis();
}

// --- Loop ---
void loop() {
  while (gpsSerial.available()) gps.encode(gpsSerial.read());
  js.update();

  if (js.any()) {
    lastActivityMs = millis();
    if (autoScroll) {
      autoScroll = false;
      autoScrollIdx = 255;
    }
  }

  if (showingMessage) {
    if (millis() >= messageEnd) {
      showingMessage = false;
      currentMode = messageReturnMode;
    } else {
      if (js.any()) showingMessage = false;
      else return;
    }
  }

  if (showingModeLabel) {
    if (millis() >= modeLabelEnd) {
      showingModeLabel = false;
    } else {
      if (js.clickDir == DIR_RIGHT) {
        nextMode();
        js.consume();
        return;
      }
      if (js.clickDir == DIR_LEFT) {
        prevMode();
        js.consume();
        return;
      }
      return;
    }
  }

  if (gps.location.isValid() && millis() - lastGpsSync > GPS_SYNC_INTERVAL) {
    gpsSyncRTC();
    gpsAvailable = true;
    lastGpsSync = millis();
  }

  if (rtcOk && !alarmRinging) {
    DateTime n = rtc.now();
    checkAlarm(n.hour(), n.minute(), n.second());
    if (alarmRinging) currentMode = Mode::ALARM;
  }

  if (rtcOk) {
    DateTime n = rtc.now();
    if (n.second() != lastSecond) lastSecond = n.second();
  }

  if (!autoScroll && currentMode != Mode::ALARM && currentMode != Mode::SPEED
      && millis() - lastActivityMs > AUTO_CYCLE_IDLE) {
    autoScroll = true;
    autoScrollStart = millis();
    autoScrollIdx = 255;
    enterMode(Mode::TIME, false);
  }

  if (autoScroll) {
    static const Mode CYCLE[] = { Mode::TIME, Mode::DATE, Mode::DAY };
    uint8_t idx = (uint8_t)((millis() - autoScrollStart) / AUTO_CYCLE_DUR % 3);
    if (idx != autoScrollIdx) {
      autoScrollIdx = idx;
      enterMode(CYCLE[idx], false);
    }
  }

  if (currentMode != Mode::ALARM && handleCommon()) return;

  static uint32_t lastSpeedDebounce = 0;
  const uint32_t SPEED_DEBOUNCE = 250;

  if (rtcOk) {
    DateTime now = rtc.now();
    switch (currentMode) {
      case Mode::TIME: updateTime(now); break;
      case Mode::DATE: updateDate(now); break;
      case Mode::DAY: updateDay(now); break;
      case Mode::TEMP: updateTemp(); break;
      case Mode::SPEED:
        if (js.clickBtn && millis() - lastSpeedDebounce > SPEED_DEBOUNCE) {
          lastSpeedDebounce = millis();
          updateSpeed();
        } else if (!js.clickBtn) {
          updateSpeed();
        }
        break;
      case Mode::ALARM: updateAlarm(); break;
      default: break;
    }
  } else {
    if (currentMode == Mode::SPEED) {
      if (js.clickBtn && millis() - lastSpeedDebounce > SPEED_DEBOUNCE) {
        lastSpeedDebounce = millis();
        updateSpeed();
      } else if (!js.clickBtn) {
        updateSpeed();
      }
    } else if (currentMode == Mode::ALARM) {
      updateAlarm();
    }
  }

  delay(50);
}
