#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "secrets.h"
#include <WiFi.h>
#include <PubSubClient.h>

WiFiClient espClient;
PubSubClient client(espClient);

#define DELAY_AFTER_SEND 100

#define MSG_BUFFER_SIZE  (1024)
char msg[MSG_BUFFER_SIZE];
char heapMSG[MSG_BUFFER_SIZE];

const char* publish_path_rolgordijnen  = "esp32/gordijnen/voor";

const char* subsribe_path = "/set";
      char* subscribe_path_rolgordijnen;
      char* subscribe_path_rolgordijn_rechts;
      char* subscribe_path_rolgordijn_links;

const char* max_step_path = "/max_steps/set";
      char* subscribe_path_rolgordijn_rechts_max;
      char* subscribe_path_rolgordijn_links_max;

const char* speed_path = "/speed/set";
      char* subscribe_path_rolgordijn_rechts_speed;
      char* subscribe_path_rolgordijn_links_speed;

const char* accel_path = "/accel/set";
      char* subscribe_path_rolgordijn_rechts_accel;
      char* subscribe_path_rolgordijn_links_accel;

const char* stop_path = "/stop";
      char* subscribe_path_rolgordijn_stop;

char* createSubscribePath(const char* side, const char* extra_path);
void setup_mqtt();
void checkMQTTMessage();
void sendUpdateMessage();

boolean flag_update = true;
boolean flag_step_update = false;
