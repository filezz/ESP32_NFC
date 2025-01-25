#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h> 
#include<WiFi.h> 
#include<HTTPClient.h>
#include<FS.h>
#include<SPIFFS.h>
 


#define PN532_SDA 21
#define PN532_SCL 22

#define ROUNDS 8              // Number of rounds for encryption
#define REDLED 2              
#define GREENLED 5
#define DELAY 200
#define MAX_STUDENTS 10000

#define SSID_1 "PiConf"
#define PASSWORD_1 "PiConf247"

#define PISERVER "http://145.93.130.19:5000/AllStudents" 

#define SERVER_REQUEST_TIME 1000//5 min 

Adafruit_PN532 nfc(PN532_SDA, PN532_SCL); 


uint8_t key[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};  // Default MIFARE key
bool isUser;

int StudentID[MAX_STUDENTS];
String Recv;  


IPAddress Local_IP(192,168,4,20);
IPAddress gateway(192, 168, 4,1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);


bool initNFC();
void readCard();
int dec(int number, uint8_t *key);
void WriteFile(String RawString);
void ReadFile(int *ArrayStudentID, int &count);
bool checkUser(int result, int *StudentID);


unsigned long int start_time ; 
unsigned long doorOpenTime = 0;
int servoPosition = 0;
unsigned long int current_time; 
int currentCount=0; 
int decryptedInt;
 
 void setup() {


  Serial.begin(115200);  
  WiFi.config(Local_IP, gateway,subnet,primaryDNS,secondaryDNS);

  if(!SPIFFS.begin(true)) {
    Serial.println("Failed To mount FS");
  }

  pinMode(REDLED, OUTPUT);
  pinMode(GREENLED, OUTPUT);

  if (!initNFC()) {
    Serial.println("Cannot find PN532, check wiring.");
    while (1);  // Stop program if initNFC fails  
  }
  Serial.println("Ready...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID_1, PASSWORD_1); 

  WiFi.mode(WIFI_STA);
WiFi.begin(SSID_1, PASSWORD_1);
unsigned long wifiStartTime = millis();

while (WiFi.status() != WL_CONNECTED && millis() - wifiStartTime < 10000) {
    delay(1000);
    Serial.println("Trying to connect...");
}
if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection failed.");
    return;
}

  
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.gatewayIP());
  Serial.println(WiFi.subnetMask());
  Serial.println(WiFi.dnsIP()); 



  start_time = millis();

}


void loop() {
  
    if (millis() - start_time > SERVER_REQUEST_TIME) {
        if (WiFi.status() == WL_CONNECTED) {
            HTTPClient Flask;
            Flask.begin(PISERVER);

            int Response = Flask.GET();
            if (Response == HTTP_CODE_OK) {
                Recv = Flask.getString();
                WriteFile(Recv);
            } else {
                Serial.println("Error: " + String(Response));
            }
            Flask.end();
        } else {
            Serial.println("WiFi not connected.");
        }
    }

    ReadFile(StudentID, currentCount);
    readCard();

    isUser = checkUser(decryptedInt, StudentID);

    //Serial.println(isUser);
    if(!isUser)
      Serial.println("Not Ok") ;
    else 
      Serial.println("oK");

}
// Function to initialize NFC module
bool initNFC() {
  if (!nfc.begin()) {
    Serial.println(F("PN532 initialization failed. Check wiring."));
    return false;
  }

  uint32_t version = nfc.getFirmwareVersion();  
  if (!version) {
    Serial.println(F("Cannot get PN532 firmware version..."));
    return false;
  }

  Serial.print(F("Found PN532 with firmware version: "));
  Serial.print((version >> 16) & 0xFF, DEC);
  Serial.print(F("."));
  Serial.println((version >> 8) & 0xFF, DEC);

  nfc.SAMConfig(); // Configure Secure Access Module (SAM)
  return true;
}
void readCard() {
  uint8_t uid[7];    // Buffer to store the card's unique identifier
  uint8_t uid_len;   // Length of the UID

  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uid_len)) {

    // Authenticate to access a specific block on the card
    if (nfc.mifareclassic_AuthenticateBlock(uid, uid_len, 4, 0, key)) {
      uint8_t dataBlock[16]; // Temporary buffer for reading a block of data from the card

      if (nfc.mifareclassic_ReadDataBlock(4, dataBlock)) { // Read block 4 of data
        // Extract the integer from the data block
        int readInt = 0;
        memcpy(&readInt, dataBlock, sizeof(int)); // Copy the first 4 bytes as an integer

        // Decrypt the integer using the `dec` function
        decryptedInt = dec(readInt, key);

        // Print or store the decrypted integer value
        Serial.print(F("Decrypted integer: "));
        Serial.println(decryptedInt);
      } else {
        Serial.println(F("Failed to read data from block 4."));
      }
    } else {
      Serial.println(F("Authentication failed."));
    }
  } else {
    Serial.println(F("No card detected."));
  }
}

// Function to decrypt the integer
int dec(int number, uint8_t *key) {
  int result = number;                
  uint8_t *resultBytes = (uint8_t *)&result; 

  for (int i = 0; i < ROUNDS; i++) {  
    for (int j = 0; j < sizeof(int); j++) { 
      resultBytes[j] ^= key[j % 6];   
    }
  }

  return result; 
}

bool checkUser(int result , int *StudentArray) {
  for (int i = 0; i < currentCount; i++)
      if(result == StudentArray[i])
        return true;
  return false;
} 
void WriteFile(String RawString)
{
  RawString.replace("[","");
  RawString.replace("]" ,""); 
  RawString.replace("\"","");

  Serial.print(RawString); 

  int startIndex = 0;
  int Comma = RawString.indexOf(',');  

  File file = SPIFFS.open("/Students.txt",FILE_WRITE);
  if(!file){
    Serial.print("failed to open file for writing");
    return; 
  }
  while(Comma != -1)
  {
    String StudentID = RawString.substring(startIndex,Comma);
    file.println(StudentID);
    startIndex = Comma+1; 
    Comma = RawString.indexOf(',',startIndex);
  }
  String LastId = RawString.substring(startIndex);
  file.println(LastId);
  
  file.close();
}
void ReadFile(int *ArrayStudentID, int &count)
{
    File file = SPIFFS.open("/Students.txt",FILE_READ);
    if(!file) 
      Serial.print("Can not Open File");

    while(file.available() && count < MAX_STUDENTS)
    {
      String line = file.readStringUntil('\n');
      line.trim();
      if(line.length() > 0){
        ArrayStudentID[count] = line.toInt();
        count++;
      } 
    } 
    file.close();
}