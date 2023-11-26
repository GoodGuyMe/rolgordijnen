#include "window.h"
#include "mqtt.h"


unsigned long t_last_send_time = 0;
unsigned long t_debounce_time = 200;

unsigned long last_send_time = 0;
unsigned long debounce_time = 500;

void IRAM_ATTR window_isr() {
  if (millis() - last_send_time > debounce_time) {
    last_send_time = millis();
    flag_window = true;
  }
}

boolean setup_check_window() {
  pinMode(window_pin, INPUT_PULLUP);
  attachInterrupt(window_pin, window_isr, CHANGE);

  publish_path_rolgordijnen_window = createSubscribePath("", window_path);
}

void send_window_message() {
  if (digitalRead(window_pin)) {
    client.publish(publish_path_rolgordijnen_window, "{\"state\": \"OPEN\"}");
  } else {
    client.publish(publish_path_rolgordijnen_window, "{\"state\": \"CLOSED\"}");
  }
}

void test_window() {
  if (millis() - t_last_send_time > t_debounce_time) {
    t_last_send_time = millis();
    Serial.println(digitalRead(window_pin));
  }
}
