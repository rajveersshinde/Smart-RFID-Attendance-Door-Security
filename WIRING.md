# Wiring Reference

This document is a quick wiring checklist. See the main README for setup and safety notes.

## ESP32 DevKit

| DevKit GPIO | Connection |
|---:|---|
| 4 | Mode push button (other terminal to GND) |
| 5 | RC522 SDA/SS |
| 13 | Servo signal |
| 18 | RC522 SCK |
| 19 | RC522 MISO |
| 22 | RC522 RST |
| 23 | RC522 MOSI |
| 25 | OLED SDA |
| 26 | OLED SCL |
| 27 | Buzzer positive |
| 32 | ESP32-CAM GPIO 13 trigger |

## Shared power

- RC522: 3.3V and GND.
- OLED: 3.3V and GND.
- Servo: regulated 5V and GND; share ground with DevKit.
- ESP32-CAM: its own stable 5V source and common GND with DevKit.
- Button: GPIO 4 to GND; configured with `INPUT_PULLUP`.

## Before powering on

1. Check that the RC522 is not connected to 5V.
2. Verify the ESP32-CAM trigger wire is DevKit GPIO 32 to CAM GPIO 13.
3. Verify all grounds share a common reference.
4. Do not connect independent 5V outputs together.
5. Keep the servo mechanically unloaded during first tests.
