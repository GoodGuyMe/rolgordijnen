#include <Arduino.h>

#include <AccelStepper.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "secrets.h"
#include "mqtt.h"
#include "motor.h"

WiFiClient espClient;
PubSubClient client(espClient);

#define DELAY_AFTER_SEND 100

long total_steps_rolgordijn_rechts = 0;
long total_steps_rolgordijn_links = 0;
long total_steps_gordijn_rechts = 0;
long total_steps_gordijn_links = 0;

//Update flags
boolean flag_update_step_rol = false;
boolean flag_update_step_rails = false;

boolean flag_update_step_rol_rechts = false;
boolean flag_update_step_rol_links = false;

boolean flag_update_step_rails_rechts = false;
boolean flag_update_step_rails_links = false;

boolean flag_update_max_step = true;
boolean flag_update_speed = true;
boolean flag_update_accel = true;

long new_step_rol_links  = 0;
long new_step_rol_rechts = 0;
long new_step_rol_alles  = 0;
long new_max_step_rol_links  = 80;
long new_max_step_rol_rechts = 80;
long new_speed_rol_links  = 800;
long new_speed_rol_rechts = 800;
long new_accel_rol_links  = 10000;
long new_accel_rol_rechts = 10000;

long new_step_rails_links  = 0;
long new_step_rails_rechts = 0;
long new_step_rails_alles  = 0;
long new_max_step_rails_links  = 80;
long new_max_step_rails_rechts = 80;
long new_speed_rails_links  = 800;
long new_speed_rails_rechts = 800;
long new_accel_rails_links  = 10000;
long new_accel_rails_rechts = 10000;

void setupMotor(AccelStepper* motor) {
  motor->setMaxSpeed(800);
  motor->setAcceleration(10000);
}

char* concatPath(char* new_path, char* old_path, char* add_path) {
  new_path = (char *)malloc(64);
  new_path = strcpy(new_path, old_path);
  new_path = strcat(new_path, add_path);
  return new_path;
}

char * createSubscribePath(char* new_path, char* old_path) {
  return concatPath(new_path, old_path, "/set");
}

void runSingleMotor(AccelStepper* motor, int percentage, const char* publish_path, long total_steps, int enable_pin) {
  snprintf (msg, MSG_BUFFER_SIZE, "{\"percentage\": %u}", percentage);

  int steps = percentage * total_steps;
  Serial.print("Turning to position: ");
  Serial.println(steps);

  digitalWrite(enable_pin, LOW);
  delay(2);
  motor->runToNewPosition(steps);
  digitalWrite(enable_pin, HIGH);
  client.publish(publish_path, msg);
}

void runBothMotors(AccelStepper* motor1, AccelStepper* motor2,
                   int motor_1_enable, int motor_2_enable,
                   const char* publish_path_both, const char* publish_path_1, const char* publish_path_2,
                   long total_steps_1, long total_steps_2,
                   int percentage) {

  long stepsL = percentage * total_steps_1;
  long stepsR = percentage * total_steps_2;

  motor1->moveTo(stepsL);
  motor2->moveTo(stepsR);

  digitalWrite(motor_1_enable, LOW);
  digitalWrite(motor_2_enable, LOW);
  delay(2);
  boolean isRunningLinks = true;
  boolean isRunningRechts = true;
  while (isRunningLinks && isRunningRechts) {
    isRunningLinks = motor1->run();
    isRunningRechts = motor2->run();
    yield();
  }
  while (isRunningLinks) {
    isRunningLinks = motor1->run();
    yield();
  }
  while (isRunningRechts) {
    isRunningRechts = motor2->run();
    yield();
  }
  digitalWrite(motor_1_enable, HIGH);
  digitalWrite(motor_2_enable, HIGH);

  snprintf (msg, MSG_BUFFER_SIZE, "{\"percentage\": %u}", percentage);

  client.publish(publish_path_both, msg);
  client.publish(publish_path_1, msg);
  client.publish(publish_path_2, msg);
}

