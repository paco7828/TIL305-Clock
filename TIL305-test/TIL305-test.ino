/*
--- WIRING ---
TIL305 -> MCU

Columns (active HIGH):
!!! Use 150R / 220R resistors on ALL row pins!!!
TIL305 Pin 12 -> IO10 (col 1)
TIL305 Pin 8  -> IO9  (col 2)
TIL305 Pin 1  -> IO20 (col 3)
TIL305 Pin 7  -> IO21 (col 4)
TIL305 Pin 6  -> IO8  (col 5)

Rows (active LOW):
TIL305 Pin 2  -> IO0 (row 1)
TIL305 Pin 3  -> IO1 (row 2)
TIL305 Pin 4  -> IO2 (row 3)
TIL305 Pin 11 -> IO3 (row 4)
TIL305 Pin 10 -> IO4 (row 5)
TIL305 Pin 5  -> IO5 (row 6)
TIL305 Pin 9  -> IO6 (row 7)

Decimal Point:
TIL305 Pin 13 -> IO7
*/

// Pin definitions
const int rowPins[7] = { 0, 1, 2, 3, 4, 5, 6 };
const int colPins[5] = { 10, 9, 20, 21, 8 };
const int decimalPin = 7;

// Matrix dimensions
const int COLS = 5;
const int ROWS = 7;

void setup() {
  // Set up row pins (active LOW)
  for (int i = 0; i < ROWS; i++) {
    pinMode(rowPins[i], OUTPUT);
    digitalWrite(rowPins[i], HIGH);
  }

  // Set up column pins (active HIGH)
  for (int i = 0; i < COLS; i++) {
    pinMode(colPins[i], OUTPUT);
    digitalWrite(colPins[i], LOW);
  }

  // Set up decimal point pin
  pinMode(decimalPin, OUTPUT);
  digitalWrite(decimalPin, LOW);
}

void loop() {
  rowScan();
  delay(500);
  columnScan();
  delay(500);
  blinkAll();
  delay(500);
}

void allOff() {
  // Turn off all rows
  for (int i = 0; i < ROWS; i++) {
    digitalWrite(rowPins[i], HIGH);
  }

  // Turn off all columns
  for (int i = 0; i < COLS; i++) {
    digitalWrite(colPins[i], LOW);
  }

  // Turn off decimal point
  digitalWrite(decimalPin, LOW);
}

void rowScan() {
  // Turn on all columns
  for (int col = 0; col < COLS; col++) {
    digitalWrite(colPins[col], HIGH);
  }

  // Scan through each row
  for (int row = 0; row < ROWS; row++) {
    allOff();

    // Turn on all columns
    for (int col = 0; col < COLS; col++) {
      digitalWrite(colPins[col], HIGH);
    }

    // Turn on current row
    digitalWrite(rowPins[row], LOW);
    delay(150);
  }

  // Decimal point test
  allOff();
  digitalWrite(decimalPin, HIGH);
  digitalWrite(rowPins[6], LOW);
  allOff();
}

void columnScan() {
  // Turn on all rows
  for (int row = 0; row < ROWS; row++) {
    digitalWrite(rowPins[row], LOW);
  }

  // Scan through each column
  for (int col = 0; col < COLS; col++) {
    allOff();

    // Turn on all rows
    for (int row = 0; row < ROWS; row++) {
      digitalWrite(rowPins[row], LOW);
    }

    // Turn on current column
    digitalWrite(colPins[col], HIGH);
    delay(150);
  }
  allOff();
}

void blinkAll() {
  for (int i = 0; i < 5; i++) {
    // All on
    for (int row = 0; row < ROWS; row++) {
      digitalWrite(rowPins[row], LOW);
    }
    for (int col = 0; col < COLS; col++) {
      digitalWrite(colPins[col], HIGH);
    }
    delay(5000);

    // All off
    allOff();
  }
}