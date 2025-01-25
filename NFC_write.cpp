#include <Wire.h>         // I2C communications
#include <Adafruit_PN532.h> // Imported module for NFC
#include <SPI.h>           // SPI communication
#include <Arduino.h> 

#define SDA_PIN 21
#define SCL_PIN 22
#define ROUNDS 8

Adafruit_PN532 nfc(SDA_PIN, SCL_PIN); // NFC module

uint8_t key[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};  // Default MIFARE key

int encryptedInt;  // Variable to store encrypted integer
int receivedInt;   // Variable to store the received integer

// Function declarations
int enc(int number, uint8_t *key);
bool initNFC();
void writeCard(int number);

void setup() {
  Serial.begin(115200); 
  while (!Serial);  // Wait for Serial port to be ready
  Serial.println("Initializing PN532"); 

  if (!initNFC()) {
    Serial.println("Cannot find PN532, check wiring.");
  } 

  // NFC initialized
  Serial.println("NFC Initialized, Ready to Write.");
}

void loop() {
  if (Serial.available()) {
    receivedInt = Serial.parseInt();  // Read an integer from Serial input
    if (receivedInt > 0) {
      // Process the received integer to write to the NFC card
      encryptedInt = enc(receivedInt, key);  // Encrypt the integer
      writeCard(encryptedInt);  // Write to NFC card
    }
  }
}

// Function to initialize NFC module
bool initNFC() {
  nfc.begin();  // Initialize the NFC module
  delay(100);    // Wait for the module to stabilize 
  nfc.SAMConfig(); // Configure the Secure Access Module (SAM)
  
  uint32_t SystemVersion = nfc.getFirmwareVersion(); // Get the firmware version
  if (!SystemVersion) {
    Serial.println("Cannot get PN532 version...");
    return false;
  }

  return true;
}

// Function to write an integer to the NFC card
void writeCard(int number) {
  uint8_t success;
  uint8_t uid[7];
  uint8_t uidLength;

  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
  if (success) {
    if (nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 0, key)) {
      uint8_t dataToWrite[16];
      memset(dataToWrite, 0, 16);  // Clear the data buffer
      memcpy(dataToWrite, &number, sizeof(int));  // Copy the integer to the buffer

      if (nfc.mifareclassic_WriteDataBlock(4, dataToWrite)) {
        Serial.println("Integer written to block 4 successfully!");
      } else {
        Serial.println("Failed to write to block 4.");
      }
    } else {
      Serial.println("Authentication failed. Check the key or block number.");
    }
  } else {
    Serial.println("No NFC card detected. Try again.");
  }
}

// Encryption function (XOR encryption for integer)
int enc(int number, uint8_t *key) {
  int result = number;  // Copy the integer to be encrypted
  uint8_t *resultBytes = (uint8_t *)&result;  // Access the integer as bytes

  for (int i = 0; i < ROUNDS; i++) {  // Perform ROUNDS of XOR encryption
    for (int j = 0; j < sizeof(int); j++) {
      resultBytes[j] ^= key[j % 6];  // XOR with key
    }
  }

  return result; 
}
