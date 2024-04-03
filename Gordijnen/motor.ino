#include "motor.h"
#include "mqtt_handler.h"
#include "mqtt.h"

void motor_mqtt_helper(const char* base_path, const char* ext_path, Motor* motor_obj, void (Motor::*callback)(const JsonDocument& local_doc)) {
  char str[96];
  strcpy(str, base_path);
  strcat(str, ext_path);
  if (first_mqtt == NULL) {
    first_mqtt = new MqttPath(str, motor_obj, callback);
  } else {
    first_mqtt->add_mqtt_path(str, motor_obj, callback);
  }
}

Motor::Motor(const char* mqtt_path, const int step_pin, const int dir_pin, const int enable_pin, String id, boolean inverted) {
  _stepper = new AccelStepper(motorInterfaceType, step_pin, dir_pin);
  _enable_pin = enable_pin;
  _id = id;
  
  pinMode(enable_pin, OUTPUT);
  delay(10);
  digitalWrite(enable_pin, HIGH);

  _stepper->setPinsInverted(inverted, false, false);
  
  int str_len = 96;
  char max_str[str_len], speed_str[str_len], acc_str[str_len], pos_str[str_len];
  (id + "_max_step").toCharArray(max_str, str_len);
  (id + "_speed").toCharArray(speed_str, str_len);
  (id + "_acc").toCharArray(acc_str, str_len);
  (id + "_pos").toCharArray(pos_str, str_len);

  _max_steps = preferences.getInt(max_str, 668) / 100.0;
  int max_speed = preferences.getInt(speed_str, 3500);
  int acceleration = preferences.getInt(acc_str, 10000);
  int initial_pos = preferences.getInt(pos_str, 0);
  
  _stepper->setMaxSpeed(max_speed);
  _stepper->setAcceleration(acceleration);
  _stepper->setCurrentPosition(initial_pos * _max_steps);

//  paths = /set, /max_steps/set, /speed/set, /accel/set, /step/set, /stop
  motor_mqtt_helper(mqtt_path, "/set", this, &Motor::set_pos);
  motor_mqtt_helper(mqtt_path, "/max_steps/set", this, &Motor::set_max_step);
  motor_mqtt_helper(mqtt_path, "/speed/set", this, &Motor::set_max_speed);
  motor_mqtt_helper(mqtt_path, "/accel/set", this, &Motor::set_acceleration);
  motor_mqtt_helper(mqtt_path, "/step/set", this, &Motor::set_current_step);
  motor_mqtt_helper(mqtt_path, "/stop", this, &Motor::set_stop);
}

boolean Motor::set_enable_pin(int level) {
  if (_stepper->isRunning() && level == LOW) {
    digitalWrite(_enable_pin, LOW);
    return true;
  }
  digitalWrite(_enable_pin, HIGH);
  return false;
}

void Motor::force_enable() {
  digitalWrite(_enable_pin, LOW);
}

void Motor::save(boolean only_step) {
  int str_len = 96;
  char pos_str[str_len];
  (_id + "_pos").toCharArray(pos_str, str_len);
  if (_max_steps == 0) {
    _max_steps = 1;
  }
  preferences.putInt(pos_str, _stepper->currentPosition() / _max_steps);
  if (only_step) {
    return;
  }
  char max_str[str_len], speed_str[str_len], acc_str[str_len];
  (_id + "_max_step").toCharArray(max_str, str_len);
  (_id + "_speed").toCharArray(speed_str, str_len);
  (_id + "_acc").toCharArray(acc_str, str_len);
  preferences.putInt(max_str, _max_steps * 100);
  preferences.putInt(speed_str, _stepper->maxSpeed());
  preferences.putInt(acc_str, _stepper->acceleration());
}

boolean Motor::run() {
  return _stepper->run();
}

void Motor::set_pos(const JsonDocument& local_doc) {
  flag_step_update = true;
  int percentage = local_doc["percentage"];
  _stepper->moveTo(percentage * _max_steps);
}

void Motor::set_max_step(const JsonDocument& local_doc) {
  _max_steps = ((float)local_doc["max_step"]) / 100.0;
}

void Motor::set_max_speed(const JsonDocument& local_doc) {
  _stepper->setMaxSpeed(local_doc["speed"]);
}

void Motor::set_acceleration(const JsonDocument& local_doc) {
  _stepper->setAcceleration(local_doc["accel"]);
}

void Motor::set_current_step(const JsonDocument& local_doc) {
  flag_step_update = true;
  int percentage = local_doc["percentage"];
  _stepper->setCurrentPosition(percentage * _max_steps);
}

void Motor::set_stop(const JsonDocument& local_doc) {
  flag_step_update = true;
  _stepper->stop();
}

void Motor::send_mqtt(JsonDocument& local_doc) {
  JsonObject json = local_doc.createNestedObject(_id);
  if (_max_steps == 0) {
    _max_steps = 1;
  }
  json["percentage"] = (int)(_stepper->currentPosition() / _max_steps);
  json["speed"] = _stepper->maxSpeed();
  json["accel"] = _stepper->acceleration();
  json["max_step"] = (int)(_max_steps * 100.0);
}

Motors::Motors(int initial_size) {
  _motors = (Motor**)malloc(initial_size * sizeof(Motor*));
  _max_size = initial_size;
  _size = 0;
  _state = IDLING;
}

void Motors::addMotor(Motor* motor) {
  if (_size == _max_size) {
    Serial.println("Max size of motors exceeded");
    return;
  }
  _motors[_size] = motor;
  _size += 1;
}

void Motors::save(boolean only_step) {
  for (int i = 0; i < _size; i++) {
    _motors[i]->save(only_step);
  }
}

void Motors::run() {
  switch(_state) {
    case IDLING: {
      boolean need_running = false;
      for (int i = 0; i < _size; i++) {
        need_running = need_running || _motors[i]->set_enable_pin(LOW);
      }
      if (need_running) {
        for (int i = 0; i < _size; i++) {
          _motors[i]->force_enable();
        }
        _start_wait = millis();
        _state = WAITING_ENABLE;
      }
      break;
    }
    case WAITING_ENABLE: {
      if (millis() - _start_wait > 5) {
        _state = RUNNING;
      }
      break;
    }
    case RUNNING: {
      boolean running = false;
      for (int i = 0; i < _size; i++) {
        running = running | _motors[i]->run();
      }
      if (!running) {
        _start_wait = millis();
        _state = WAITING_DISABLE;
        flag_step_update = true;
        flag_update = true;
      }
      break;
      
    }
    case WAITING_DISABLE: {
      if (millis() - _start_wait > 5) {
        _state = IDLING;
      }
      break;
    }
  }
}

void Motors::sendMQTT() {
  StaticJsonDocument<192> doc;

  for (int i = 0; i < _size; i++) {
    _motors[i]->send_mqtt(doc);
  }
  
  serializeJson(doc, msg);
  client.publish(publish_path_rolgordijnen, msg);
}
