/*
 Name:		xns.generic.open-collector-switch.ino
 Created:	12/29/2016 11:46:45 AM
 Author:	roarf
*/


#include <ArduinoJson.h>
#include <xnsclient.h>

#define HW_NAME		"xns.generic.open-collector-switch"
#define HW_REV		"A"
const int version[] = { 1, 0, 0, 10 };


#define DOOR_PIN 5
#define AP_PIN 12

// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(115200);
	while (!Serial);

	pinMode(DOOR_PIN, OUTPUT);

	xnsClient.begin(HW_NAME, HW_REV, version, AP_PIN, Serial);
	xnsClient.setAliveMessageInterval(60);
	xnsClient.setMqttMessageCallback(mqttMessageReceived);
}

// the loop function runs over and over again until power down or reset
void loop() {
	xnsClient.loop();
}

void mqttMessageReceived(JsonObject& json)
{
	if (xnsClient.getJsonValue(json, "command").equals("Open"))
	{
		Serial.println("Pushing the button...");
		xnsClient.sendMqttMessage("Pushing the button...");
		digitalWrite(DOOR_PIN, 1);
		delay(2000);
		digitalWrite(DOOR_PIN, 0);
	}
}