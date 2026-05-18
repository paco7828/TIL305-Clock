#include "number-bits.h"
#include "letter-bits.h"

class MAX7219Daisy {
private:
  uint8_t dinPin;
  uint8_t clkPin;
  uint8_t csPin;
  uint8_t numberOfDevices;

  uint8_t displayBuffer[8][8];

  static constexpr uint8_t REG_NOOP = 0x00;
  static constexpr uint8_t REG_DIGIT0 = 0x01;
  static constexpr uint8_t REG_DIGIT1 = 0x02;
  static constexpr uint8_t REG_DIGIT2 = 0x03;
  static constexpr uint8_t REG_DIGIT3 = 0x04;
  static constexpr uint8_t REG_DIGIT4 = 0x05;
  static constexpr uint8_t REG_DIGIT5 = 0x06;
  static constexpr uint8_t REG_DIGIT6 = 0x07;
  static constexpr uint8_t REG_DIGIT7 = 0x08;
  static constexpr uint8_t REG_DECODE = 0x09;
  static constexpr uint8_t REG_INTENSITY = 0x0A;
  static constexpr uint8_t REG_SCANLIMIT = 0x0B;
  static constexpr uint8_t REG_SHUTDOWN = 0x0C;
  static constexpr uint8_t REG_DISPLAYTEST = 0x0F;

  void writeToAllDevices(byte reg, byte data) {
    digitalWrite(csPin, LOW);

    for (int i = 0; i < numberOfDevices; i++) {
      shiftOut(dinPin, clkPin, MSBFIRST, reg);
      shiftOut(dinPin, clkPin, MSBFIRST, data);
    }

    digitalWrite(csPin, HIGH);
  }

  void writeToDevice(uint8_t deviceIndex, byte reg, byte data) {
    digitalWrite(csPin, LOW);

    for (int i = numberOfDevices - 1; i >= 0; i--) {
      if (i == deviceIndex) {
        shiftOut(dinPin, clkPin, MSBFIRST, reg);
        shiftOut(dinPin, clkPin, MSBFIRST, data);
      } else {
        shiftOut(dinPin, clkPin, MSBFIRST, REG_NOOP);
        shiftOut(dinPin, clkPin, MSBFIRST, 0x00);
      }
    }

    digitalWrite(csPin, HIGH);
  }

public:
  void begin(uint8_t dinPin, uint8_t clkPin, uint8_t csPin, uint8_t numberOfDevices) {
    this->dinPin = dinPin;
    this->clkPin = clkPin;
    this->csPin = csPin;
    this->numberOfDevices = numberOfDevices;

    pinMode(csPin, OUTPUT);
    pinMode(dinPin, OUTPUT);
    pinMode(clkPin, OUTPUT);

    digitalWrite(csPin, HIGH);

    for (uint8_t i = 0; i < 8; i++) {
      for (uint8_t j = 0; j < 8; j++) {
        displayBuffer[i][j] = 0x00;
      }
    }

    writeToAllDevices(REG_SHUTDOWN, 0x00);
    writeToAllDevices(REG_DISPLAYTEST, 0x00);
    writeToAllDevices(REG_DECODE, 0x00);
    writeToAllDevices(REG_SCANLIMIT, 0x07);  // Changed to 0x07 to use all 8 rows
    writeToAllDevices(REG_INTENSITY, 0x0F);
    writeToAllDevices(REG_SHUTDOWN, 0x01);

    clearDisplay();
  }

  void displayNumber(uint8_t deviceIndex, uint8_t number) {
    if (number > 9 || deviceIndex >= numberOfDevices)
      return;

    for (uint8_t row = 0; row < 7; row++) {
      uint8_t rowData = 0;

      for (uint8_t col = 0; col < 5; col++) {
        if (digits[number][row][col]) {
          rowData |= (1 << (col + 2));
        }
      }

      displayBuffer[deviceIndex][row] = rowData;
    }

    // Clear row 7 (the LED row) for this device
    displayBuffer[deviceIndex][7] = 0x00;
  }

