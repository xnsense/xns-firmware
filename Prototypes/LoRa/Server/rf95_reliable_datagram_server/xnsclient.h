// xinit.h

#ifndef _xnsClientCLASS_h
#define _xnsClientCLASS_h


#ifndef WIFI_CONNECTION_TIMEOUT
#define WIFI_CONNECTION_TIMEOUT 30000
#endif // !WIFI_CONNECTION_TIMEOUT

#include <arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>


class xnsClientClass
{
private:
	bool secure = false;
	bool debugMode = false;
	Stream* debugger = NULL;
	//Name of Hardware Eq. xns.rfid
	const char* hwName;
	//Revision number Eq. A
	const char* hwRevision;
	//Revision number Eq. 1.0.0.0
	const int* fwRevision;
	const char* ssid;
	const char* ssidPassword;
	const char* mqttName;
	int mqttPort;
	const char* mqttUser = 0;
	const char* mqttPass = 0;

	const char* mqttClientName = 0;
	const char* mqttClientID = 0;
	const char* mqttPublishTopic = 0;
	const char* mqttSubscribeTopic = 0;
	unsigned long aliveMessageInterval = 0;
	unsigned long lastAliveMessage = 0;

	int accessPointButtonPin = -1;

	WiFiClient *client;
	byte mac[6];                     // the MAC address of your Wifi shield
	PubSubClient mqtt;

	void MQTT_connect();
	void UpgradeFirmware(String pUrl);

	void(*mqttMessageCallback)(JsonObject& json) = NULL;

protected:

public:
	xnsClientClass();
	void begin(const char* hwName,const char* hwRevision, const int fwRevision[], const char* ssid, const char* ssidPassword, const char* mqtt, int mqttPort);
	void begin(const char* hwName,const char* hwRevision, const int fwRevision[], const char* ssid, const char* ssidPassword, const char* mqtt, int mqttPort, Stream& debugger);
	void begin(const char* hwName,const char* hwRevision, const int fwRevision[], const char* ssid, const char* ssidPassword, const char* mqtt, int mqttPort, const char* mqttUser, const char* mqttPass);
	void begin(const char* hwName,const char* hwRevision, const int fwRevision[], const char* ssid, const char* ssidPassword, const char* mqtt, int mqttPort, const char* mqttUser, const char* mqttPass, Stream& debugger);
	void beginSecure(const char* hwName,const char* hwRevision, const int fwRevision[], const char* ssid, const char* ssidPassword, const char* mqtt, int mqttPort, const char* mqttClientID, const char* mqttUser, const char* mqttPass, const char* mqttPublishTopic, const char* mqttSubscribeTopic, Stream& debugger, bool debugMode);
	void loop();
	bool connected();
	void printFlashInfo();
	void printDeviceInfo();
	String GetVersionString();

	void printIndexHeapSize();
	uint32_t getIndexHeapSize();
	void sendMqttConnectMessage();
	void sendMqttMessage(String message);
	void sendMqttMessage(JsonObject& message);
	void sendMqttData(String message);
	void sendMqttData(JsonObject& message);

	void setMqttMessageCallback(void(*mqttMessageCallback)(JsonObject& json));
	bool hasValue(JsonObject& json, String key);
	String getJsonValue(JsonObject& json, String key);
	JsonVariant getJsonObject(JsonObject& json, String key);
	void mqttMessageReceived(char* topic, unsigned char* payload, unsigned int length);
	void setAliveMessageInterval(int interval);
};

extern xnsClientClass xnsClient;


#endif
