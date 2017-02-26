

// Select your modem:
//#define TINY_GSM_MODEM_SIM800
//#define TINY_GSM_MODEM_SIM900
#define TINY_GSM_MODEM_A7
//#define TINY_GSM_MODEM_M590
#define mqtt_server "79.161.196.15"
#define mqtt_port  1883

#define TINY_GSM_DEBUG Serial
#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Your GPRS credentials
// Leave empty, if missing user or pass
const char apn[]  = "telenor.smart";
const char user[] = "";
const char pass[] = "";


// or Software Serial on Uno, Nano
#include <SoftwareSerial.h>
SoftwareSerial SerialAT(12, 13); // RX, TX

TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient mqtt(client);

const char * clientId = "11:11:11:11:11:11";
const char * username = "";
const char * password = "";

const char* publishTopic = "sensors/out/11:11:11:11:11:11";
const char* subscribeTopic = "";
unsigned long lastkeepAliveTime = 0;


#define LED_PIN 5
int ledStatus = LOW;

long lastReconnectAttempt = 0;

void setup() {
  pinMode(LED_PIN, OUTPUT);

  // Set console baud rate
  Serial.begin(115200);
  while (!Serial) {}
  Serial.println("Serial port initialized...");

  // Set GSM module baud rate
  SerialAT.begin(115200);
  delay(3000);

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  Serial.println("Initializing modem...");
  modem.restart();

  // Unlock your SIM card with a PIN
  //modem.simUnlock("1234");

  Serial.print("Waiting for network...");
  if (!modem.waitForNetwork()) {
    Serial.println(" fail");
    while (true);
  }
  Serial.println(" OK");

  Serial.print("Connecting to ");
  Serial.print(apn);
  if (!modem.gprsConnect(apn, user, pass)) {
    Serial.println(" fail");
    while (true);
  }
  Serial.println(" OK");
  Serial.print("Signal Quality: ");
  Serial.println(modem.getSignalQuality());
  // MQTT Broker setup
  mqtt.setServer(mqtt_server, mqtt_port);
}


void loop() 
{

  //getSignalQuality() //returns int
//
//  switch(modem.getRegistrationStatus())
//  {
//    case 1:
//    case 5:
//    //Serial.println("Start");
//    if (!mqtt.connected()) 
//    {
//      yield(); 
//      //Serial.println("Middel");
//      reconnect();
//    }
//    
//     yield();
//     //Serial.println("Almost End");
//     mqtt.loop();
//     //Serial.println("End");  
//     break;
//     default: 
//     Serial.println("Lost Connection");
//     break;    
//  }
    
    
    
    
  mqtt.loop();
  delay(10);
  if (!mqtt.connected()) 
  {
    yield(); 
    Serial.println("Re-connect");
    reconnect();
  }

  yield();
  if(millis() - lastkeepAliveTime > 60000){
      lastkeepAliveTime = millis();
      Serial.println("Send MQTT Alive");
      StaticJsonBuffer<500> jsonBuffer;
      JsonObject& json = jsonBuffer.createObject();
      json["id"] = clientId;
      json["up"] = millis() / 1000;
      json["hwName"] = "xns.test.gsm";
      json["hwRevision"] = "A";
      json["fwRevision"] = "1.0.0.1";
      json["message"] = "I'm alive!";
    
      String msg;
      json.printTo(msg);
    
      mqtt.publish(publishTopic, msg.c_str());
  }
  delay(5000);
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqtt.connected()) 
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    yield();
    if (mqtt.connect(clientId)) 
    {
      yield();
      Serial.println("connected");
      yield();
      // Once connected, publish an announcement...
      StaticJsonBuffer<500> jsonBuffer;
      JsonObject& json = jsonBuffer.createObject();
      json["id"] = clientId;
      json["up"] = millis() / 1000;
      json["hwName"] = "xns.test.gsm";
      json["hwRevision"] = "A";
      json["fwRevision"] = "1.0.0.1";
      json["message"] = "Connected!";
      
      String msg;
      json.printTo(msg);
    
      mqtt.publish(publishTopic, msg.c_str());
      yield();
    } else 
    {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

