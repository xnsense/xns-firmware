#include <ArduinoJson.h>
#include <xnsclient.h>
#include "PulsePort.h"
#include "MBus.h"


#define MBUS_METER_ADDRESS 0 // The address of the MBus meter

/*
* Buffers to hold pointers in global scope
*/
char BUFFER_ELECTRICAL_METER[sizeof(PulsePort)];			// Buffer for gElectricalMeter
char BUFFER_HEAT_PUMP_METER[sizeof(MBus)];				// Buffer for gHeatPumpMeter


/*
* Application Constants
*/
const unsigned long REPORT_INTERVAL = 30000;	// 60000ms == 1 minute delay between MQTT reports
const byte EEPROM_CHECK_SUM = 124;				// Used to check if config is stored. Change if structure changes
const int EEPROM_CONFIG_ADDRESS = 0;			// EEPROM address where config class is stored
const int AP_PIN = 12;							// Input pin for Access Point mode
const int GREEN_LED = 4;						// Green LED on Pin 4
const int RED_LED = 5;							// Red LED on Pin 5

/*
* Global variables
*/
PulsePort* gElectricalMeter;				// The port used to read the electrical meter
MBus* gHeatPumpMeter;						// The port used to read the heat pump meter

/*
* Application Logic
*/
float ToPercentage(int pValue, int pFull, int pMin);	// Convert a number to a percentage between a min and a max
void SetupLEDs();										// Define the LED ports as outputs
void LED(int pPin, bool pOn);							// Signal LED on or off
void FlashLED(int pPin, int pCycles);					// Flash the LED a number of times


/*
* MQTT setup and reporting
*/
void ReportPulsePortToMqtt(PulsePort* pMeter);											// Set up the JSON and report data, for PulsePort
void ReportMBusToMqtt(MBus* pMeter);													// Set up the JSON and report data, for MBus
void GetMqttTopic(char* pBuffer);														// Get the name of the MQTT topic
void GetJSON(char* pBuffer, int pBufferSize, PulsePort* pMeter);						// Produce the JSON to be reported for pulse meter
void GetJSONForImmediateValues(char* pBuffer, int pBufferSize, Telegram* pMeter);		// Produce the JSON to be reported for MBus device, immediate values
void GetJSONForTotalValues(char* pBuffer, int pBufferSize, Telegram* pMeter);			// Produce the JSON to be reported for MBus device, total values


/*
* MBus methods
*/
void MBus_TelegramCallback(Telegram &pTelegram) {
	Serial1.println("MBus_TelegramCallback: Received telegram");
	ReportMBusToMqtt(&pTelegram);
}
void MBus_ErrorCallback(const char* pMessage) {
	Serial1.print("MBus_ErrorCallback: ");
	Serial1.println(pMessage);
}

#define HV_NAME "xns.bridge.zenner-energy-logger"
#define HV_REV "B"
const int version[] { 2, 0, 0, 0 };

void setup()
{
	SetupLEDs();

	// Initialize second H/W serial port for debug communication. This one is fixed on IO Pin 2 for ESP8266
	Serial1.begin(115200);
	while (!Serial1) {}
	Serial1.setDebugOutput(true);
	Serial1.println("Serial1");
	Serial1.println("Serial port initialized");

	xnsClient.begin(HV_NAME, HV_REV, version, AP_PIN, Serial1);

	

	//noInterrupts();
	FlashLED(GREEN_LED, 3);
	FlashLED(RED_LED, 3);

	// Initialize H/W serial port for MBus communication
	Serial.begin(2400, SERIAL_8E1);
	while (!Serial) {}
	Serial1.println("MBUS serial setup complete");

	LED(GREEN_LED, false);
	LED(RED_LED, true);

	// let's setup stuff and get on going
	Serial1.println(F("Starting Setup..."));
	FlashLED(RED_LED, 1);
	LED(RED_LED, true);
	SetupPorts();
	FlashLED(RED_LED, 3);
	FlashLED(GREEN_LED, 3);
	Serial1.println(F("Setup completed!"));
	delay(1000);
	LED(GREEN_LED, true);
}

void loop() {
	LED(GREEN_LED, true);
	xnsClient.loop();
	return;

	// Report data to MQTT
	ReportPulsePortToMqtt(gElectricalMeter);
	ReportMBusToMqtt(gHeatPumpMeter);

	// wait a period, for the next reporting
	unsigned long vDelay = 0;
	while (vDelay < REPORT_INTERVAL)
	{
		LED(GREEN_LED, true);
		delay(300);
		LED(GREEN_LED, false);
		delay(4700);
		vDelay += 5000;

		gElectricalMeter->UpdateAverage();
	}
}
float ToPercentage(int pValue, int pFull, int pMin)
{
	int vActualValue = pValue - pMin;
	return (float)vActualValue / (float)(pFull - pMin) * 100.0;
}


