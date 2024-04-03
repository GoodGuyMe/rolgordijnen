# Roller blinds
DIY automatic roller blinds, controlled by two TMC2209 stepper motor drivers. Modified a gear in the roller blind to allow for a timing belt to drive the mechanism.

## Code
The code connects to a MQTT server, for the 

## Dependencies
The project has the following dependencies:
1. AccelStepper
2. PubSubClient
3. ArduinoJson

Accelstepper is used for controller the motor drivers, pubsubclient is used for MQTT connection.
ArduinoJson is a light weight json converter. The project is running on an ESP32, it connects to the MQTT server via WiFi.

## MQTT
Supports updates over mqtt to:
- Speed
- Acceleration
- Amount of steps
- Current position
- Stop immediately
- And goto position ofcourse

## Setup
To setup the project, create a secrets.h file containing the folowing items:
```
const char* ssid = "<ssid>"; // Enter your WiFi name
const char* password =  "<pwd>"; // Enter WiFi password
const char* mqtt_server = "<ip>"; // Mqtt server IP
#define mqtt_port <port>
#define MQTT_USER "<username>"
#define MQTT_PASSWORD "<pwd>"
```

Make sure in the Arduino IDE all the dependencies are installed.

The code is able to handle multiple motors at the same time, 
a motor is added by the following snippet:
```
char left_path[96];
strcpy(left_path, publish_path_rolgordijnen);
strcat(left_path, "/links");
Motor* rol_left = new Motor(left_path, MOTOR_1_STEP, MOTOR_1_DIR, MOTOR_1_ENABLE, "links", false);
motors.addMotor(rol_left);
```
The pin number `MOTOR_x_STEP` is defined in the [motor.h](Gordijnen/motor.h) file. 
All three possible motor connections for the pcb in as found in [the pcb file](pcbs/esp32.zip) are defined.

### Running the code
It is important that in the setup the function ```setup_mqtt()``` is called to register to all the mqtt topics.
Then as often as possible call ```motors.run()```, such that the stepper motors can run as smoothly as possible.

### Hardware setup
To learn how to set up the hardware, take a look at the README.md file in [the stl_files](stl_files/README.md)(COMMING SOON) directory.