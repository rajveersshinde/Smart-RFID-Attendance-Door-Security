/*
 * Smart RFID Attendance, Door Access & Security System
 * Board: AI Thinker ESP32-CAM
 *
 * Receives a trigger from ESP32 DevKit GPIO 32 on ESP32-CAM GPIO 13.
 * Captures a JPEG and sends it to a Telegram chat.
 *
 * Select "AI Thinker ESP32-CAM" in Arduino IDE.
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "esp_camera.h"

// ---------------- USER CONFIGURATION ----------------
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* TELEGRAM_BOT_TOKEN = "YOUR_BOT_TOKEN";
const char* TELEGRAM_CHAT_ID = "YOUR_CHAT_ID";
// ----------------------------------------------------

#define TRIGGER_PIN 13  // Connected to ESP32 DevKit GPIO 32

// AI Thinker ESP32-CAM camera pin mapping
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

unsigned long lastPhotoMillis = 0;
const unsigned long PHOTO_COOLDOWN_MS = 5000;

bool initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  return esp_camera_init(&config) == ESP_OK;
}

bool sendPhotoToTelegram() {
  // Discard one frame so the uploaded image is as recent as possible.
  camera_fb_t* oldFrame = esp_camera_fb_get();
  if (oldFrame) esp_camera_fb_return(oldFrame);

  camera_fb_t* frame = esp_camera_fb_get();
  if (!frame) {
    Serial.println("Camera capture failed");
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure(); // For production, validate Telegram's certificate.
  client.setTimeout(10000);

  if (!client.connect("api.telegram.org", 443)) {
    Serial.println("Telegram connection failed");
    esp_camera_fb_return(frame);
    return false;
  }

  const String boundary = "----ESP32CAMBoundary7MA4YWxk";
  String head =
    "--" + boundary + "\r\n"
    "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n" +
    String(TELEGRAM_CHAT_ID) + "\r\n" +
    "--" + boundary + "\r\n"
    "Content-Disposition: form-data; name=\"caption\"\r\n\r\n"
    "Unknown card detected!\r\n"
    "--" + boundary + "\r\n"
    "Content-Disposition: form-data; name=\"photo\"; filename=\"photo.jpg\"\r\n"
    "Content-Type: image/jpeg\r\n\r\n";

  String tail = "\r\n--" + boundary + "--\r\n";
  size_t contentLength = head.length() + frame->len + tail.length();

  client.println("POST /bot" + String(TELEGRAM_BOT_TOKEN) + "/sendPhoto HTTP/1.1");
  client.println("Host: api.telegram.org");
  client.println("Content-Type: multipart/form-data; boundary=" + boundary);
  client.println("Content-Length: " + String(contentLength));
  client.println("Connection: close");
  client.println();
  client.print(head);

  for (size_t offset = 0; offset < frame->len; offset += 1024) {
    size_t chunkSize = min((size_t)1024, frame->len - offset);
    client.write(frame->buf + offset, chunkSize);
  }
  client.print(tail);
  esp_camera_fb_return(frame);

  String response;
  unsigned long start = millis();
  while (millis() - start < 10000) {
    while (client.available()) response += (char)client.read();
    if (!client.connected() && !client.available()) break;
    delay(10);
  }
  client.stop();

  bool success = response.indexOf("\"ok\":true") >= 0;
  Serial.println(success ? "Photo sent to Telegram" : "Telegram send failed");
  if (!success) Serial.println(response);
  return success;
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIGGER_PIN, INPUT_PULLDOWN);

  if (!initCamera()) {
    Serial.println("Camera initialization failed. Check board selection and camera.");
    while (true) delay(1000);
  }

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected");
  Serial.println(WiFi.localIP());
}

void loop() {
  if (digitalRead(TRIGGER_PIN) == HIGH &&
      millis() - lastPhotoMillis >= PHOTO_COOLDOWN_MS) {
    lastPhotoMillis = millis();
    Serial.println("Trigger received - capturing photo");
    sendPhotoToTelegram();
  }
  delay(20);
}
