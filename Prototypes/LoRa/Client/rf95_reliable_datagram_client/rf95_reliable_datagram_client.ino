
#include <Adafruit_SleepyDog.h>
#include <RHReliableDatagram.h>
#include <RH_RF95.h>
#include <SPI.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <ArduinoJson.h>

#define ONE_WIRE_BUS 5
#define TEMPERATURE_PRECISION 9
#define CLIENT_ADDRESS 2
#define SERVER_ADDRESS 2
#define ENCRYPTKEY    "" //exactly the same 16 characters/bytes on all nodes!

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
// arrays to hold device addresses
DeviceAddress thermometer1, thermometer2;

float lastTemp = 0;
float lastBatt = 0;
unsigned long lastMillis = 0;
int sensorReadings = 0;

// RF communication, Dont put this on the stack:
byte buf[RH_RF95_MAX_MESSAGE_LEN];

RH_RF95 driver(8, 7); // Rocket Scream Mini Ultra Pro with the RFM95W

//CS 8
//RST 4
// INT 7

RHReliableDatagram manager(driver, CLIENT_ADDRESS);
void setup() 
{
  Serial.begin(9600);
  //while (!Serial) ; // Wait for serial port to be available
  if (!manager.init())
    Serial.println("init failed");
  Serial.println("init sucess");
  if (!driver.setFrequency(868)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(868);
  driver.setTxPower(23, false);

  //Set Encryption
  //driver.encrypt(ENCRYPTKEY);
  
  float measuredvbat = analogRead(A9);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage
  Serial.print("VBat: " ); Serial.println(measuredvbat);

  // Start up the library
  sensors.begin();
  // locate devices on the bus
  Serial.print("Locating devices...");
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");
  
  // report parasite power requirements
  Serial.print("Parasite power is: "); 
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");
  
  if (!sensors.getAddress(thermometer1, 0)) Serial.println("Unable to find address for Device 0"); 
  if (!sensors.getAddress(thermometer2, 1)) Serial.println("Unable to find address for Device 1"); 

  // show the addresses we found on the bus
  Serial.print("Device 0 Address: ");
  printAddress(thermometer1);
  Serial.println();

  Serial.print("Device 1 Address: ");
  printAddress(thermometer2);
  Serial.println();

  // set the resolution to 9 bit
  sensors.setResolution(thermometer1, TEMPERATURE_PRECISION);
  sensors.setResolution(thermometer2, TEMPERATURE_PRECISION);

  Serial.print("Device 0 Resolution: ");
  Serial.print(sensors.getResolution(thermometer1), DEC); 
  Serial.println();

  Serial.print("Device 1 Resolution: ");
  Serial.print(sensors.getResolution(thermometer2), DEC); 
  Serial.println();
  
}


void loop()
{

// ------ Code to get device to sleep ---START--- //

//  int time_Between_Readings_in_ms = 1000;
//  // Since the watchdog timer maxes out at 8 seconds....
//  int number_of_sleeper_loops = 8; //i.e.: time between taking a moisture reading is 4 * 8 seconds = 32 seconds.
//  for (int i = 0; i < number_of_sleeper_loops; i++) {
//    time_Between_Readings_in_ms = Watchdog.sleep(8000);
//  }
//  time_Between_Readings_in_ms = time_Between_Readings_in_ms * number_of_sleeper_loops;
// ------ Code to get device to sleep --END---- //  


  StaticJsonBuffer<500> jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  JsonObject& temperatures = json.createNestedObject("Temperatures");

  float measuredvbat = analogRead(A9);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage
  
  sensors.requestTemperatures();
  float temp1 = sensors.getTempC(thermometer1);
  float temp2 = sensors.getTempC(thermometer2);
  
  temperatures["temp1"] = String(temp1);
  temperatures["temp2"] = String(temp2);
  json["battery"] = String(measuredvbat);
  json["sensorReadings"] = String(sensorReadings);
  
  size_t jsonLength = json.printTo(buf, RH_RF95_MAX_MESSAGE_LEN);
  
  // Send a message to manager_server
  if (manager.sendtoWait(buf, jsonLength, SERVER_ADDRESS))
  {
    // Now wait for a reply from the server
    uint8_t len = sizeof(buf);
    uint8_t from;
    if (manager.recvfromAckTimeout(buf, &len, 2000, &from))
    {
        //Serial.print("Sent Sucessfully");
    }
    else
    {
      //Serial.println("No reply, is rf95_reliable_datagram_server running?");
    }
    
    //Count Sensor Reading +1
    sensorReadings += 1;

    //Put Radio to sleep
    if(driver.sleep())
    {
      //Serial.print("Radio set to sleep");
    }
  }
  else
  {
    //Serial.println("sendtoWait failed");
  }      
  
  delay(15000);
}
// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}
