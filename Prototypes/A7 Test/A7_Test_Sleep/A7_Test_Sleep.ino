

#define TINY_GSM_MODEM_A7
#define mqtt_server "79.161.196.15"
#define mqtt_port  1883

#define TINY_GSM_DEBUG Serial
#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>

// Your GPRS credentials
// Leave empty, if missing user or pass
const char apn[]  = "telenor.smart";
const char user[] = "";
const char pass[] = "";

// Dallas Temperature (18B20) sensor
#define TEMP_PIN1 12 //Inside the box
#define TEMP_PIN2 4 //Outside the box
#define TEMP_PIN3 5 //Extra, AUX

OneWire oneWire1(TEMP_PIN1);
DallasTemperature tempSensor1(&oneWire1);
OneWire oneWire2(TEMP_PIN2);
DallasTemperature tempSensor2(&oneWire2);
OneWire oneWire3(TEMP_PIN3);
DallasTemperature tempSensor3(&oneWire3);

// or Software Serial on Uno, Nano
#include <SoftwareSerial.h>
SoftwareSerial SerialAT(13, 14); // RX, TX

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

  Serial.println("Starting");
  // Set console baud rate
  Serial.begin(115200);
  // Set GSM module baud rate
  SerialAT.begin(115200);
  delay(3000);

  float lastTemp1 = 0;
  float lastTemp2 = 0;
  float lastTemp3 = 0;

  //Open eeprom and get value on adr 1,2,3
  EEPROM.begin(512);
  lastTemp1 = EEPROM.read(0);
  lastTemp2 = EEPROM.read(1);
  lastTemp3 = EEPROM.read(2);
  Serial.print("Temp1 in EEPROM: ");Serial.println(lastTemp1);
  Serial.print("Temp2 in EEPROM: ");Serial.println(lastTemp3);
  Serial.print("Temp3 in EEPROM: ");Serial.println(lastTemp3);
  //Get Temp from Temp Sensor 1,2,3
  tempSensor1.requestTemperatures();
  tempSensor2.requestTemperatures();
  tempSensor3.requestTemperatures();

  float temp1 = tempSensor1.getTempCByIndex(0);
  float temp2 = tempSensor2.getTempCByIndex(0);
  float temp3 = tempSensor3.getTempCByIndex(0);
  if(temp1 == -127){temp1 = 0;};
  if(temp2 == -127){temp2 = 0;};
  if(temp3 == -127){temp3 = 0;};
  Serial.print("Temp1 in Sensor: ");Serial.println(temp1);
  Serial.print("Temp2 in Sensor: ");Serial.println(temp2);
  Serial.print("Temp3 in Sensor: ");Serial.println(temp3);
  //Check if temp1,2,3 is different then from EEPROM and is more then 0.25 in different, if so we need to send the value to backend
  if ( abs(lastTemp1 - temp1) > 0.50 || abs(lastTemp2 - temp2) > 0.50 || abs(lastTemp3 - temp3) > 0.50 )
  {
      EEPROM.write(0, temp1);
      EEPROM.write(1, temp2);
      EEPROM.write(2, temp3);
      EEPROM.commit();
      EEPROM.end();
    
    // Restart takes quite some time
      // To skip it, call init() instead of restart()
      Serial.println("Initializing modem...");
    
      //Power modem
      pinMode(2, OUTPUT);
      digitalWrite(2, HIGH);
      delay(2000);
      digitalWrite(2, LOW);
      
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
    
     //Start MQTT
      mqtt.loop();
      delay(10);
      if (!mqtt.connected()) 
      {
        yield();
        reconnect();
      }
      sendTemp(temp1,temp2,temp3);
  }else
  {
    //Close EEPROM
    EEPROM.end();  
  }
  
  Serial.println("Going to sleep for 10 minutes");
  //Go to sleep for 10 minutes
  ESP.deepSleep(600000000, WAKE_RF_DISABLED);
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

void sendTemp(float temp1, float temp2, float temp3)
{

    StaticJsonBuffer<500> jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    JsonObject& data = jsonBuffer.createObject();
    JsonObject& temperatures = data.createNestedObject("Temperatures");
    if (temp1 != -127)
      temperatures["in"] = String(temp1);
    if (temp2 != -127)
      temperatures["out"] = String(temp2);
    if (temp3 != -127)
      temperatures["aux"] = String(temp3);
    data["SignalQuality"] = modem.getSignalQuality();
    
    json["id"] = clientId;
    json["up"] = millis() / 1000;
    json["hwName"] = "xns.test.gsm";
    json["hwRevision"] = "A";
    json["fwRevision"] = "1.0.0.1";
    json["data"] = data;

    String msg;
    json.printTo(msg);
    mqtt.publish(publishTopic, msg.c_str());
    Serial.println(msg);
}

void reconnect() 
{
  int retries = 0;
  switch(modem.getRegistrationStatus())
  {
    case 1:
    case 5:
      // Loop until we're reconnected
      while (!mqtt.connected()) 
      {
        retries += 1;
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
          // Wait 3 seconds before retrying
          delay(3000);
          //Try to restart modem if MQTT cannot connect after 5 times
          if(retries == 5)
          {
            reConnectModem();  
            retries == 0;
          }
        }
      }      
    break;
    default:
      Serial.println("Lost Connection");
      reConnectModem();
      break;    
  }
}

void reConnectModem()
{
    yield();
    modem.restart();
    yield();
    Serial.print("Waiting for network...");
    if (!modem.waitForNetwork()) {
      Serial.println(" fail");
      while (true);
    }
    Serial.println(" OK");
    yield();
    Serial.print("Connecting to ");
    Serial.print(apn);
    yield();
    if (!modem.gprsConnect(apn, user, pass)) {
      Serial.println(" fail");
    }
    yield();
}

