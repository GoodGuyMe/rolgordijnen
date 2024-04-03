#include "mqtt_handler.h"

MqttPath::MqttPath(char* path, Motor* motor_obj, void (Motor::*callback)(const JsonDocument& local_doc)) {
  _path = (char *)malloc(strlen(path) + 1);
  strcpy(_path, path);
  _callback = callback;
  _next = NULL;
  _motor_obj = motor_obj;
}

void MqttPath::compare(char* path, const JsonDocument& local_doc) {
  if (strcmp(_path, path) == 0) {
    (_motor_obj ->* ((MqttPath*)this)->MqttPath::_callback) (local_doc);
  }
  if (_next == NULL) {
    return;
  }
  _next->compare(path, local_doc);
}

void MqttPath::add_mqtt_path(char* path, Motor* motor_obj, void (Motor::*callback)(const JsonDocument& local_doc)) {
  if (_next == NULL) {
    MqttPath *next = new MqttPath(path, motor_obj, callback);
    _next = next;
  }
  else {
    _next->add_mqtt_path(path, motor_obj, callback);
  }
}

void MqttPath::subscribe() {
  client.subscribe(_path);
  Serial.print("Subscribed to:");
  Serial.println(_path);
  if (_next == NULL) {
    return;
  }
  _next->subscribe();
}
