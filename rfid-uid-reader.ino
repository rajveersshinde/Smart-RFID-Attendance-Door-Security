/*
 * RFID UID Reader Utility
 * Board: ESP32 DevKit
 * Use this sketch to read card/tag UIDs before adding them to
 * the people[] array in firmware/esp32-devkit/esp32-devkit.ino.
 */

#include <SPI.h>
#include <MFRC522.h>

#define RFID_SS_PIN  5
#define RFID_RST_PIN 22

MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);

void setup() {
  Serial.begin(115200);
  SPI.begin();
  rfid.PCD_Init();
  Serial.println("Scan an RFID card or tag...");
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  String uid;
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();

  Serial.print("UID: ");
  Serial.println(uid);

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  delay(1000);
}
