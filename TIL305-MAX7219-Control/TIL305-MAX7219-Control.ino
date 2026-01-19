/*
  Wiring:
  ESP32-C3 Supermini -> MAX7219
  IO3 -> DIN
  IO4 -> CS
  IO5 -> CLK
  
  MAX7219 -> TIL305
  Rows (active LOW):
  DIG0 -> Pin 2  (Row 1)
  DIG1 -> Pin 3  (Row 2)
  DIG2 -> Pin 4  (Row 3)
  DIG3 -> Pin 11 (Row 4)
  DIG4 -> Pin 10 (Row 5)
  DIG5 -> Pin 5  (Row 6)
  DIG6 -> Pin 9  (Row 7)
  
  Columns (active HIGH):
  SEGA -> Pin 12 (Col 5)
  SEGB -> Pin 8  (Col 4)
  SEGC -> Pin 1  (Col 3)
  SEGD -> Pin 7  (Col 2)
  SEGE -> Pin 6  (Col 1)
  
  
  ISET -> 16kΩ -> 5V
  V+ -> 5V
  GND -> GND
*/

#include <SPI.h>

// Pin definitions
#define DIN_PIN 3
#define CS_PIN 4
#define CLK_PIN 5

// MAX7219 Register addresses
#define REG_NOOP 0x00
#define REG_DIGIT0 0x01
#define REG_DIGIT1 0x02
#define REG_DIGIT2 0x03
#define REG_DIGIT3 0x04
#define REG_DIGIT4 0x05
#define REG_DIGIT5 0x06
#define REG_DIGIT6 0x07
#define REG_DIGIT7 0x08
#define REG_DECODE 0x09
#define REG_INTENSITY 0x0A
#define REG_SCANLIMIT 0x0B
#define REG_SHUTDOWN 0x0C
#define REG_DISPLAYTEST 0x0F

const bool digits[10][7][5] = {
  // 0
  {
    { 0, 1, 1, 1, 0 },
    { 1, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 1 },
    { 0, 1, 1, 1, 0 } },
  // 1
  {
    { 0, 0, 1, 0, 0 },
    { 0, 1, 1, 0, 0 },
    { 0, 0, 1, 0, 0 },
    { 0, 0, 1, 0, 0 },
    { 0, 0, 1, 0, 0 },
    { 0, 0, 1, 0, 0 },
    { 0, 1, 1, 1, 0 } },
  // 2
  {
    { 0, 1, 1, 1, 0 },
    { 1, 0, 0, 0, 1 },
    { 0, 0, 0, 0, 1 },
    { 0, 0, 0, 1, 0 },
    { 0, 0, 1, 0, 0 },
    { 0, 1, 0, 0, 0 },
    { 1, 1, 1, 1, 1 } },
  // 3
  {
    { 0, 1, 1, 1, 0 },
    { 1, 0, 0, 0, 1 },
    { 0, 0, 0, 0, 1 },
    { 0, 0, 1, 1, 0 },
    { 0, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 1 },
    { 0, 1, 1, 1, 0 } },
  // 4
  {
    { 0, 0, 0, 1, 0 },
    { 0, 0, 1, 1, 0 },
    { 0, 1, 0, 1, 0 },
    { 1, 0, 0, 1, 0 },
    { 1, 1, 1, 1, 1 },
    { 0, 0, 0, 1, 0 },
    { 0, 0, 0, 1, 0 } },
  // 5
  {
    { 1, 1, 1, 1, 1 },
    { 1, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 0 },
    { 0, 0, 0, 0, 1 },
    { 0, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 1 },
    { 0, 1, 1, 1, 0 } },
  // 6
  {
    { 0, 1, 1, 1, 0 },
    { 1, 0, 0, 0, 0 },
    { 1, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 0 },
    { 1, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 1 },
    { 0, 1, 1, 1, 0 } },
  // 7
  {
    { 1, 1, 1, 1, 1 },
    { 0, 0, 0, 0, 1 },
    { 0, 0, 0, 1, 0 },
    { 0, 0, 1, 0, 0 },
    { 0, 1, 0, 0, 0 },
    { 0, 1, 0, 0, 0 },
    { 0, 1, 0, 0, 0 } },
  // 8
  {
    { 0, 1, 1, 1, 0 },
    { 1, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 1 },
    { 0, 1, 1, 1, 0 },
    { 1, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 1 },
    { 0, 1, 1, 1, 0 } },
  // 9
  {
    { 0, 1, 1, 1, 0 },
    { 1, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 1 },
    { 0, 1, 1, 1, 1 },
    { 0, 0, 0, 0, 1 },
    { 0, 0, 0, 0, 1 },
    { 0, 1, 1, 1, 0 } }
};

void writeRegister(byte reg, byte data) {
  digitalWrite(CS_PIN, LOW);
  shiftOut(DIN_PIN, CLK_PIN, MSBFIRST, reg);
  shiftOut(DIN_PIN, CLK_PIN, MSBFIRST, data);
  digitalWrite(CS_PIN, HIGH);
}

void initMAX7219() {
  pinMode(CS_PIN, OUTPUT);
  pinMode(DIN_PIN, OUTPUT);
  pinMode(CLK_PIN, OUTPUT);

  digitalWrite(CS_PIN, HIGH);

  writeRegister(REG_SHUTDOWN, 0x00);     // Shutdown mode
  writeRegister(REG_DISPLAYTEST, 0x00);  // Normal operation
  writeRegister(REG_DECODE, 0x00);       // No decode mode (raw data)
  writeRegister(REG_SCANLIMIT, 0x06);    // Scan 7 digits (0-6)
  writeRegister(REG_INTENSITY, 0x0F);    // Maximum brightness
  writeRegister(REG_SHUTDOWN, 0x01);     // Normal operation

  // Clear display
  for (int i = 1; i <= 7; i++) {
    writeRegister(i, 0x00);
  }
}

void displayDigit(int digit) {
  if (digit < 0 || digit > 9) return;
  // For each row in the display
  for (int row = 0; row < 7; row++) {
    byte rowData = 0;
    for (int col = 0; col < 5; col++) {
      if (digits[digit][row][col]) {
        rowData |= (1 << (col + 2));
      }
    }
    writeRegister(row + 1, rowData);
  }
}

void clearDisplay() {
  for (int row = 1; row <= 7; row++) {
    writeRegister(row, 0x00);
  }
}

void setBrightness(byte level) {
  // level: 0-15
  writeRegister(REG_INTENSITY, level & 0x0F);
}

void setup() {
  initMAX7219();
}

void loop() {
  // Cycle through digits 0-9
  for (int i = 0; i < 10; i++) {
    displayDigit(i);
    delay(1000);
  }
  clearDisplay();
}