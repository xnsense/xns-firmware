/*
 Name:		GarageDoor.ino
 Created:	8/8/2016 8:21:14 PM
 Author:	roarf
*/


#include <ArduinoJson.h>
#include <xnsclient.h>
#include <DallasTemperature.h>
#include <OneWire.h>


#define HW_NAME		"xns.bridge.garage-door-controller"
#define HW_REV		"A"
const int version[] = { 1, 0, 1, 0 };


#define PIN_ENCODER_DATA 5	// Note: Marked as 4 on the board
#define PIN_ENCODER_CLK 4	// Note: Marked as 5 on the board
#define PIN_SWITCH 12
#define PIN_DOOR_RELAY 14

// Garage port position
long lastPosition = 0;			// The last position from when we last reported
unsigned long lastReport = 0;	// The last time we reported a position
const int minReportingInterval = 15;
volatile long position = 0;		// The current position, modified by interrupt routines
volatile bool clkSet = false;


// Dallas Temperature (18B20) sensor
#define TEMP_PIN 13
OneWire oneWire(TEMP_PIN);
DallasTemperature tempSensor(&oneWire);
float lastTemp = 0;
unsigned long lastMillis = 0;


// the setup function runs once when you press reset or power the board
void setup() {

	Serial.begin(115200);
	while (!Serial);

	xnsClient.begin(HW_NAME, HW_REV, version, -1, Serial);
	xnsClient.setAliveMessageInterval(60); // Alive message every 1 hour
	xnsClient.setMqttMessageCallback(mqttMessageReceived);

	pinMode(PIN_ENCODER_CLK, INPUT);
	pinMode(PIN_ENCODER_DATA, INPUT);

	pinMode(PIN_SWITCH, INPUT);
	pinMode(PIN_DOOR_RELAY, OUTPUT);

	attachInterrupt(PIN_ENCODER_DATA, encoder_Data, CHANGE);
	/*
	attachInterrupt(PIN_ENCODER_DATA, encoder_Data_Falling, FALLING);
	attachInterrupt(PIN_ENCODER_CLK, encoder_Clk_Falling, FALLING);
	*/
	Serial.println("Setup complete");
}


void loop() {
	xnsClient.loop();

	if (!digitalRead(PIN_SWITCH))
		position = 0;

	long pos = position;

	tempSensor.requestTemperatures();
	float temp = tempSensor.getTempCByIndex(0);


	// only publish a message if position changed and at max every 10 second.
	if ((abs(lastTemp - temp) >= 0.4 || lastPosition != pos) && (millis() - lastReport > (minReportingInterval * 1000)))
	{
		if (lastPosition != pos)
		{
			Serial.print("Position changed to: ");
			Serial.println(pos);
		}
		if (abs(lastTemp - temp) >= 0.4)
		{
			Serial.print("Temperature changed to: ");
			Serial.println(temp);
		}

		lastPosition = pos;
		lastTemp = temp;


		StaticJsonBuffer<500> jsonBuffer;
		JsonObject& json = jsonBuffer.createObject();
		JsonObject& temperatures = json.createNestedObject("Temperatures");
		JsonObject& positions = json.createNestedObject("Positions");
		if (temp != -127)
			temperatures["inside"] = String(temp);
		positions["door"] = String(pos);
		
		xnsClient.sendMqttData(json);
		lastReport = millis();
	}
}


void mqttMessageReceived(JsonObject& json)
{
	String vJson("");
	json.prettyPrintTo(vJson);

	if (xnsClient.getJsonValue(json, "command").equals("open"))
	{
		String vMessage = "Opening door. Position is now: " + String(position);
		xnsClient.sendMqttMessage(vMessage);
		Serial.println("Received [open] command on MQTT:");
		Serial.println(vJson);
		long vPosition = position;
		delay(1000);
		if (vPosition < position) // we're currently moving downwards
		{
			vMessage = "Door was moving downwards, positions changed to " + String(position);
			xnsClient.sendMqttMessage(vMessage);

			pushDoorSwitch();
			delay(2000);
			pushDoorSwitch();
		}
		else if (vPosition > position) // we're currently moving upwards
		{
			vMessage = "Already moving upwards, positions changed to " + String(position);
			xnsClient.sendMqttMessage(vMessage);
		}
		else // we're standing still
		{
			pushDoorSwitch();
			delay(2000);

			if (vPosition < position) // door moved, but in the wrong direction
			{
				vMessage = "Wrong direction, positions changed to " + String(position);
				xnsClient.sendMqttMessage(vMessage);
				pushDoorSwitch();
				delay(2000);
				pushDoorSwitch();
			}
		}
	}
	else if (xnsClient.getJsonValue(json, "command").equals("close"))
	{
		String vMessage = "Closing door. Position is now: " + String(position);
		xnsClient.sendMqttMessage(vMessage);
		Serial.println("Received [close] command on MQTT:");
		Serial.println(vJson);
		long vPosition = position;
		delay(1000);
		if (vPosition > position) // we're currently moving upwards
		{
			vMessage = "Door was moving upwards, positions changed to " + String(position);
			xnsClient.sendMqttMessage(vMessage);
			
			pushDoorSwitch();
			delay(2000);
			pushDoorSwitch();
		}
		else if (vPosition < position) // we're currently moving upwards
		{
			vMessage = "Already moving downwards, positions changed to " + String(position);
			xnsClient.sendMqttMessage(vMessage);
		}
		else // we're standing still
		{
			pushDoorSwitch();
			delay(2000);
			if (vPosition > position) // door moved, but in the wrong direction
			{
				vMessage = "Wrong direction, positions changed to " + String(position);
				xnsClient.sendMqttMessage(vMessage);
				pushDoorSwitch();
				delay(2000);
				pushDoorSwitch();
			}
		}
	}
	else if (xnsClient.getJsonValue(json, "command").equals("stop"))
	{
		String vMessage = "Stopping door. Position is now: " + String(position);
		xnsClient.sendMqttMessage(vMessage);
		Serial.println("Received [stop] command on MQTT:");
		Serial.println(vJson);
		long vPosition = position;
		//pushDoorSwitch();
		delay(1000);
		if (vPosition != position) // door moved, any direction
		{
			vMessage = "Door is moving, positions changed to " + String(position);
			xnsClient.sendMqttMessage(vMessage);
			pushDoorSwitch();
		}
		else
		{
			vMessage = "Door is not moving, positions is still " + String(position);
			xnsClient.sendMqttMessage(vMessage);
		}
	}
}

void pushDoorSwitch()
{
	digitalWrite(PIN_DOOR_RELAY, HIGH);
	delay(500);
	digitalWrite(PIN_DOOR_RELAY, LOW);
}

void encoder_Data()
{
	bool data = digitalRead(PIN_ENCODER_DATA) == HIGH;
	bool newClkSet = digitalRead(PIN_ENCODER_CLK) == HIGH;

	if (!data)
	{
		clkSet = newClkSet;
	}
	else if (clkSet != newClkSet)
	{
		position += newClkSet ? 1 : -1;
	}
}

