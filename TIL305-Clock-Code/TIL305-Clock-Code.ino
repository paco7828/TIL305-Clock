#include <SPI.h>
#include <Wire.h>
#include <RTClib.h>
#include "MAX7219-Daisy.h"

// Config
constexpr uint8_t DIN_PIN = 3;
constexpr uint8_t CS_PIN = 4;
constexpr uint8_t CLK_PIN = 5;
constexpr uint8_t SDA_PIN = 6;
constexpr uint8_t SCL_PIN = 7;
constexpr uint8_t NUM_DEVICES = 6;

MAX7219Daisy display;
RTC_DS3231 rtc;

void setup() {
  Serial.begin(115200);
  display.begin(DIN_PIN, CLK_PIN, CS_PIN, NUM_DEVICES);

  Wire.begin(SDA_PIN, SCL_PIN);

  if (!rtc.begin()) {
    display.displayText("NO RTC");
    display.refresh();
    while (1)
      ;
  }

  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

void loop() {
  DateTime now = rtc.now();

  uint8_t h = now.hour();
  uint8_t m = now.minute();
  uint8_t s = now.second();

  Serial.print("TIME: ");
  Serial.print(h);
  Serial.print(":");
  Serial.print(m);
  Serial.print(":");
  Serial.println(s);

  // Hour
  display.displayNumber(0, h / 10);
  display.displayNumber(1, h % 10);

  // Minute
  display.displayNumber(2, m / 10);
  display.displayNumber(3, m % 10);

  // Second
  display.displayNumber(4, s / 10);
  display.displayNumber(5, s % 10);

  display.refresh();
  delay(1000);
}