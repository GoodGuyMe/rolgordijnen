#include <Arduino.h>

#include <AccelStepper.h>
#include <Preferences.h>

#include "window.h"
#include "mqtt.h"
#include "motor.h"

// Library for saving the state of the esp when turned off.
Preferences preferences;

void save(boolean only_step) {
  preferences.putInt("step_rechts", rolgordijn_rechts.currentPosition() / total_steps_rolgordijn_rechts);
  preferences.putInt("step_links",  rolgordijn_links.currentPosition()  / total_steps_rolgordijn_links);
  if (only_step) {
    return;
  }
  preferences.putInt("max_step_links", total_steps_rolgordijn_links * 100);
  preferences.putInt("max_step_rechts", total_steps_rolgordijn_rechts * 100);
  
  preferences.putInt("speed_links", rolgordijn_rechts.maxSpeed());
  preferences.putInt("speed_rechts", rolgordijn_rechts.maxSpeed());

  preferences.putInt("accel_links", rolgordijn_links.acceleration());
  preferences.putInt("accel_rechts", rolgordijn_rechts.acceleration());
}


void loadSave() {
  total_steps_rolgordijn_links = preferences.getInt("max_step_links", 668) / 100.0;
  total_steps_rolgordijn_rechts = preferences.getInt("max_step_rechts", 668) / 100.0;
  
  int new_step_links = preferences.getInt("step_links", 0);
  int new_step_rechts = preferences.getInt("step_rechts", 0);
  int new_speed_links = preferences.getInt("speed_links", 3500);
  int new_speed_rechts = preferences.getInt("speed_rechts", 3500);
  int new_accel_links = preferences.getInt("accel_links", 10000);
  int new_accel_rechts = preferences.getInt("accel_rechts", 10000);

  setupMotor(&rolgordijn_rechts, new_speed_links, new_accel_rechts, new_step_rechts * total_steps_rolgordijn_rechts);
  setupMotor(&rolgordijn_links, new_speed_rechts, new_accel_links, new_step_links * total_steps_rolgordijn_links);
}

void updateMaxStep(float* item, long max_step, const char* publish_path) {
  snprintf (msg, MSG_BUFFER_SIZE, "{\"percentage\": %u}", (long)max_step);
  client.publish(publish_path, msg);
}

void updateMaxSpeed(AccelStepper* motor, long max_speed, const char* publish_path) {
  snprintf (msg, MSG_BUFFER_SIZE, "{\"speed\": %u}", (long)max_speed);
  client.publish(publish_path, msg);
}

void updateAccel(AccelStepper* motor, long accel, const char* publish_path) {
  snprintf (msg, MSG_BUFFER_SIZE, "{\"accel\": %u}", (long)accel);
  client.publish(publish_path, msg);
}

void setup() {
  Serial.begin(115200);

  delay(10);
  pinMode(MOTOR_1_ENABLE, OUTPUT);
  pinMode(MOTOR_2_ENABLE, OUTPUT);

  delay(1000);

  digitalWrite(MOTOR_1_ENABLE, HIGH);
  digitalWrite(MOTOR_2_ENABLE, HIGH);

  // Load the saved state the table was in
  preferences.begin("rolgordijnen", false);
  loadSave();

  Serial.setTimeout(500);// Set time out for

  setup_mqtt();

  setup_check_window();

  rolgordijn_rechts.setPinsInverted(true, false, false);
  rolgordijn_links.setPinsInverted(true, false, false);
}

void loop() {
  checkMQTTMessage();
  runMotors();
  if (flag_window) {
    flag_window = false;
    send_window_message();
  }
  if (flag_update) {
    flag_update = false;
    save(flag_step_update);
    sendUpdateMessage();
  }
//  test_window();
}
