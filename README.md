# Roller blinds
Self made automatic roller blinds to control two TMC2209 motor drivers.


## Dependencies
The project has the following dependencies:
1. AccelStepper
2. PubSubClient
3. ArduinoJson

Accelstepper is used for controller the motor drivers, pubsubclient is used for MQTT connection.
ArduinoJson is a light weight json converter. The project is running on an ESP32, the wifi library is used from there.

## MQTT
Supports updates over mqtt to:
- Speed
- Acceleration
- Amount of steps
- Current position