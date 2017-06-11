#include <ESP8266WiFi.h>
#include <SPI.h>
#include "MFRC522.h"
#include <xnsclient.h>
#include <ArduinoJson.h>

#define RST_PIN         4          // Configurable, see typical pin layout above
#define SS_PIN          5         // SDA Configurable, see typical pin layout above

/*********LED***********/
int redPin = 2;
int greenPin = 15;


/************************* xnsense MQTT Setup *********************************/

#define hw_name    "xns.bridge.rfid"
#define hw_revision    "A"
#define mqtt_server ""
#define mqtt_port  8883
#define WlanSSID ""
#define WlanPassword ""

const char * clientId = "[DeviceID]";
const char * username = "xnsense.azure-devices.net/[DeviceID]";
const char * password = "[Password]";

const char* subscribeTopic = "devices/[DeviceID]/messages/devicebound/#";
const char* publishTopic = "devices/[DeviceID]/messages/events/";

const int fw_revision[] = { 1, 0, 0, 1 };

/* wiring the MFRC522 to ESP8266 (ESP-12)
RST     = GPIO5
SDA(SS) = GPIO4 
MOSI    = GPIO13
MISO    = GPIO12
SCK     = GPIO14
GND     = GND
3.3V    = 3.3V
*/

MFRC522 rfid(SS_PIN, RST_PIN);  // Create MFRC522 instance

// Init array that will store new NUID 
byte nuidPICC[4];
MFRC522::MIFARE_Key key; 

void setup() 
{
  Serial.begin(115200);   // Initialize serial communications with the PC
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522 
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  Serial.println(F("This code scan the MIFARE Classsic NUID."));
  Serial.print(F("Using the following key:"));
  printHex(key.keyByte, MFRC522::MF_KEY_SIZE);
}
void setColor(int red, int green)
{
  if(red == 1)
  {
    digitalWrite(redPin, 0);
  }else
  {
    digitalWrite(redPin, 1);
  }
  
  if(green == 1)
  {
    digitalWrite(greenPin, 0);
  }else
  {
    digitalWrite(greenPin, 1);
  }
}
void loop() 
{ 
    
  // Look for new cards
  if ( ! rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial())
    return;

  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // Check is the PICC of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&  
    piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
    piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  if (rfid.uid.uidByte[0] != nuidPICC[0] || 
    rfid.uid.uidByte[1] != nuidPICC[1] || 
    rfid.uid.uidByte[2] != nuidPICC[2] || 
    rfid.uid.uidByte[3] != nuidPICC[3] ) {
    Serial.println(F("A new card has been detected."));

    // Store NUID into nuidPICC array
    for (byte i = 0; i < 4; i++) {
      nuidPICC[i] = rfid.uid.uidByte[i];
    }
   
    Serial.println(F("The NUID tag is:"));
    Serial.print(F("In hex: "));
    printHex(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();
    Serial.print(F("In dec: "));
    printDec(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();
  }
  else Serial.println(F("Card read previously."));

  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
}

/**
 * Helper routine to dump a byte array as hex values to Serial. 
 */
void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

/**
 * Helper routine to dump a byte array as dec values to Serial.
 */
void printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);
  }
}

void mqttMessageReceived(JsonObject& json)
{
  String vJson("");
  json.prettyPrintTo(vJson);
}
//
//String getString(char* buffer, int length, int& start)
//{
//  String data = String("");
//  char stringLength = buffer[start] & 0x3F; //remove two first bits
//  for(int i=0; i < stringLength; i++)
//  {
//    if(buffer[start+i+1] == 0xD8){
//      data += String("Ø");
//    }else{
//      data += String(buffer[start+i+1]);    
//    }
//  }
//  start += stringLength +1;
//  return data;
//}
//
//void reorganize(char* buffer, int lenght)
//{
//  for(int lineNumber = 0; lineNumber < lenght/16; lineNumber += 3){
//    swapLines(buffer, lenght, lineNumber, lineNumber+2);
//  }
//}
//
//void swapLines(char* buffer, int lenght, int line1, int line2)
//{
//  for(int i = 0; i < 16; i++){
//    char temp = buffer[line1*16+i];
//    buffer[line1*16+i] = buffer[line2*16+i];
//    buffer[line2*16+i] = temp;
//  } 
//}

