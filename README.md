# TIL305 Clock

An ESP32-based retro dot-matrix desk clock using six TIL305 5x7 LED displays, driven by three HT16K33 I2C LED drivers. Features GPS-synced time, DS3231 RTC, six display modes, joystick navigation, buzzer feedback, and a liquid-fill boot animation.

---

## Hardware

| Component | Description |
|-----------|-------------|
| MCU | ESP32 |
| Displays | 6x TIL305 (5x7 dot-matrix, 10-column per HT16K33) |
| LED Drivers | 3x HT16K33 (I2C addresses 0x71, 0x72, 0x73) |
| RTC | DS3231M (I2C) |
| GPS | NEO-6M (UART, 9600 baud) |
| Input | Analog joystick (X/Y + button) |
| Audio | Passive buzzer |
| Colons | 4x PWM-driven LED indicators (HH top/bot, MM top/bot) |

### Pin Assignment

| Signal | GPIO |
|--------|------|
| I2C SDA | 6 |
| I2C SCL | 7 |
| Buzzer | 20 |
| Colon HH Top | 3 |
| Colon HH Bottom | 4 |
| Colon MM Top | 5 |
| Colon MM Bottom | 8 |
| Joystick X | 0 |
| Joystick SW | 1 |
| Joystick Y | 2 |
| GPS RX (ESP32 side) | 10 |
| GPS TX (ESP32 side) | 9 |

---

## Display Layout

The six TIL305 modules are addressed as slots 0-5 left-to-right. Each pair of modules is driven by one HT16K33:

```
[ slot 0 | slot 1 ] -- HT16K33 @ 0x71
[ slot 2 | slot 3 ] -- HT16K33 @ 0x72
[ slot 4 | slot 5 ] -- HT16K33 @ 0x73
```

Column mapping within each HT16K33: left glyph uses columns 3-7, right glyph uses columns 9-13.

---

## Display Modes

Modes are cycled left/right with the joystick. Joystick up/down adjusts brightness.

| Mode | Display | Button toggle |
|------|---------|--------------|
| TIME | `HH MM SS` | Reversed: `SS MM HH` |
| DATE | `YYYY MM` | Reversed: `MM YYYY` |
| DAY | `DD DAY` (e.g. `14 MON`) | Reversed: `DAY DD` |
| TEMP | `XX.X C` / `XX.X F` | Toggle Celsius / Fahrenheit |
| SPEED | GPS speed in km/h | Cycle speed warning threshold |
| ALARM | Alarm set/trigger | Step through hour/min/sec setup |

---

## Features

### Time Sync

- **Primary:** DS3231M RTC (battery-backed, always-on)
- **Secondary:** NEO-6M GPS UTC time, converted to Hungarian local time (CET/CEST) with automatic DST calculation
- GPS sync runs every 60 seconds when a fix is available
- On boot, waits up to 4 seconds for GPS fix; falls back to RTC if unavailable
- GPS rate is increased to 5 Hz (200 ms) when in SPEED mode, restored to 1 Hz otherwise

### Boot Sequence

1. Splash screen: `TIL305`
2. Startup melody
3. Full display test (all characters, brightness sweep)
4. GPS acquisition attempt with animated indicator
5. Liquid fill + drain reveal animation displaying current time
6. Enter TIME mode

### Alarm

- Set via joystick in ALARM mode (hour, minute, second)
- Stored in NVS (`Preferences`) across power cycles
- Rings for up to 60 seconds; any joystick input stops it
- Display shows `!WAKE!` during alarm

### Speed Warning

- Cycles through thresholds: off / 50 / 70 / 90 / 110 / 130 km/h
- Buzzer beep interval shortens as speed exceeds threshold
- Shows `NO GPS` if no valid fix

### Brightness

- 8 levels (0-7), stored in NVS
- Colon LED brightness tracks display brightness via PWM

### Auto-scroll

- After 60 seconds of inactivity, cycles TIME / DATE / DAY every 20 seconds
- Any joystick input cancels auto-scroll

---

## Joystick Controls

| Input | Action |
|-------|--------|
| Left / Right | Previous / Next mode |
| Up | Brightness +1 |
| Down | Brightness -1 |
| Click | Mode-specific action (reverse layout, toggle unit, etc.) |
| Hold 3 s | (Reserved for future use) |

Deadzone: raw ADC values within 100 counts of center are treated as neutral. Values snapping to +/-100 (mapped) trigger directional input.

---

## Dependencies

- `RTClib` (Adafruit DS3231)
- `TinyGPSPlus`
- `Preferences` (built-in ESP32 Arduino)
- `Wire`, `HardwareSerial` (built-in)

---

## Font Coverage

The built-in 5x7 bitmap font covers:

- Digits `0-9`
- Uppercase `A-Z`
- Lowercase `a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z`
- Special: space, `-`, `.`, `:`, `!`, `?`, degree symbol (`0xB0`)

---

## Notes

- I2C clock is set to 100 kHz to ensure stable communication across all three HT16K33 drivers on the same bus
- Colon LEDs are driven via `analogWrite()` (PWM) rather than digital to allow brightness matching
- GPS DST boundary calculation uses Zeller's Congruence for day-of-week; Hungarian CEST transitions (last Sunday of March/October) are handled explicitly
