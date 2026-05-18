#include <SPI.h>
#include <Wire.h>
#include <RTClib.h>
#include "MAX7219-Daisy.h"
#include "Better-Joystick.h"
#include "Better-GPS.h"

// Config
const uint8_t JS_X = 0;
const uint8_t JS_SW = 1;
const uint8_t JS_Y = 2;
constexpr uint8_t DIN_PIN = 3;
constexpr uint8_t CS_PIN = 4;
constexpr uint8_t CLK_PIN = 5;
constexpr uint8_t SDA_PIN = 6;
constexpr uint8_t SCL_PIN = 7;
const uint8_t BUZZER = 8;
const uint8_t GPS_TX = 9;
const uint8_t GPS_RX = 10;
constexpr uint8_t NUM_DEVICES = 6;
constexpr unsigned long GPS_BAUD = 9600UL;

MAX7219Daisy display;
RTC_DS3231 rtc;
BetterJoystick joystick;
BetterGPS gps;

// Mode management
uint8_t currentMode = 0;  // 0 = Time, 1 = Date, 2 = GPS Speed
const uint8_t NUM_MODES = 3;
unsigned long lastJoystickChange = 0;
const unsigned long JOYSTICK_DEBOUNCE = 300;

// GPS sync management
unsigned long lastGPSSync = 0;
const unsigned long GPS_SYNC_INTERVAL = 300000UL;  // 5 minutes
bool initialSyncDone = false;

// Temperature display management
unsigned long lastTempDisplay = 0;
const unsigned long TEMP_DISPLAY_INTERVAL = 300000UL;  // 5 minutes
const unsigned long TEMP_DISPLAY_DURATION = 3000UL;    // 3 seconds
bool showingTemperature = false;
unsigned long tempDisplayStart = 0;

// Hourly chime
uint8_t lastChimeHour = 255;

// Function prototypes
void playStartupMelody();
void playHourlyChime();
void playModeChangeBeep();
bool syncRTCFromGPS();
void displayTimeMode(DateTime now);
void displayDateMode(DateTime now);
void displayGPSSpeedMode();

void setup() {
  Serial.begin(115200);
  display.begin(DIN_PIN, CLK_PIN, CS_PIN, NUM_DEVICES);

  Wire.begin(SDA_PIN, SCL_PIN);

  pinMode(BUZZER, OUTPUT);

  // Startup sequence
  playStartupMelody();
  display.displayText("TILCLK");
  display.refresh();
  delay(3000);

  if (!rtc.begin()) {
    display.displayText("NO RTC");
    display.refresh();
    while (1)
      ;
  }

  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  joystick.begin(JS_SW, JS_X, JS_Y);
  gps.begin(GPS_RX, GPS_TX, GPS_BAUD);
}

void loop() {
  gps.update();

  // GPS sync on startup
  if (!initialSyncDone && gps.hasFix()) {
    if (syncRTCFromGPS()) {
      initialSyncDone = true;
      lastGPSSync = millis();
    }
  }

  // GPS sync every 5 minutes
  if (initialSyncDone && millis() - lastGPSSync >= GPS_SYNC_INTERVAL) {
    if (syncRTCFromGPS()) {
      lastGPSSync = millis();
    }
  }

  // Handle joystick mode changes
  if (millis() - lastJoystickChange >= JOYSTICK_DEBOUNCE) {
    byte direction = joystick.getDirection();

    if (direction == 2) {  // RIGHT
      currentMode = (currentMode + 1) % NUM_MODES;
      lastJoystickChange = millis();
      playModeChangeBeep();
      Serial.print("Mode changed to: ");
      Serial.println(currentMode);
    } else if (direction == 1) {  // LEFT
      currentMode = (currentMode == 0) ? (NUM_MODES - 1) : (currentMode - 1);
      lastJoystickChange = millis();
      playModeChangeBeep();
      Serial.print("Mode changed to: ");
      Serial.println(currentMode);
    }
  }

  // Display based on current mode
  DateTime now = rtc.now();

  switch (currentMode) {
    case 0:
      displayTimeMode(now);
      break;
    case 1:
      displayDateMode(now);
      break;
    case 2:
      displayGPSSpeedMode();
      break;
  }

  delay(100);
}

