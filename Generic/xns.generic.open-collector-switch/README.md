# xns.generic.open-collector-switch

> A simple generic device to use as a switch. One singe output set up as open collector through an N-FET 
> will be activated for 2 seconds when a message is received


This device is based on the [xns-client](https://github.com/xnsense/xns-client) arduino library, which takes care of
* SSID / Password storage in EEPROM (boot as AP to set config)
* MQTT communication using json
* OTA updates by passing a URL in mqtt message
* Fitting the device into the [XNSENSE IoT ecosystem](http://www.xnsense.no/)

## Supported commands


| Command        | Parameters                | Action                                |
| -------------- | ------------------------- | ------------------------------------- |
| click          | *none*                    | Activates the output for 2 seconds    |
| *Echo*         | { "message": "*text*" }   | Device replies a message with the *text* |
| *SetConfig*    | { "config": { *config* } } | Stores received *config* in EEPROM  |
| *GetConfig*    | *none*   | Device replies the current config from EEPROM |
