/*
 * Smart RFID Attendance, Door Access & Security System
 * Board: ESP32 DevKit (classic ESP32)
 *
 * Libraries:
 * - MFRC522
 * - Adafruit GFX Library
 * - Adafruit SSD1306
 * - ESP32Servo
 *
 * Configure Wi-Fi, Apps Script URL, and card UIDs below before uploading.
 */

#include <SPI.h>
#include <Wire.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>

// ---------------- USER CONFIGURATION ----------------
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* GOOGLE_SCRIPT_URL = "YOUR_DEPLOYED_APPS_SCRIPT_WEB_APP_URL";

struct Person {
  const char* uid;   // Uppercase hexadecimal UID, no spaces
  const char* name;
};

Person people[] = {
  {"AABBCCDD", "Rajveer"},
  {"11223344", "Tag1"}
};
// ----------------------------------------------------

// Pin assignments
#define RFID_SS_PIN       5
#define RFID_RST_PIN      22
#define OLED_SDA_PIN      25
#define OLED_SCL_PIN      26
#define BUZZER_PIN        27
#define SERVO_PIN         13
#define MODE_BUTTON_PIN   4
#define CAM_TRIGGER_PIN   32

#define SERVO_CLOSED      0
#define SERVO_OPEN        90

#define SCREEN_WIDTH      128
#define SCREEN_HEIGHT     64
#define OLED_ADDRESS      0x3C

const char* modeNames[] = {"ATTENDANCE", "DOOR", "SECURITY"};
const int MODE_COUNT = 3;

MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Servo doorServo;

int currentMode = 0;
unsigned long lastButtonPress = 0;
String lastUid = "";
unsigned long lastScanMillis = 0;

void showScreen(const String& line1, const String& line2 = "") {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Mode: ");
  display.println(modeNames[currentMode]);
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 20);
  display.println(line1);

  display.setTextSize(1);
  display.setCursor(0, 48);
  display.println(line2);
  display.display();
}

void showIdle() {
  showScreen("Scan card");
}

void beep(int count, int onMs, int offMs) {
  for (int i = 0; i < count; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(onMs);
    digitalWrite(BUZZER_PIN, LOW);
    if (i < count - 1) delay(offMs);
  }
}

void openCloseDoor() {
  doorServo.write(SERVO_OPEN);
  delay(3000);
  doorServo.write(SERVO_CLOSED);
  delay(500);
}

void triggerCamera() {
  digitalWrite(CAM_TRIGGER_PIN, HIGH);
  delay(200);
  digitalWrite(CAM_TRIGGER_PIN, LOW);
}

String readCardUID() {
  String uid;
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  return uid;
}

int findPerson(const String& uid) {
  const size_t count = sizeof(people) / sizeof(people[0]);
  for (size_t i = 0; i < count; i++) {
    if (uid == people[i].uid) return (int)i;
  }
  return -1;
}

String urlEncode(const String& input) {
  const char hex[] = "0123456789ABCDEF";
  String encoded;
  for (size_t i = 0; i < input.length(); i++) {
    uint8_t c = (uint8_t)input[i];
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') || c == '-' || c == '_' ||
        c == '.' || c == '~') {
      encoded += (char)c;
    } else {
      encoded += '%';
      encoded += hex[c >> 4];
      encoded += hex[c & 0x0F];
    }
  }
  return encoded;
}

void sendToGoogleSheet(const String& uid, const String& name,
                       const String& status) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Google Sheets upload skipped: Wi-Fi disconnected");
    return;
  }

  String url = String(GOOGLE_SCRIPT_URL) +
    "?uid=" + urlEncode(uid) +
    "&name=" + urlEncode(name) +
    "&status=" + urlEncode(status) +
    "&mode=" + urlEncode(modeNames[currentMode]);

  WiFiClientSecure client;
  client.setInsecure(); // For a production deployment, validate the server certificate.
  HTTPClient https;
  https.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  Serial.println("Sending scan record to Google Sheets...");
  if (https.begin(client, url)) {
    int code = https.GET();
    Serial.printf("[HTTPS] Response code: %d\n", code);
    if (code > 0) Serial.println(https.getString());
    https.end();
  } else {
    Serial.println("Unable to start HTTPS connection");
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  pinMode(CAM_TRIGGER_PIN, OUTPUT);
  digitalWrite(CAM_TRIGGER_PIN, LOW);
  pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);

  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED not found. Check I2C wiring/address.");
  } else {
    showScreen("Starting...");
  }

  doorServo.setPeriodHertz(50);
  doorServo.attach(SERVO_PIN);
  doorServo.write(SERVO_CLOSED);

  SPI.begin();
  rfid.PCD_Init();

  showScreen("WiFi...", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi connected");
  Serial.println(WiFi.localIP());
  showIdle();
}

void loop() {
  // Button is active LOW. Simple debounce; one press changes one mode.
  if (digitalRead(MODE_BUTTON_PIN) == LOW &&
      millis() - lastButtonPress > 300) {
    lastButtonPress = millis();
    currentMode = (currentMode + 1) % MODE_COUNT;
    beep(1, 50, 0);
    showIdle();
  }

  if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    delay(500);
    return;
  }

  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  String uid = readCardUID();
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  // Ignore the same UID if scanned again within three seconds.
  if (uid == lastUid && millis() - lastScanMillis < 3000) return;
  lastUid = uid;
  lastScanMillis = millis();

  Serial.println("Scanned UID: " + uid);
  int personIndex = findPerson(uid);

  if (personIndex >= 0) {
    String personName = people[personIndex].name;
    showScreen("ACCEPTED", personName + " " + uid);
    beep(1, 150, 0);

    if (currentMode >= 1) openCloseDoor();
    sendToGoogleSheet(uid, personName, "Accepted");
  } else {
    showScreen("DECLINED", "Unknown " + uid);

    if (currentMode == 2) triggerCamera();
    beep(3, 400, 200);
    sendToGoogleSheet(uid, "Unknown", "Declined");
  }

  delay(500);
  showIdle();
}
