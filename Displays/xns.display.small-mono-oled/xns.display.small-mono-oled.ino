/*
 Name:		OledTemp.ino
 Created:	12/25/2016 1:26:20 PM
 Author:	roarf
*/

#include "xnsense.h"
#include <Wire.h>
#include <gfxfont.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <xnsclient.h>

#define HW_NAME		"xns.display.small-mono-oled"
#define HW_REV		"A"
#define WLAN_SSID       "Roar_Etne"
#define WLAN_PASS       "monoLi10"
#define AIO_SERVER      "192.168.10.203"
#define AIO_SERVERPORT  1883
const int version[] = { 1, 0, 0, 9 };

// OLED setup
Adafruit_SSD1306 display;
#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif
unsigned long lastMessageReceived = 0;
String lastMessage("");
unsigned long updateDisplayInterval = 1000;
unsigned long lastUpdateDisplay = 0;

// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(115200);
	while (!Serial);
	
	display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
	display.clearDisplay();
	display.drawXBitmap(4, 25, xnsense::bitmap, 120, 17, WHITE);
	display.display();
	delay(2000);

	xnsClient.begin(HW_NAME, HW_REV, version, WLAN_SSID, WLAN_PASS, AIO_SERVER, AIO_SERVERPORT, Serial);
	xnsClient.setAliveMessageInterval(60);
	xnsClient.setMqttMessageCallback(mqttMessageReceived);

}


// the loop function runs over and over again until power down or reset
void loop() {
	xnsClient.loop();

	if (millis() - lastUpdateDisplay > updateDisplayInterval)
	{
		lastUpdateDisplay = millis();
		UpdateDisplay();
	}
}

void mqttMessageReceived(JsonObject& json)
{
	if (getJsonValue(json, "command").equals("Display"))
	{
		String message = getJsonValue(json, "message");
		DisplayText(message);
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


void DisplayText(const String message)
{
	lastMessage = String(message);
	lastMessageReceived = millis();
}

void UpdateDisplay()
{
	display.clearDisplay();
	display.setTextSize(2);
	display.setTextColor(WHITE);
	display.setCursor(0, 0);
	display.println(lastMessage);

	display.setTextSize(1);
	long timeSinceLast = (millis() - lastMessageReceived) / 1000;
	
	display.println("");
	display.print(timeSinceLast);
	display.println(" seconds ago...");

	display.display();
}