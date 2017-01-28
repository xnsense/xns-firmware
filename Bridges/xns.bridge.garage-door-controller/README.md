# xns.bridge.garage-door-controller

> Device to report garage door position and to control the door. The connected garage door opener has only a simple 
> input that will open, stop or close the door when activated. A rotary encoder is placed in the rotating shaft and 
> is used to determine any current movement. The device then can support commands like "open", "close" and "stop"
> and activate the door opener input enough times to perform the requested action.
> A dallas temp sensor is also added to report temperatures

## Supported commands


| Command        | Parameters                | Action                                |
| -------------- | ------------------------- | ------------------------------------- |
| open           | *none*                    | Opens the door                        |
| close          | *none*                    | Closes the door                       |
| stop           | *none*                    | Stops any current movement            |
| *Echo*         | { "message": "*text*" }   | Device replies a message with the *text* |
| *SetConfig*    | { "config": { *config* } } | Stores received *config* in EEPROM  |
| *GetConfig*    | *none*   | Device replies the current config from EEPROM |


## Reported data

```javascript
  {
    "Temperatures": {
      "inside": "12.00"
    },
    "Positions": {
      "door": "117"
    }
  }
```
