#pragma once
#include <Arduino.h>
#define motorInterfaceType 1

const int MOTOR_1_STEP = 22;
const int MOTOR_1_DIR = 23;
const int MOTOR_1_ENABLE = 21;

const int MOTOR_2_STEP = 25;
const int MOTOR_2_DIR = 26;
const int MOTOR_2_ENABLE = 27;

AccelStepper rolgordijn_rechts(motorInterfaceType, MOTOR_1_STEP, MOTOR_1_DIR);
AccelStepper rolgordijn_links(motorInterfaceType, MOTOR_2_STEP, MOTOR_2_DIR);

//MultiStepper rolgordijnen;

float total_steps_rolgordijn_rechts = 80; // * 100
float total_steps_rolgordijn_links = 80; // * 100

void setupMotor(AccelStepper* motor, long max_speed, long acceleration, long initial_pos);
void runMotors();
