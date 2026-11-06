# Smart RFID Attendance, Door Access & Security System

An ESP32-based multi-mode system that combines RFID identification, attendance logging, servo-based door access, an OLED status display, audible alerts, and ESP32-CAM security photography with Telegram notifications.

> **Project type:** Second-year engineering / embedded systems project  
> **Core platform:** ESP32 DevKit + AI Thinker ESP32-CAM  
> **Backend:** Google Sheets via Google Apps Script  
> **Notification:** Telegram Bot API

## Features

The system has three selectable modes. Press the mode button to cycle through them.

| Feature | Attendance | Door | Security |
|---|:---:|:---:|:---:|
| OLED accepted/declined message | ✅ | ✅ | ✅ |
| Buzzer on declined card | ✅ | ✅ | ✅ |
| Google Sheets scan log | ✅ | ✅ | ✅ |
| Servo opens for accepted card | — | ✅ | ✅ |
| Telegram photo on unknown card | — | — | ✅ |

### Operating notes

- The active mode is held in RAM; after a restart or power loss, the system starts in **Attendance** mode.
- The mode button produces one short beep when the mode changes.
- Re-scanning the same card within three seconds is ignored.
- The ESP32-CAM has a five-second photo cooldown to prevent repeated scans from flooding Telegram.
- The Google Sheet records name, UID, status, mode, date, and time.

## System architecture

```text
                     RFID Card / Tag
                            |
                            v
                    +---------------+
                    | RC522 Reader  |
                    +-------+-------+
                            |
                            v
+-------------+     +-------+--------+      +----------------+
| Mode Button |---->|                 |----->| OLED Display   |
+-------------+     |   ESP32 DevKit |      +----------------+
                    |                 |
                    |                 |-----> Buzzer
                    |                 |
                    |                 |-----> Servo / Door
                    |                 |
                    |                 |---- GPIO 32 trigger
                    +-------+---------+               |
                            |                         v
                            |                  +-------------+
                            |                  | ESP32-CAM   |
                            |                  +------+------+
                            |                         |
                  Wi-Fi     |                         | Wi-Fi
                            v                         v
                    +---------------+         +----------------+
                    | Google Sheets |         | Telegram Bot   |
                    | Apps Script   |         | Photo Alert    |
                    +---------------+         +----------------+
```

## Hardware requirements

| Component | Purpose |
|---|---|
| ESP32 DevKit (classic ESP32) | Main controller |
| AI Thinker ESP32-CAM | Captures security photos |
| MFRC522 RFID reader | Reads RFID card/tag UIDs |
| RFID cards or tags | User identification |
| 0.96-inch SSD1306 OLED, I2C | Displays mode and scan result |
| Servo motor | Door lock/door model actuator |
| Active buzzer | Audio feedback |
| Push button | Changes operating mode |
| Breadboard and jumper wires | Prototyping |
| Regulated 5V supply for servo and ESP32-CAM | Stable power |

## Wiring

### ESP32 DevKit connections

| ESP32 GPIO | Component | Component pin |
|---:|---|---|
| 4 | Mode button | One leg; other leg to GND |
| 5 | RC522 | SDA / SS |
| 13 | Servo | Signal |
| 18 | RC522 | SCK |
| 19 | RC522 | MISO |
| 22 | RC522 | RST |
| 23 | RC522 | MOSI |
| 25 | OLED display | SDA |
| 26 | OLED display | SCL |
| 27 | Buzzer | Positive (+) |
| 32 | ESP32-CAM | GPIO 13 trigger input |

### Power and ground

| Component | Power | Ground |
|---|---|---|
| RC522 | 3.3V | GND |
| OLED display | 3.3V | GND |
| Servo | Regulated 5V | GND |
| Buzzer | Signal pin from GPIO 27 (as wired) | GND |
| Mode button | No separate supply | GND |

**Important power guidance**
- The RC522 is a **3.3V device**. Do not power it from 5V.
- A servo can draw substantial startup/stall current. Use a suitable regulated 5V supply rather than relying on the ESP32 board's 5V rail. Connect the servo supply ground to ESP32 GND.
- Power the ESP32-CAM from a stable 5V supply or its own USB programming/power board. Connect its GND to DevKit GND so the trigger signal has a common reference.
- Do not connect two 5V outputs together. Check your board's power arrangement before connecting USB and an external supply simultaneously.

### ESP32-CAM connections

| ESP32-CAM pin | Connects to |
|---|---|
| GPIO 13 | DevKit GPIO 32 |
| GND | DevKit GND |
| 5V | Its own USB board or regulated 5V supply |

The ESP32-CAM GPIO 13 is used as the trigger input in this project. Avoid using the microSD interface at the same time unless you redesign the pin assignment.

## Repository structure

```text
Smart-RFID-Attendance-Door-Security/
├── backend/
│   └── google-apps-script/
│       └── Code.gs
├── docs/
│   ├── WIRING.md
│   └── images/
│       ├── devkit-pinout-reference.png
│       ├── esp32-cam-connections.png
│       ├── feature-summary.png
│       └── buzzer-connections.png
├── firmware/
│   ├── esp32-devkit/
│   │   └── esp32-devkit.ino
│   ├── esp32-cam/
│   │   └── esp32-cam.ino
│   └── rfid-uid-reader/
│       └── rfid-uid-reader.ino
├── .gitignore
├── LICENSE
└── README.md
```

