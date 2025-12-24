/*
--- WIRING ---
TIL305 -> MCU

Rows:
!!! Use 150R / 220R resistors!!!
IO0 -> pin 1 (row3)
IO1 -> pin 6 (row1)
IO2 -> pin 7 (row2)
IO3 -> pin 8 (row4)
IO4 -> pin 12 (row5)
IO5 -> pin 13 (row6)

Columns:
IO6 -> pin 2 (col7)
IO7 -> pin 3 (col6)
IO8 -> pin 4 (col5)
IO9 -> pin 11 (col2)
IO10 -> pin 10 (col1)
IO20 -> pin 5 (col3)
IO21 -> pin 9 (col4)
*/

// Pin definitions
const int rowPins[6] = { 1, 2, 0, 3, 4, 5 };        // Rows 1-6
const int colPins[7] = { 10, 9, 20, 21, 8, 7, 6 };  // Cols 1-7

// Matrix dimensions
const int COLS = 7;
const int ROWS = 5;
const int DECIMAL_ROW = 5;

void setup() {
  // Set up row pins (active HIGH)
  for (int i = 0; i < 6; i++) {
    pinMode(rowPins[i], OUTPUT);
    digitalWrite(rowPins[i], LOW);  // Start LOW (off)
  }

  // Set up column pins (active LOW)
  for (int i = 0; i < COLS; i++) {
    pinMode(colPins[i], OUTPUT);
    digitalWrite(colPins[i], HIGH);  // Start HIGH (off)
  }
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
  for (int i = 0; i < 6; i++) {
    digitalWrite(rowPins[i], LOW);
  }

  // Turn off all columns
  for (int i = 0; i < COLS; i++) {
    digitalWrite(colPins[i], HIGH);
  }
}

void rowScan() {
  // Turn on all columns (set them LOW)
  for (int col = 0; col < COLS; col++) {
    digitalWrite(colPins[col], LOW);
  }

  // Scan through each row
  for (int row = 0; row < ROWS; row++) {
    // Turn on current row
    digitalWrite(rowPins[row], HIGH);
    delay(150);
  }

  // Test decimal point
  digitalWrite(rowPins[DECIMAL_ROW], HIGH);

  // Turn off all columns
  delay(3000);
  allOff();
}

void columnScan() {
  // Turn on all rows (set them HIGH)
  for (int row = 0; row < ROWS; row++) {
    digitalWrite(rowPins[row], HIGH);
  }

  // Scan through each column
  for (int col = 0; col < COLS; col++) {
    // Turn on current column
    digitalWrite(colPins[col], LOW);
    delay(150);
  }

  // Turn off all rows
  delay(3000);
  allOff();
}

void blinkAll() {
  for (int i = 0; i < 5; i++) {
    // All on
    for (int row = 0; row < ROWS; row++) {
      digitalWrite(rowPins[row], HIGH);
    }
    for (int col = 0; col < COLS; col++) {
      digitalWrite(colPins[col], LOW);
    }
    delay(300);

    // All off
    allOff();
    delay(300);
  }
}