void playStartupMelody() {
  tone(BUZZER, 523, 150);
  delay(200);
  tone(BUZZER, 659, 150);
  delay(200);
  tone(BUZZER, 784, 200);
  delay(250);
  noTone(BUZZER);
}

void playHourlyChime() {
  tone(BUZZER, 784, 200);
  delay(250);
  tone(BUZZER, 659, 200);
  delay(250);
  tone(BUZZER, 523, 300);
  delay(350);
  noTone(BUZZER);
}

void playModeChangeBeep() {
  tone(BUZZER, 1000, 100);
  delay(150);
  noTone(BUZZER);
}

bool syncRTCFromGPS() {
  if (!gps.hasFix()) {
    return false;
  }

  int year, month, day, dayIndex, hour, minute, second;
  gps.getHungarianTime(year, month, day, dayIndex, hour, minute, second);

  if (year > 0 && month > 0 && day > 0) {
    rtc.adjust(DateTime(year, month, day, hour, minute, second));
    Serial.println("RTC synced from GPS");
    return true;
  }

  return false;
}

void displayTimeMode(DateTime now) {
  uint8_t h = now.hour();
  uint8_t m = now.minute();
  uint8_t s = now.second();

  // Check if it's time to show temperature
  if (millis() - lastTempDisplay >= TEMP_DISPLAY_INTERVAL && !showingTemperature) {
    showingTemperature = true;
    tempDisplayStart = millis();
    lastTempDisplay = millis();
  }

  if (showingTemperature) {
    if (millis() - tempDisplayStart < TEMP_DISPLAY_DURATION) {
      // Display temperature
      float temp = rtc.getTemperature();
      int tempWhole = (int)temp;
      int tempDecimal = (int)((temp - tempWhole) * 10);

      display.clearAllLEDs();

      // Display temperature digits
      if (tempWhole >= 10) {
        display.displayNumber(0, tempWhole / 10);
        display.displayNumber(1, tempWhole % 10);
      } else {
        display.clearDevice(0);
        display.displayNumber(1, tempWhole);
      }

      display.displayNumber(2, tempDecimal);
      display.setDecimalPointLED(1, true);

      display.clearDevice(3);
      display.setDegreeSymbol(4, true);
      display.displayLetter(5, 'C');

      display.refresh();
      return;
    } else {
      showingTemperature = false;
    }
  }

  // Normal time display
  display.clearAllLEDs();

  display.displayNumber(0, h / 10);
  display.displayNumber(1, h % 10);
  display.displayNumber(2, m / 10);
  display.displayNumber(3, m % 10);
  display.displayNumber(4, s / 10);
  display.displayNumber(5, s % 10);

  // Colon leds
  display.setColonLED(0, true);
  display.setColonLED(1, true);
  display.setColonLED(2, true);
  display.setColonLED(3, true);

  display.refresh();

  // Hourly chime
  if (h != lastChimeHour && m == 0 && s == 0) {
    playHourlyChime();
    lastChimeHour = h;
  }
}

void displayDateMode(DateTime now) {
  display.clearAllLEDs();

  int year = now.year();
  uint8_t month = now.month();

  // Display YYYY.MM
  display.displayNumber(0, (year / 1000) % 10);
  display.displayNumber(1, (year / 100) % 10);
  display.displayNumber(2, (year / 10) % 10);
  display.displayNumber(3, year % 10);
  display.displayNumber(4, month / 10);
  display.displayNumber(5, month % 10);

  display.setDecimalPointLED(3, true);  // Decimal point after YYYY

  display.refresh();
}

void displayGPSSpeedMode() {
  display.clearAllLEDs();

  if (!gps.hasFix()) {
    display.displayText("NO FIX");
    display.refresh();
    return;
  }

  int speed = (int)gps.getSpeedKmph();

  // Display speed with "kmh" suffix
  if (speed >= 100) {
    display.displayNumber(0, speed / 100);
    display.displayNumber(1, (speed / 10) % 10);
    display.displayNumber(2, speed % 10);
  } else if (speed >= 10) {
    display.clearDevice(0);
    display.displayNumber(1, speed / 10);
    display.displayNumber(2, speed % 10);
  } else {
    display.clearDevice(0);
    display.clearDevice(1);
    display.displayNumber(2, speed);
  }

  display.displayLetter(3, 'K');
  display.displayLetter(4, 'M');
  display.displayLetter(5, 'H');

  display.refresh();
}