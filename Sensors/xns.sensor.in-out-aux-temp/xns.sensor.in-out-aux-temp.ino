/*
MIT License

Copyright (c) 2017 xnsense as

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

 Name:		xns.sensor.in-out-aux-temp.ino
 Created:	10/26/2016 8:22:46 PM
 Author:	roarf
*/

#include <SPI.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include "xnsclient.h"

<<<<<<< HEAD
#define hw_name		"xns.sensor.in-out-aux-temp"
#define hw_revision    "A"
const int fw_revision[] = { 1, 0, 0, 1 };
=======
#define HW_NAME		"xns.sensors.tripletemp"
#define HW_REV		"A"
const int version[] = { 1, 0, 0, 20 };
>>>>>>> origin/master

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

float lastTemp1 = 0;
float lastTemp2 = 0;
float lastTemp3 = 0;

unsigned long lastMillis = 0;

// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(115200);
	while (!Serial);

<<<<<<< HEAD
	xnsClient.begin(hw_name, hw_revision,fw_revision, -1, Serial);
=======
	xnsClient.begin(HW_NAME, HW_REV, version, -1, Serial);
>>>>>>> origin/master
	xnsClient.setAliveMessageInterval(60);
	xnsClient.setMqttMessageCallback(mqttMessageReceived);
}

// the loop function runs over and over again until power down or reset
void loop() {
	xnsClient.loop();

	tempSensor1.requestTemperatures();
	tempSensor2.requestTemperatures();
	tempSensor3.requestTemperatures();

	float temp1 = tempSensor1.getTempCByIndex(0);
	float temp2 = tempSensor2.getTempCByIndex(0);
	float temp3 = tempSensor3.getTempCByIndex(0);

	// only publish a message if temp changed and at max every 60 second.
	if ((
		abs(lastTemp1 - temp1) > 0.25 ||
		abs(lastTemp2 - temp2) > 0.25 ||
		abs(lastTemp3 - temp3) > 0.25		
		) && millis() - lastMillis > 30000)
	{
		Serial.print("Temperatures changed to: ");
		Serial.print(temp1);
		Serial.print(" C / ");
		Serial.print(temp2);
		Serial.print(" C / ");
		Serial.print(temp3);
		Serial.print(" C\r\n");

		lastMillis = millis();

		StaticJsonBuffer<500> jsonBuffer;
		JsonObject& json = jsonBuffer.createObject();
		JsonObject& temperatures = json.createNestedObject("Temperatures");
		if (temp1 != -127)
			temperatures["in"] = String(temp1);
		if (temp2 != -127)
			temperatures["out"] = String(temp2);
		if (temp3 != -127)
			temperatures["aux"] = String(temp3);

		xnsClient.sendMqttData(json);
		
		lastTemp1 = temp1;
		lastTemp2 = temp2;
		lastTemp3 = temp3;
	}
}


void mqttMessageReceived(JsonObject& json)
{
	String vJson("");
	json.prettyPrintTo(vJson);
	Serial.println("Received JSON on MQTT:");
	Serial.println(vJson);
}