## Software setup

### 1. Install Arduino IDE and board support

Install Arduino IDE and add the Espressif ESP32 board package using the official Arduino-ESP32 installation instructions.

Select:
- Main controller: your ESP32 Dev Module / ESP32 DevKit board.
- Camera board: **AI Thinker ESP32-CAM**.

### 2. Install libraries

In Arduino IDE Library Manager, install:

For the ESP32 DevKit sketch:
- MFRC522
- Adafruit GFX Library
- Adafruit SSD1306
- ESP32Servo

The ESP32-CAM sketch uses the ESP32 camera library included with the ESP32 Arduino board package.

### 3. Read your RFID card UIDs

1. Open `firmware/rfid-uid-reader/rfid-uid-reader.ino`.
2. Upload it to the ESP32 DevKit.
3. Open Serial Monitor at **115200 baud**.
4. Scan each card/tag and copy the printed UID.
5. Add each UID and display name to the `people[]` array in the main DevKit sketch.

Example:

```cpp
Person people[] = {
  {"A1B2C3D4", "Student One"},
  {"11223344", "Student Two"}
};
```

UIDs must be uppercase hexadecimal strings without spaces.

### 4. Configure Google Sheets

1. Create a Google Spreadsheet.
2. Create or rename a tab to `Sheet1`.
3. Open **Extensions → Apps Script**.
4. Replace the contents with `backend/google-apps-script/Code.gs`.
5. Set `SPREADSHEET_ID` to your spreadsheet ID.
6. Save the project.
7. Deploy as a **Web app**.
8. Set the execution identity and access policy appropriate for your use case, then deploy.
9. Copy the deployed `/exec` URL into `GOOGLE_SCRIPT_URL` in the DevKit sketch.

The script creates the header row automatically when the sheet is empty.

**Security note:** A web app that accepts unauthenticated GET requests can be abused to write false records. For a real deployment, add an authentication mechanism (for example, a shared secret checked by the script), restrict access where possible, and do not publish private deployment details.

### 5. Configure Telegram

1. Create a Telegram bot using the official BotFather account.
2. Copy the bot token.
3. Obtain the destination chat ID.
4. Start a conversation with the bot (or add it to the intended group).
5. Set `TELEGRAM_BOT_TOKEN` and `TELEGRAM_CHAT_ID` in the ESP32-CAM sketch.

Keep the bot token private. Anyone who has it may be able to control your bot.

### 6. Configure Wi-Fi and upload

In both firmware sketches, set:

```cpp
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
```

Upload the DevKit and ESP32-CAM sketches to their respective boards. Ensure both boards use the same Wi-Fi network.

### 7. Test the system

- Scan a registered card in Attendance mode: OLED shows accepted, buzzer beeps, and a row is logged.
- Scan an unknown card: OLED shows declined, buzzer sounds, and a declined row is logged.
- Switch to Door mode: an accepted card opens the servo for approximately three seconds.
- Switch to Security mode: an accepted card opens the servo; an unknown card triggers the ESP32-CAM and Telegram photo alert.

## Google Sheets log format

| Name | UID | Status | Mode | Date | Time |
|---|---|---|---|---|---|
| Student One | A1B2C3D4 | Accepted | ATTENDANCE | 06/11/2026 | 09:15:30 |
| Unknown | 99887766 | Declined | SECURITY | 06/11/2026 | 09:16:10 |



## Important limitations and future improvements

- The current card allow-list is stored directly in firmware. A future version could load users from a secure backend.
- The current mode is not saved across restarts.
- The servo is a prototype actuator, not a certified physical access-control lock.
- The current Apps Script endpoint should be protected before deployment in a real environment.
- `WiFiClientSecure::setInsecure()` skips certificate verification. Use certificate validation for production deployments.
- Consider adding offline event buffering, retry logic, OTA updates, and authenticated API requests.

## Troubleshooting

| Problem | Checks |
|---|---|
| RC522 not detecting cards | Confirm 3.3V power, SPI wiring, SS GPIO 5, RST GPIO 22 |
| OLED blank | Check 3.3V, GND, SDA GPIO 25, SCL GPIO 26, and address `0x3C` |
| Servo jitters or ESP32 restarts | Use a separate regulated 5V supply and common ground |
| Google Sheet not updating | Check deployed Apps Script URL, deployment access, spreadsheet ID, tab name, and Serial Monitor response |
| ESP32-CAM not taking photos | Check GPIO 32 → GPIO 13, common GND, stable 5V, camera board selection, and Wi-Fi |
| Telegram photo not arriving | Check bot token, chat ID, bot conversation permissions, and Serial Monitor output |

## License

This project is released under the MIT License. See [LICENSE](LICENSE).

## Acknowledgements

Built as a second-year engineering project using the ESP32 ecosystem, MFRC522 RFID module, SSD1306 OLED, Google Apps Script, and Telegram Bot API.
