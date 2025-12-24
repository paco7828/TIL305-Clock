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

// Your custom pattern
bool pattern[5][7] = {
  { 1, 1, 1, 0, 0, 0, 0 },  // Row 1
  { 0, 0, 0, 0, 0, 0, 0 },  // Row 2
  { 0, 0, 0, 0, 0, 0, 0 },  // Row 3
  { 0, 0, 0, 0, 0, 0, 0 },  // Row 4
  { 0, 0, 0, 0, 0, 0, 0 }   // Row 5
};

bool decimalOn = false;

void setup() {
  // Set up row pins (active HIGH)
  for (int i = 0; i < 6; i++) {
    pinMode(rowPins[i], OUTPUT);
    digitalWrite(rowPins[i], LOW);
  }

  // Set up column pins (active LOW)
  for (int i = 0; i < COLS; i++) {
    pinMode(colPins[i], OUTPUT);
    digitalWrite(colPins[i], HIGH);
  }
}

void loop() {
  displayPattern();
}

void displayPattern() {
  // Multiplexing: scan through each row
  for (int row = 0; row < ROWS; row++) {
    // Turn off everything
    allOff();

    // Set column states for this row
    for (int col = 0; col < COLS; col++) {
      digitalWrite(colPins[col], pattern[row][col] ? LOW : HIGH);
    }

    // Turn on current row
    digitalWrite(rowPins[row], HIGH);

    delay(2);  // Brief delay for persistence of vision
  }

  // Handle decimal point (row 6, col 6 - IO5, IO7)
  if (decimalOn) {
    allOff();
    digitalWrite(colPins[5], LOW);   // Col 6 (IO7)
    digitalWrite(rowPins[5], HIGH);  // Row 6 (IO5)
    delay(2);
  }
}