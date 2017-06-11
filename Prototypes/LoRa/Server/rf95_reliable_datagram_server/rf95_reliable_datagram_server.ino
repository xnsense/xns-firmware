
#include "xnsclient.h"
#include <ArduinoJson.h>

#include <RHReliableDatagram.h>
#include <RH_RF95.h>
#include <SPI.h>

#define ENCRYPTKEY    "" //exactly the same 16 characters/bytes on all nodes!
#define CLIENT_ADDRESS 1
#define SERVER_ADDRESS 2

//xns Client
#define hw_name    "xns.bridges.lora"
#define hw_revision    "A"
const int fw_revision[] = { 1, 0, 0, 1 };
#define mqtt_server ""
#define mqtt_port  8883
#define WlanSSID ""
#define WlanPassword ""

const char * clientId = "";
const char * username = "";
const char * password = "";

const char* subscribeTopic = "devices/{DeviceID}/messages/devicebound/#";
const char* publishTopic = "devices/{DeviceID}/messages/events/";

// Dont put this on the stack:
byte buf[RH_RF95_MAX_MESSAGE_LEN];
uint8_t data[] = "Ok";

//ESP Feather LoRa wing Wiring
//#define RFM95_CS  2    // "E"
//#define RFM95_RST 16   // "D"
//#define RFM95_INT 15   // "B"

// Singleton instance of the radio driver
//RH_RF95 driver;
//CS , INT
RH_RF95 driver(2, 15); // Rocket Scream Mini Ultra Pro with the RFM95W

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram manager(driver, SERVER_ADDRESS);

void setup() 
{
  //Reset Radio
  pinMode(16, OUTPUT);
  digitalWrite(16, HIGH);
  
  Serial.begin(115200);
  
  //while (!Serial) ; // Wait for serial port to be available
  if (!manager.init())
    Serial.println("init failed");
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
  Serial.println("init sucess");
  if (!driver.setFrequency(868)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(868);
  driver.setTxPower(23, false);
  //Set Encryption
  //driver.encrypt(ENCRYPTKEY);

  xnsClient.beginSecure(hw_name,hw_revision,fw_revision, WlanSSID, WlanPassword, mqtt_server, mqtt_port, clientId, username, password, publishTopic, subscribeTopic, Serial, false);
  xnsClient.setAliveMessageInterval(60);
  xnsClient.setMqttMessageCallback(mqttMessageReceived);
  
}

void mqttMessageReceived(JsonObject& json)
{
  String vJson("");
  json.prettyPrintTo(vJson);
  Serial.println("Received JSON on MQTT:");
  Serial.println(vJson);
}

void loop()
{
  xnsClient.loop();
  
  if (manager.available())
  {
    // Wait for a message addressed to us from the client
    uint8_t len = sizeof(buf);
    uint8_t from;
    if (manager.recvfromAck(buf, &len, &from))
    {
      yield();
       
      Serial.print("Got message from unit: ");
      Serial.println(from, DEC);
      
      StaticJsonBuffer<500> jsonBuffer;
      JsonObject& json = jsonBuffer.parseObject((char*)buf);

      // Send a reply back to the originator client
      if (!manager.sendtoWait(data, sizeof(data), from))
      {
        Serial.println("sendtoWait failed");
      }
      yield();
      
      Serial.println("Getting RSSI");
      //Best -15, worst -100, in db
      int8_t lastRssi = driver.lastRssi();
      //uint32_t freeHeap = xnsClient.getIndexHeapSize();
      //json["freeHeap"] = (String)freeHeap;
      json["lastRssi"] = (String)lastRssi;
      
//      int lastSNR = driver.lastSNR();
//      uint16_t rxBad = driver.rxBad();
//      uint16_t rxGood = driver.rxGood();
//      uint16_t txGood = driver.txGood();
//      json["lastSnr"] = (String)lastSNR;
//      json["rxBad"] = (String)rxBad;
//      json["rxGood"] = (String)rxGood;
//      json["txGood"] = (String)txGood;

      //Serial.print("Json Message ");
      //json.prettyPrintTo(Serial);
      Serial.println("Sending MQTT");
      yield();
      xnsClient.sendMqttData(json);
      yield();
      Serial.println("MQTT Sent");
    }
    //xnsClient.printIndexHeapSize();
  }
  //delay(150);
}