void updateMaxStep(long* item, long max_step, const char* publish_path) {
  snprintf (msg, MSG_BUFFER_SIZE, "{\"percentage\": %u}", (long)max_step);
  client.publish(publish_path, msg);
  *item = max_step;
}

void updateMaxSpeed(AccelStepper* motor, long max_speed, const char* publish_path) {
  snprintf (msg, MSG_BUFFER_SIZE, "{\"speed\": %u}", (long)max_speed);
  client.publish(publish_path, msg);
  motor->setMaxSpeed(max_speed);
}

void updateAccel(AccelStepper* motor, long accel, const char* publish_path) {
  snprintf (msg, MSG_BUFFER_SIZE, "{\"accel\": %u}", (long)accel);
  client.publish(publish_path, msg);
  motor->setAcceleration(accel);
}

void cmpPath(StaticJsonDocument<48> doc, char* topic, char* testTopic, boolean* flag, long* value, char* str) {
  if (strcmp(topic, testTopic) == 0) {
    Serial.println(topic);
    *flag = true;
    *value = doc[str];
    Serial.println("Done with the things!");
  }
}

void callback(const char topic[], byte* payload, unsigned int length) {
  char* topicCopy = (char*)malloc(strlen(topic) + 1);
  topicCopy = strcpy(topicCopy, topic);

  byte* payloadCopy = (byte*)malloc(length);
  payloadCopy = (byte*)memcpy(payloadCopy, payload, length);

  StaticJsonDocument<48> doc;
  DeserializationError error = deserializeJson(doc, payloadCopy);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    client.publish("esp32/gordijnen/error", "{\"error\":\"COULD NOT DECODE\"}");
    return;
  }
  cmpPath(doc, topicCopy, subscribe_path_rolgordijn_rechts, &flag_update_step_rol_rechts, &new_step_rol_rechts, "percentage");
  cmpPath(doc, topicCopy, subscribe_path_rolgordijn_links,  &flag_update_step_rol_links, &new_step_rol_links, "percentage");

  cmpPath(doc, topicCopy, subscribe_path_gordijn_rechts, &flag_update_step_rails_rechts, &new_step_rails_rechts, "percentage");
  cmpPath(doc, topicCopy, subscribe_path_gordijn_links, &flag_update_step_rails_links, &new_step_rails_links,   "percentage");

  cmpPath(doc, topicCopy, subscribe_path_rolgordijnen, &flag_update_step_rol,   &new_step_rol_alles,   "percentage");
  cmpPath(doc, topicCopy, subscribe_path_gordijnen,    &flag_update_step_rails, &new_step_rails_alles, "percentage");

  cmpPath(doc, topicCopy, subscribe_path_rolgordijn_rechts_max, &flag_update_max_step, &new_max_step_rol_rechts,   "max_step");
  cmpPath(doc, topicCopy, subscribe_path_gordijn_rechts_max,    &flag_update_max_step, &new_max_step_rails_rechts, "max_step");

  cmpPath(doc, topicCopy, subscribe_path_rolgordijn_links_max, &flag_update_max_step, &new_max_step_rol_links,   "max_step");
  cmpPath(doc, topicCopy, subscribe_path_gordijn_links_max,    &flag_update_max_step, &new_max_step_rails_links, "max_step");

  cmpPath(doc, topicCopy, subscribe_path_rolgordijn_rechts_speed, &flag_update_speed, &new_speed_rol_rechts,   "speed");
  cmpPath(doc, topicCopy, subscribe_path_gordijn_rechts_speed,    &flag_update_speed, &new_speed_rails_rechts, "speed");

  cmpPath(doc, topicCopy, subscribe_path_rolgordijn_links_speed, &flag_update_speed, &new_speed_rol_links,   "speed");
  cmpPath(doc, topicCopy, subscribe_path_gordijn_links_speed,    &flag_update_speed, &new_speed_rails_links, "speed");

  cmpPath(doc, topicCopy, subscribe_path_rolgordijn_rechts_accel, &flag_update_accel, &new_accel_rol_rechts,   "accel");
  cmpPath(doc, topicCopy, subscribe_path_gordijn_rechts_accel,    &flag_update_accel, &new_accel_rails_rechts, "accel");

  cmpPath(doc, topicCopy, subscribe_path_rolgordijn_links_speed, &flag_update_accel, &new_accel_rol_links,   "accel");
  cmpPath(doc, topicCopy, subscribe_path_gordijn_links_speed,    &flag_update_accel, &new_accel_rails_links, "accel");

  free(topicCopy);
  free(payloadCopy);

  Serial.println("Returning...");
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32Gordijnen-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("connected");
      //Once connected, publish an announcement...
      // ... and resubscribe
      //      Rolgordijnen
      client.subscribe(subscribe_path_rolgordijn_rechts);
      client.subscribe(subscribe_path_rolgordijn_links);
      client.subscribe(subscribe_path_rolgordijnen);

      client.subscribe(subscribe_path_rolgordijn_rechts_max);
      client.subscribe(subscribe_path_rolgordijn_links_max);

      client.subscribe(subscribe_path_rolgordijn_rechts_speed);
      client.subscribe(subscribe_path_rolgordijn_links_speed);

      //      Gordijnen
      client.subscribe(subscribe_path_gordijn_rechts);
      client.subscribe(subscribe_path_gordijn_links);
      client.subscribe(subscribe_path_gordijnen);

      client.subscribe(subscribe_path_gordijn_rechts_max);
      client.subscribe(subscribe_path_gordijn_links_max);

      client.subscribe(subscribe_path_gordijn_rechts_speed);
      client.subscribe(subscribe_path_gordijn_links_speed);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  delay(10);
  pinMode(MOTOR_1_ENABLE, OUTPUT);
  pinMode(MOTOR_2_ENABLE, OUTPUT);

  delay(1000);

  digitalWrite(MOTOR_1_ENABLE, HIGH);
  digitalWrite(MOTOR_2_ENABLE, HIGH);

  Serial.setTimeout(500);// Set time out for
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  publish_path_rolgordijn_rechts = concatPath(publish_path_rolgordijn_rechts, begin_path, "/rol/rechts");
  publish_path_rolgordijn_links = concatPath(publish_path_rolgordijn_links, begin_path, "/rol/links");
  publish_path_rolgordijnen = concatPath(publish_path_rolgordijnen, begin_path, "/rol");

  publish_path_gordijn_rechts = concatPath(publish_path_gordijn_rechts, begin_path, "/rails/rechts");
  publish_path_gordijn_links = concatPath(publish_path_gordijn_links, begin_path, "/rails/links");
  publish_path_gordijnen = concatPath(publish_path_gordijnen, begin_path, "/rails");

  publish_path_rolgordijn_rechts_max = concatPath(publish_path_rolgordijn_rechts_max, begin_path, "/rol/rechts/max_steps");
  publish_path_rolgordijn_links_max = concatPath(publish_path_rolgordijn_links_max, begin_path, "/rol/rechts/max_steps");

  publish_path_gordijn_rechts_max = concatPath(publish_path_gordijn_rechts_max, begin_path, "/rails/rechts/max_steps");
  publish_path_gordijn_links_max = concatPath(publish_path_gordijn_links_max, begin_path, "/rails/rechts/max_steps");

  publish_path_rolgordijn_links_speed = concatPath(publish_path_rolgordijn_links_speed, begin_path, "/rol/links/speed");
  publish_path_rolgordijn_rechts_speed = concatPath(publish_path_rolgordijn_rechts_speed, begin_path, "/rol/rechts/speed");

  publish_path_gordijn_links_speed = concatPath(publish_path_gordijn_links_speed, begin_path, "/rails/links/speed");
  publish_path_gordijn_rechts_speed = concatPath(publish_path_gordijn_rechts_speed, begin_path, "/rails/rechts/speed");

  publish_path_rolgordijn_rechts_accel = concatPath(publish_path_rolgordijn_rechts_speed, begin_path, "/rol/rechts/accel");
  publish_path_rolgordijn_links_accel = concatPath(publish_path_rolgordijn_rechts_speed, begin_path, "/rol/links/accel");

  publish_path_gordijn_rechts_accel = concatPath(publish_path_gordijn_rechts_speed, begin_path, "/rails/rechts/accel");
  publish_path_gordijn_links_accel = concatPath(publish_path_gordijn_rechts_speed, begin_path, "/rails/links/accel");

  subscribe_path_rolgordijn_rechts = createSubscribePath(subscribe_path_rolgordijn_rechts, publish_path_rolgordijn_rechts);
  subscribe_path_rolgordijn_links = createSubscribePath(subscribe_path_rolgordijn_links, publish_path_rolgordijn_links);
  subscribe_path_rolgordijnen = createSubscribePath(subscribe_path_rolgordijnen, publish_path_rolgordijnen);

  subscribe_path_gordijn_rechts = createSubscribePath(subscribe_path_gordijn_rechts, publish_path_gordijn_rechts);
  subscribe_path_gordijn_links = createSubscribePath(subscribe_path_gordijn_links, publish_path_gordijn_links);
  subscribe_path_gordijnen = createSubscribePath(subscribe_path_gordijnen, publish_path_gordijnen);

  subscribe_path_rolgordijn_rechts_max = createSubscribePath(subscribe_path_rolgordijn_rechts_max, publish_path_rolgordijn_rechts_max);
  subscribe_path_rolgordijn_links_max = createSubscribePath(subscribe_path_rolgordijn_links_max, publish_path_rolgordijn_links_max);

  subscribe_path_gordijn_rechts_max = createSubscribePath(subscribe_path_gordijn_rechts_max, publish_path_gordijn_rechts_max);
  subscribe_path_gordijn_links_max = createSubscribePath(subscribe_path_gordijn_links_max, publish_path_gordijn_links_max);

  subscribe_path_rolgordijn_rechts_speed = createSubscribePath(subscribe_path_rolgordijn_rechts_speed, publish_path_rolgordijn_rechts_speed);
  subscribe_path_rolgordijn_links_speed = createSubscribePath(subscribe_path_rolgordijn_links_speed, publish_path_rolgordijn_links_speed);

  subscribe_path_gordijn_rechts_speed = createSubscribePath(subscribe_path_gordijn_rechts_speed, publish_path_gordijn_rechts_speed);
  subscribe_path_gordijn_links_speed = createSubscribePath(subscribe_path_gordijn_links_speed, publish_path_gordijn_links_speed);

  subscribe_path_rolgordijn_rechts_accel = createSubscribePath(subscribe_path_rolgordijn_rechts_accel, publish_path_rolgordijn_rechts_accel);
  subscribe_path_rolgordijn_links_accel = createSubscribePath(subscribe_path_rolgordijn_links_accel, publish_path_rolgordijn_links_accel);
  
  subscribe_path_gordijn_rechts_accel = createSubscribePath(subscribe_path_gordijn_rechts_accel, publish_path_gordijn_rechts_accel);
  subscribe_path_gordijn_links_accel = createSubscribePath(subscribe_path_gordijn_links_accel, publish_path_gordijn_links_accel);

  setupMotor(&rolgordijn_links);
  setupMotor(&rolgordijn_rechts);
  setupMotor(&gordijn_links);
  setupMotor(&gordijn_rechts);

  rolgordijn_rechts.setPinsInverted(true, false, false);
  rolgordijn_links.setPinsInverted(true, false, false);

  reconnect();

  client.publish(begin_path, "{\"state\": \"ON\"}");
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  yield();
  if (flag_update_step_rol_rechts) {
    runSingleMotor(&rolgordijn_rechts, new_step_rol_rechts, publish_path_rolgordijn_rechts, total_steps_rolgordijn_rechts, MOTOR_1_ENABLE);
    flag_update_step_rol_rechts = false;
  }
  if (flag_update_step_rol_links) {
    runSingleMotor(&rolgordijn_links, new_step_rol_links, publish_path_rolgordijn_links, total_steps_rolgordijn_links, MOTOR_2_ENABLE);
    flag_update_step_rol_links = false;
  }
  if (flag_update_step_rails_rechts) {
    runSingleMotor(&gordijn_rechts, new_step_rails_rechts, publish_path_gordijn_rechts, total_steps_gordijn_rechts, MOTOR_3_ENABLE);
    flag_update_step_rails_rechts = false;
  }
  if (flag_update_step_rails_links) {
    runSingleMotor(&gordijn_links, new_step_rails_links, publish_path_gordijn_links, total_steps_gordijn_links, MOTOR_4_ENABLE);
    flag_update_step_rails_links = false;
  }
  if (flag_update_step_rol) {
    runBothMotors(&rolgordijn_rechts, &rolgordijn_links,
                  MOTOR_1_ENABLE, MOTOR_2_ENABLE,
                  publish_path_rolgordijnen, publish_path_rolgordijn_rechts, publish_path_rolgordijn_links,
                  new_max_step_rol_rechts, new_max_step_rol_links,
                  new_step_rol_alles);
    flag_update_step_rol = false;
  }
  if (flag_update_step_rails) {
    runBothMotors(&gordijn_rechts, &gordijn_links,
                  MOTOR_3_ENABLE, MOTOR_4_ENABLE,
                  publish_path_gordijnen, publish_path_gordijn_rechts, publish_path_gordijn_links,
                  new_max_step_rails_rechts, new_max_step_rails_links,
                  new_step_rails_alles);
    flag_update_step_rails = false;
  }
  if (flag_update_max_step) {
    updateMaxStep(&total_steps_rolgordijn_links,  new_max_step_rol_links,  publish_path_rolgordijn_links_max);
    updateMaxStep(&total_steps_rolgordijn_rechts, new_max_step_rol_rechts, publish_path_rolgordijn_rechts_max);
    updateMaxStep(&total_steps_gordijn_links,  new_max_step_rails_links,  publish_path_gordijn_links_max);
    updateMaxStep(&total_steps_gordijn_rechts, new_max_step_rails_rechts, publish_path_gordijn_rechts_max);
    flag_update_max_step = false;
  }
  if (flag_update_speed) {
    updateMaxSpeed(&rolgordijn_links,  new_speed_rol_links,  publish_path_rolgordijn_links_speed);
    updateMaxSpeed(&rolgordijn_rechts, new_speed_rol_rechts, publish_path_rolgordijn_rechts_speed);
    updateMaxSpeed(&gordijn_links,  new_speed_rails_links,  publish_path_gordijn_links_speed);
    updateMaxSpeed(&gordijn_rechts, new_speed_rails_rechts, publish_path_gordijn_rechts_speed);
    flag_update_speed = false;
  }
  if (flag_update_accel) {
    updateAccel(&rolgordijn_links,  new_accel_rol_links,  publish_path_rolgordijn_links_accel);
    updateAccel(&rolgordijn_rechts, new_accel_rol_rechts, publish_path_rolgordijn_rechts_accel);
    updateAccel(&gordijn_links,  new_accel_rails_links,  publish_path_gordijn_links_accel);
    updateAccel(&gordijn_rechts, new_accel_rails_rechts, publish_path_gordijn_rechts_accel);
    flag_update_accel = false;
  }
}