void GetJSONForImmediateValues(char* pBuffer, int pBufferSize, Telegram* pMeter)
{
	StaticJsonBuffer<JSON_OBJECT_SIZE(12)> vJsonBuffer;
	JsonObject& vRoot = vJsonBuffer.createObject();
	vRoot["s"] = "HP";

	UserData* vData = pMeter->Data;

	while (vData != NULL)
	{
		if (vData->Storage == 0)
		{
			switch (vData->Type)
			{
			case UserDataType_FlowTemperature:
				vRoot["t1"] = vData->Value;
				break;
			case UserDataType_ReturnTemperature:
				vRoot["t2"] = vData->Value;
				break;
			case UserDataType_TemperatureDifference:
				vRoot["td"] = vData->Value;
				break;
			case UserDataType_Power1:
				vRoot["pwr"] = vData->Value;
				break;
			case UserDataType_VolumeFlow:
				vRoot["flw"] = vData->Value;
				break;
			case UserDataType_DateAndTime:
				vRoot["hpt"] = vData->Value;
				break;
			}
		}
		vData = vData->Next;
	}

	vRoot.printTo(pBuffer, pBufferSize);
}

void GetJSONForTotalValues(char* pBuffer, int pBufferSize, Telegram* pMeter)
{
	StaticJsonBuffer<JSON_OBJECT_SIZE(12)> vJsonBuffer;
	JsonObject& vRoot = vJsonBuffer.createObject();
	vRoot["s"] = "HP";
	//	vRoot["b"] = ToPercentage(analogRead(A0), 1024, 512);

	UserData* vData = pMeter->Data;

	while (vData != NULL)
	{
		if (vData->Storage == 0)
		{
			switch (vData->Type)
			{
			case UserDataType_Energy1: // Main Storage for total production
				vRoot["en"] = vData->Value;
				break;
			case UserDataType_Volume:
				vRoot["vol"] = vData->Value;
				break;
			case UserDataType_DateAndTime:
				vRoot["hpt"] = vData->Value;
				break;
			case UserDataType_OnTime:
				vRoot["on"] = vData->Value;
				break;
			}
		}
		vData = vData->Next;
	}

	vRoot.printTo(pBuffer, pBufferSize);
}

void ReportMBusToMqtt(MBus* pMeter)
{
	gHeatPumpMeter->ReadDevice(1);
}
void ReportMBusToMqtt(Telegram* pMeter)
{
	if (pMeter->Data == NULL)
	{
		Serial1.println("Telegram contains no data...");
		return;
	}

	char vMessage[250];
	GetJSONForImmediateValues(vMessage, sizeof(vMessage), pMeter);
	//ReportToMqtt(vMessage);

	GetJSONForTotalValues(vMessage, sizeof(vMessage), pMeter);
	//ReportToMqtt(vMessage);
}

void ReportPulsePortToMqtt(PulsePort* pMeter)
{
	pMeter->UpdateAverage();

	StaticJsonBuffer<JSON_OBJECT_SIZE(8)> vJsonBuffer;
	JsonObject& vRoot = vJsonBuffer.createObject();
	vRoot["s"] = pMeter->Name;
	vRoot["pwr"] = pMeter->TicksInWattHours();
	vRoot["tot"] = pMeter->TotalTicksInWattHours() + pMeter->TicksInWattHours();
	vRoot["avg"] = pMeter->GetAverage();
	vRoot["cur"] = pMeter->Current();
	vRoot["b"] = ToPercentage(analogRead(A0), 1024, 512);
	xnsClient.sendMqttData(vRoot);

	// Increase total values and reset counter
	pMeter->CommitTicksToTotal();

	// Store the values to EEPROM
	pMeter->SaveTotalValue();
}

void LED(int pPin, bool pOn)
{
	digitalWrite(pPin, (byte)pOn);
}
void FlashLED(int pPin, int pCycles)
{
	for (int i = 0; i < pCycles; i++)
	{
		LED(pPin, true);
		delay(150);
		LED(pPin, false);
		delay(150);
	}
}
void SetupLEDs()
{
	pinMode(GREEN_LED, OUTPUT);
	FlashLED(GREEN_LED, 2);

	pinMode(RED_LED, OUTPUT);
	FlashLED(RED_LED, 2);
}

void SetupPorts()
{
	Serial1.println(F("Pin 13 = Electrical meter reading (90ms pulse)"));

	gElectricalMeter = new (BUFFER_ELECTRICAL_METER)PulsePort("EL", 13, 1000, 90000, 60, 0);
	gElectricalMeter->ReadTotalValue();
	gElectricalMeter->Begin();

	gHeatPumpMeter = new (BUFFER_HEAT_PUMP_METER)MBus(MBUS_METER_ADDRESS, MBus_TelegramCallback, MBus_ErrorCallback);
	gHeatPumpMeter->Debug = true;
}