  void displayLetter(uint8_t deviceIndex, char letter) {
    if (deviceIndex >= numberOfDevices)
      return;

    if (letter >= 'a' && letter <= 'z')
      letter = letter - 'a' + 'A';

    if (letter < 'A' || letter > 'Z')
      return;

    uint8_t letterIndex = letter - 'A';

    for (uint8_t row = 0; row < 7; row++) {
      uint8_t rowData = 0;

      for (uint8_t col = 0; col < 5; col++) {
        if (letters[letterIndex][row][col]) {
          rowData |= (1 << (col + 2));
        }
      }

      displayBuffer[deviceIndex][row] = rowData;
    }

    // Clear row 7 (the LED row) for this device
    displayBuffer[deviceIndex][7] = 0x00;
  }

  void displayText(const char* text) {
    uint8_t len = strlen(text);

    for (uint8_t i = 0; i < numberOfDevices && i < len; i++) {
      char c = text[i];

      if (c == ' ') {
        clearDevice(i);
      }

      else if (c >= '0' && c <= '9') {
        displayNumber(i, c - '0');
      }

      else if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
        displayLetter(i, c);
      }

      else {
        clearDevice(i);
      }
    }

    for (uint8_t i = len; i < numberOfDevices; i++) {
      clearDevice(i);
    }
  }

  void clearDevice(uint8_t deviceIndex) {
    if (deviceIndex >= numberOfDevices)
      return;

    for (int row = 0; row < 8; row++) {
      displayBuffer[deviceIndex][row] = 0x00;
    }
  }

  // Set the colon LED (left LED on row 7)
  void setColonLED(uint8_t deviceIndex, bool state) {
    if (deviceIndex >= numberOfDevices)
      return;

    if (state) {
      displayBuffer[deviceIndex][7] |= 0x02;  // Set bit 1 (left LED)
    } else {
      displayBuffer[deviceIndex][7] &= ~0x02;  // Clear bit 1
    }
  }

  // Set the decimal point LED (right LED on row 7)
  void setDecimalPointLED(uint8_t deviceIndex, bool state) {
    if (deviceIndex >= numberOfDevices)
      return;

    if (state) {
      displayBuffer[deviceIndex][7] |= 0x80;  // Set bit 7 (right LED)
    } else {
      displayBuffer[deviceIndex][7] &= ~0x80;  // Clear bit 7
    }
  }

  // Set the degree symbol on row 0 and row 1
  void setDegreeSymbol(uint8_t deviceIndex, bool state) {
    if (deviceIndex >= numberOfDevices)
      return;

    if (state) {
      displayBuffer[deviceIndex][0] = 0x18;  // Bits 3 and 4 for top of degree
      displayBuffer[deviceIndex][1] = 0x18;  // Bits 3 and 4 for bottom of degree
    } else {
      displayBuffer[deviceIndex][0] = 0x00;
      displayBuffer[deviceIndex][1] = 0x00;
    }
  }

  // Clear all LEDs (row 7) on all devices
  void clearAllLEDs() {
    for (uint8_t i = 0; i < numberOfDevices; i++) {
      displayBuffer[i][7] = 0x00;
    }
  }

  void refresh() {
    for (uint8_t row = 0; row < 8; row++) {  // Changed to 8 rows
      digitalWrite(csPin, LOW);

      for (int i = numberOfDevices - 1; i >= 0; i--) {
        shiftOut(dinPin, clkPin, MSBFIRST, row + 1);
        shiftOut(dinPin, clkPin, MSBFIRST, displayBuffer[i][row]);
      }

      digitalWrite(csPin, HIGH);
    }
  }

  void clearDisplay() {
    for (uint8_t i = 0; i < numberOfDevices; i++) {
      for (int row = 0; row < 8; row++) {  // Changed to 8 rows
        displayBuffer[i][row] = 0x00;
      }
    }
    refresh();
  }
};