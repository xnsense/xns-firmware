# xns-firmware

> A collection of ESP8266 firmware built on top of xns-client. All devices will support OTA updates and will communicate through an MQTT server. When connected to the xns IoT Hub (either directly, or via a local MQTT), the device will be managed in the xns-backend using the xns-api and/or the xns-admin user interface

Category | Description | Example
--------- | ---------- | ----------
Sensor    | Independent devices with integrated sensors. Would typically not have any UI | xns.sensor.in-out-aux-temp
Display   | Devices built solely for displaying a visual element or an audible sound | xns.display.small-oled-mono
Bridge    | Special purpose devices to internet enable any existing hardware. Would typically interface equipment using a serial protocol or simple I/O  | xns.bridge.zenner-heatmeter
Generic | General purpose devices that have multiple use cases. Can typically be a device to get a users input (like a button or a keypad) or output to electrical appliances, like a relay or a dimmer | xns.generic.open-collector-switch
