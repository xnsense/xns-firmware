

// Select your modem:
#define TINY_GSM_MODEM_SIM800
//#define TINY_GSM_MODEM_SIM900
//#define TINY_GSM_MODEM_A7
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
SoftwareSerial SerialAT(10, 2); // RX, TX

TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient mqtt(client);

const char * clientId = "11:11:11:11:11:11";
const char * username = "";
const char * password = "";

const char* publishTopic = "sensors/out/11:11:11:11:11:11";
const char* subscribeTopic = "sensors/in/11:11:11:11:11:11";
unsigned long lastkeepAliveTime = 0;

long lastReconnectAttempt = 0;

void setup() {

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
  Serial.print("Signal Quality: "); Serial.println(modem.getSignalQuality());
  // MQTT Broker setup
  mqtt.setServer(mqtt_server, mqtt_port);
  mqtt.setCallback(mqttCallback);
  
  
}

void mqttCallback(char* topic, byte* payload, unsigned int len) 
{
  Serial.println("Incomming Message");
  char message[1024];
  for (int i = 0; i < len; i++)
    message[i] = payload[i];
  message[len] = 0;

  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(message);
  if (getJsonValue(json, "command").equals("Echo"))
    {
      String vMessage = String("Echo: ") + getJsonValue(json, "message");
      sendMqttMessage(vMessage);
    }
}

String getJsonValue(JsonObject& json, String key)
{
  for (JsonObject::iterator it = json.begin(); it != json.end(); ++it) {
    if (key.equals(it->key))
      return String(it->value.asString());
  }
  return String("");
}

void loop() 
{
  
  //modem.getSignalQuality() //returns int
//  switch(modem.getRegistrationStatus())
//  {
//    case 1:
//    case 5:
//      
//      }      
//    break;
//    default: 
//    Serial.println("Lost Connection");
//    Serial.print("Signal Quality: "); Serial.println(modem.getSignalQuality());
//    break;    
//  }

  mqtt.loop();
  delay(10);
  if (!mqtt.connected()) 
  {
  yield();
  reconnect();
  }
  yield();
  //checkTempAndSend();
  yield();
  if(millis() - lastkeepAliveTime > 900000){
    lastkeepAliveTime = millis();
    Serial.println("Send MQTT Alive");
    sendMqttMessage("I'm alive");
  }

  delay(1000);
}

void sendMqttMessage(String messageSend)
{
    StaticJsonBuffer<500> jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["id"] = clientId;
    json["up"] = millis() / 1000;
    json["hwName"] = "xns.test.gsm";
    json["hwRevision"] = "A";
    json["fwRevision"] = "1.0.0.1";
    json["message"] = messageSend;
    String msg;
    json.printTo(msg);
    mqtt.publish(publishTopic, msg.c_str());  
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
      mqtt.subscribe(subscribeTopic);
      sendMqttMessage("Connected");
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

