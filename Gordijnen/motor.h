#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <AccelStepper.h>

#define motorInterfaceType 1

// Library for saving the state of the esp when turned off.
Preferences preferences;

const int MOTOR_1_STEP = 22;
const int MOTOR_1_DIR = 23;
const int MOTOR_1_ENABLE = 21;

const int MOTOR_2_STEP = 25;
const int MOTOR_2_DIR = 26;
const int MOTOR_2_ENABLE = 27;

class Motor {
  public:
    Motor(const char* mqtt_path, const int step_pin, const int dir_pin, const int enable_pin, String id, boolean inverted);
    boolean set_enable_pin(int level);
    void force_enable();
    void save(boolean only_step);
    void send_mqtt(JsonDocument& local_doc);
    boolean run();
    void set_pos(const JsonDocument& local_doc);
    void set_stop(const JsonDocument& local_doc);
  private:
    void set_max_step(const JsonDocument& local_doc);
    void set_max_speed(const JsonDocument& local_doc);
    void set_acceleration(const JsonDocument& local_doc);
    void set_current_step(const JsonDocument& local_doc);
    AccelStepper* _stepper;
    int _enable_pin;
    int _max_steps;
    String _id;
};

typedef enum {
  IDLING,
  WAITING_ENABLE,
  RUNNING,
  WAITING_DISABLE,
} motors_state;

class Motors {
  public:
    Motors(int initial_size);
    void addMotor(Motor* motor);
    void save(boolean only_step);
    void run();
    void sendMQTT();
  private:
    Motor** _motors;
    int _max_size;
    int _size = 0;
    motors_state _state;
    unsigned long _start_wait;
};
