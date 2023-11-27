#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "motor.h"

class MqttPath {
  public:
    MqttPath(char* path, Motor* motor_obj, void (Motor::*callback)(const JsonDocument& local_doc));
    void compare(char* path, const JsonDocument& local_doc);
    void add_mqtt_path(char* path, Motor* motor_obj, void (Motor::*callback)(const JsonDocument& local_doc));
    void subscribe();
  private:
    char* _path;
    void (Motor::*_callback)(const JsonDocument& local_doc);
    MqttPath* _next;
    Motor* _motor_obj;
};

MqttPath *first_mqtt = NULL;
