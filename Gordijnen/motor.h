#define motorInterfaceType 1

const int MOTOR_1_STEP = 22;
const int MOTOR_1_DIR = 23;
const int MOTOR_1_ENABLE = 21;

const int MOTOR_2_STEP = 25;
const int MOTOR_2_DIR = 26;
const int MOTOR_2_ENABLE = 27;

const int MOTOR_3_STEP = 15;
const int MOTOR_3_DIR = 2;
const int MOTOR_3_ENABLE = 4;

const int MOTOR_4_STEP = 32;
const int MOTOR_4_DIR = 35;
const int MOTOR_4_ENABLE = 34;

AccelStepper rolgordijn_rechts(motorInterfaceType, MOTOR_1_STEP, MOTOR_1_DIR);
AccelStepper rolgordijn_links(motorInterfaceType, MOTOR_2_STEP, MOTOR_2_DIR);
AccelStepper gordijn_rechts(motorInterfaceType, MOTOR_3_STEP, MOTOR_3_DIR);
AccelStepper gordijn_links(motorInterfaceType, MOTOR_4_STEP, MOTOR_4_DIR);
