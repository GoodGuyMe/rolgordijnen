#include "motor.h"

void setupMotor(AccelStepper* motor, long max_speed, long acceleration, long initial_pos) {
  motor->setMaxSpeed(max_speed);
  motor->setAcceleration(acceleration);
  motor->setCurrentPosition(initial_pos);
}

void runMotors() {
  if (!rolgordijn_links.isRunning() && !rolgordijn_rechts.isRunning()) {
    return;
  }

  digitalWrite(MOTOR_1_ENABLE, LOW);
  digitalWrite(MOTOR_2_ENABLE, LOW);

  delay(2);
  boolean isRunningLinks = true;
  boolean isRunningRechts = true;
  while (isRunningLinks || isRunningRechts) {
    isRunningLinks = rolgordijn_links.run();
    isRunningRechts = rolgordijn_rechts.run();
    checkMQTTMessage();
    yield();
  }
  delay(2);
  digitalWrite(MOTOR_1_ENABLE, HIGH);
  digitalWrite(MOTOR_2_ENABLE, HIGH);
}
