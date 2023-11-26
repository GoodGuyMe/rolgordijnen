#pragma once
#include <Arduino.h>
const char* window_path = "/window";
      char* publish_path_rolgordijnen_window;

boolean flag_window = true;
const int window_pin = 13;

boolean setup_check_window();
void send_window_message();

void check_window();
void test_window();
