#include <Arduino.h>


#include "window.h"
#include "mqtt.h"
#include "motor.h"
#include "mqtt_handler.h"

Motors motors(2);

void setup() {
  Serial.begin(115200);

  delay(10);

  // Load the saved state the table was in
  preferences.begin("rolgordijnen", false);

  Serial.setTimeout(500);// Set time out for

  setup_check_window();

  char left_path[96];
  strcpy(left_path, publish_path_rolgordijnen);
  strcat(left_path, "/links");
  Motor* rol_left = new Motor(left_path, MOTOR_1_STEP, MOTOR_1_DIR, MOTOR_1_ENABLE, "links", false);
  motors.addMotor(rol_left);

  char right_path[96];
  strcpy(right_path, publish_path_rolgordijnen);
  strcat(right_path, "/rechts");
  Motor* rol_right = new Motor(right_path, MOTOR_2_STEP, MOTOR_2_DIR, MOTOR_2_ENABLE, "rechts", true);
  motors.addMotor(rol_right);

  char set_path[96];
  strcpy(set_path, publish_path_rolgordijnen);
  strcat(set_path, "/set");
  first_mqtt->add_mqtt_path(set_path, rol_left,  &Motor::set_pos);
  first_mqtt->add_mqtt_path(set_path, rol_right, &Motor::set_pos);

  char stop_path[96];
  strcpy(stop_path, publish_path_rolgordijnen);
  strcat(stop_path, "/stop");
  first_mqtt->add_mqtt_path(stop_path, rol_left,  &Motor::set_stop);
  first_mqtt->add_mqtt_path(stop_path, rol_right, &Motor::set_stop);
  
  setup_mqtt();
}

void loop() {
  checkMQTTMessage();
  motors.run();
  if (flag_window) {
    flag_window = false;
    send_window_message();
  }
  if (flag_update) {
    flag_update = false;
    motors.save(flag_step_update);
    motors.sendMQTT();
  }
  yield();
}
