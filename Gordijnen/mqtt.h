#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "secrets.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include "mqtt_handler.h"

WiFiClient espClient;
PubSubClient client(espClient);

#define DELAY_AFTER_SEND 100

#define MSG_BUFFER_SIZE  (1024)
char msg[MSG_BUFFER_SIZE];
char heapMSG[MSG_BUFFER_SIZE];

const char* publish_path_rolgordijnen  = "esp32/gordijnen/voor";

void setup_mqtt();
void checkMQTTMessage();

boolean flag_update = true;
boolean flag_step_update = false;
