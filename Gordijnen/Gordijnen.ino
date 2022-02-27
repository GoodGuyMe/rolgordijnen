#include <Arduino.h>

#include <AccelStepper.h>

#include <WiFi.h>
#include <PubSubClient.h>

WiFiClient espClient;
PubSubClient client(espClient);

const char* ssid = "Sorry?"; // Enter your WiFi name
const char* password =  "watzeije?"; // Enter WiFi password
const char* mqtt_server = "192.168.2.22";
#define mqtt_port 1883
#define MQTT_USER "openhabian"
#define MQTT_PASSWORD "openhabian"
#define DELAY_AFTER_SEND 100

#define MSG_BUFFER_SIZE  (1024)
char msg[MSG_BUFFER_SIZE];
char heapMSG[MSG_BUFFER_SIZE];

unsigned long nextPublishInterval = 300000;
unsigned long nextPublish = 0;

#include <ArduinoJson.h>

const int MOTOR_1_STEP = 22;
const int MOTOR_1_DIR = 23;
const int MOTOR_1_ENABLE = 21;

const int MOTOR_2_STEP = 25;
const int MOTOR_2_DIR = 26;
const int MOTOR_2_ENABLE = 27;

#define motorInterfaceType 1

const char* publish_path_rolgordijnen_on = "esp32/gordijnen/voor/on";

const char* publish_path_rolgordijn_rechts = "esp32/gordijnen/voor/rechts";
const char* publish_path_rolgordijn_links  = "esp32/gordijnen/voor/links";
const char* publish_path_rolgordijnen  = "esp32/gordijnen/voor";
      char* subscribe_path_rolgordijn_rechts;
      char* subscribe_path_rolgordijn_links;
      char* subscribe_path_rolgordijnen;

const char* publish_path_rolgordijn_rechts_max = "esp32/gordijnen/voor/rechts/max_steps";
const char* publish_path_rolgordijn_links_max = "esp32/gordijnen/voor/links/max_steps";
      char* subscribe_path_rolgordijn_rechts_max;
      char* subscribe_path_rolgordijn_links_max;

const char* publish_path_rolgordijn_rechts_speed = "esp32/gordijnen/voor/rechts/speed";
const char* publish_path_rolgordijn_links_speed = "esp32/gordijnen/voor/links/speed";
      char* subscribe_path_rolgordijn_rechts_speed;
      char* subscribe_path_rolgordijn_links_speed;

const char* publish_path_rolgordijn_rechts_accel = "esp32/gordijnen/voor/rechts/accel";
const char* publish_path_rolgordijn_links_accel = "esp32/gordijnen/voor/links/accel";
      char* subscribe_path_rolgordijn_rechts_accel;
      char* subscribe_path_rolgordijn_links_accel;


AccelStepper rolgordijn_rechts(motorInterfaceType, MOTOR_1_STEP, MOTOR_1_DIR);
AccelStepper rolgordijn_links(motorInterfaceType, MOTOR_2_STEP, MOTOR_2_DIR);

float total_steps_rolgordijn_rechts = 80; // * 100
float total_steps_rolgordijn_links = 80; // * 100

boolean flag_update_step_links = false;
boolean flag_update_step_rechts = false;
boolean flag_update_step_alles = false;
boolean flag_update_max_step_links = false;
boolean flag_update_max_step_rechts = false;
boolean flag_update_speed_links = false;
boolean flag_update_speed_rechts = false;
boolean flag_update_accel_links = false;
boolean flag_update_accel_rechts = false;

long new_step_links  = 0;
long new_step_rechts = 0;
long new_step_alles  = 0;
long new_max_step_links  = 80;
long new_max_step_rechts = 80;
long new_speed_links  = 800;
long new_speed_rechts = 800;
long new_accel_links  = 10000;
long new_accel_rechts = 10000;

void setupMotor(AccelStepper* motor) {
  motor->setMaxSpeed(800);
  motor->setAcceleration(10000);
}

char* createSubscribePath(char* subscribe_path, const char* publish_path) {
  subscribe_path = (char *)malloc(64);
  subscribe_path = strcpy(subscribe_path, publish_path);
  subscribe_path = strcat(subscribe_path, "/set");
  return subscribe_path;
}

void runSingleMotor(AccelStepper* motor, int percentage, const char* publish_path, float total_steps, int enable_pin) {
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

void runBothMotors(int percentage) {
  snprintf (msg, MSG_BUFFER_SIZE, "{\"percentage\": %u}", percentage);
  
  client.publish("esp32/gordijnen/voor/debug", "{\"msg\": \"Received command to run both motors\"}");
  
  long stepsL = ((long)percentage) * total_steps_rolgordijn_rechts;
  long stepsR = ((long)percentage) * total_steps_rolgordijn_links;

  rolgordijn_links.moveTo(stepsL);
  rolgordijn_rechts.moveTo(stepsR);
  
  digitalWrite(MOTOR_1_ENABLE, LOW);
  digitalWrite(MOTOR_2_ENABLE, LOW);
  client.publish("esp32/gordijnen/voor/debug", "{\"msg\": \"Running started\"}");
  delay(2);
  boolean isRunningLinks = true;
  boolean isRunningRechts = true;
  while (isRunningLinks && isRunningRechts) {
    isRunningLinks = rolgordijn_links.run();
    isRunningRechts = rolgordijn_rechts.run();
    yield();
  }
  while (isRunningLinks) {
    isRunningLinks = rolgordijn_links.run();
    yield();
  }
  while (isRunningRechts) {
    isRunningRechts = rolgordijn_rechts.run();
    yield();
  }
  digitalWrite(MOTOR_1_ENABLE, HIGH);
  digitalWrite(MOTOR_2_ENABLE, HIGH);
  
  client.publish(publish_path_rolgordijnen, msg);
  client.publish(publish_path_rolgordijn_rechts, msg);
  client.publish(publish_path_rolgordijn_links, msg);
}

void updateMaxStep(float* item, long max_step, const char* publish_path) {
  snprintf (msg, MSG_BUFFER_SIZE, "{\"percentage\": %u}", (long)max_step);
  client.publish(publish_path, msg);
  *item = ((float)max_step) / 100.0;
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
  if (strcmp(topicCopy, subscribe_path_rolgordijn_rechts) == 0) {
    flag_update_step_rechts = true;
    new_step_rechts = doc["percentage"];
    //runSingleMotor(&rolgordijn_rechts, doc["percentage"], publish_path_rolgordijn_rechts, total_steps_rolgordijn_rechts, MOTOR_1_ENABLE);
  }
  else if (strcmp(topicCopy, subscribe_path_rolgordijn_links) == 0) {
    flag_update_step_links = true;
    new_step_links = doc["percentage"];
    //runSingleMotor(&rolgordijn_links, doc["percentage"], publish_path_rolgordijn_links, total_steps_rolgordijn_links, MOTOR_2_ENABLE);
  }
  else if (strcmp(topicCopy, subscribe_path_rolgordijnen) == 0) {
    flag_update_step_alles = true;
    new_step_alles = doc["percentage"];
//    runBothMotors(doc["percentage"]);
  }
  else if (strcmp(topicCopy, subscribe_path_rolgordijn_rechts_max) == 0) {
    flag_update_max_step_rechts = true;
    new_max_step_rechts = doc["max_step"];
//    updateMaxStep(&total_steps_rolgordijn_rechts, doc["max_step"], publish_path_rolgordijn_rechts_max);
  }
  else if (strcmp(topicCopy, subscribe_path_rolgordijn_links_max) == 0) {
    flag_update_max_step_links = true;
    new_max_step_links = doc["max_step"];
//    updateMaxStep(&total_steps_rolgordijn_links, doc["max_step"], publish_path_rolgordijn_links_max);
  }
  else if (strcmp(topicCopy, subscribe_path_rolgordijn_rechts_speed) == 0) {
    flag_update_speed_rechts = true;
    new_speed_rechts = doc["speed"];
//    updateMaxSpeed(&rolgordijn_rechts, doc["speed"], publish_path_rolgordijn_rechts_speed);
  }
  else if (strcmp(topicCopy, subscribe_path_rolgordijn_links_speed) == 0) {
    flag_update_speed_links = true;
    new_speed_links = doc["speed"];
//    updateMaxSpeed(&rolgordijn_links, doc["speed"], publish_path_rolgordijn_links_speed);
  }
  else if (strcmp(topicCopy, subscribe_path_rolgordijn_rechts_accel) == 0) {
    flag_update_accel_rechts = true;
    new_accel_rechts = doc["accel"];
  }
  else if (strcmp(topicCopy, subscribe_path_rolgordijn_links_accel) == 0) {
    flag_update_accel_links = true;
    new_accel_links = doc["accel"];
  }
  
  free(topicCopy);
  free(payloadCopy);
  nextPublish = millis();
  
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
    if (client.connect(clientId.c_str(),MQTT_USER,MQTT_PASSWORD)) {
      Serial.println("connected");
      //Once connected, publish an announcement...
      // ... and resubscribe
      client.subscribe(subscribe_path_rolgordijn_rechts);
      client.subscribe(subscribe_path_rolgordijn_links);
      client.subscribe(subscribe_path_rolgordijnen);
    
      client.subscribe(subscribe_path_rolgordijn_rechts_max);
      client.subscribe(subscribe_path_rolgordijn_links_max);
      
      client.subscribe(subscribe_path_rolgordijn_rechts_speed);
      client.subscribe(subscribe_path_rolgordijn_links_speed);

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
  
  subscribe_path_rolgordijn_rechts = createSubscribePath(subscribe_path_rolgordijn_rechts, publish_path_rolgordijn_rechts);
  subscribe_path_rolgordijn_links = createSubscribePath(subscribe_path_rolgordijn_links, publish_path_rolgordijn_links);
  subscribe_path_rolgordijnen = createSubscribePath(subscribe_path_rolgordijnen, publish_path_rolgordijnen);

  subscribe_path_rolgordijn_rechts_max = createSubscribePath(subscribe_path_rolgordijn_rechts_max, publish_path_rolgordijn_rechts_max);
  subscribe_path_rolgordijn_links_max = createSubscribePath(subscribe_path_rolgordijn_links_max, publish_path_rolgordijn_links_max);
  
  subscribe_path_rolgordijn_rechts_speed = createSubscribePath(subscribe_path_rolgordijn_rechts_speed, publish_path_rolgordijn_rechts_speed);
  subscribe_path_rolgordijn_links_speed = createSubscribePath(subscribe_path_rolgordijn_links_speed, publish_path_rolgordijn_links_speed);

  reconnect();
  
  setupMotor(&rolgordijn_rechts);
  setupMotor(&rolgordijn_links);

  rolgordijn_rechts.setPinsInverted(true, false, false);
  rolgordijn_links.setPinsInverted(true, false, false);
  
  client.publish(publish_path_rolgordijnen_on, "{\"state\": \"ON\"}");
}

void loop() {
    if (!client.connected()) {
      reconnect();
    }
    client.loop();
    yield();
    if(flag_update_step_rechts) {
      runSingleMotor(&rolgordijn_rechts, new_step_rechts, publish_path_rolgordijn_rechts, total_steps_rolgordijn_rechts, MOTOR_1_ENABLE);
      flag_update_step_rechts = false;
    }
    if(flag_update_step_links) {
      runSingleMotor(&rolgordijn_links, new_step_links, publish_path_rolgordijn_links, total_steps_rolgordijn_links, MOTOR_2_ENABLE);
      flag_update_step_links = false;
    }
    if(flag_update_step_alles) {
      runBothMotors(new_step_alles);
      flag_update_step_alles = false;
    }
    if (flag_update_max_step_links) {
      updateMaxStep(&total_steps_rolgordijn_links, new_max_step_links, publish_path_rolgordijn_links_max);
      flag_update_max_step_links = false;
    }
    if (flag_update_max_step_rechts) {
      updateMaxStep(&total_steps_rolgordijn_rechts, new_max_step_rechts, publish_path_rolgordijn_rechts_max);
      flag_update_max_step_rechts = false;
    }
    if (flag_update_speed_links) {
      updateMaxSpeed(&rolgordijn_links, new_speed_links, publish_path_rolgordijn_links_speed);
      flag_update_speed_links = false;
    }
    if (flag_update_speed_rechts) {
      updateMaxSpeed(&rolgordijn_rechts, new_speed_rechts, publish_path_rolgordijn_rechts_speed);
      flag_update_speed_rechts = false;
    }
    if (flag_update_accel_links) {
      updateAccel(&rolgordijn_links, new_accel_links, publish_path_rolgordijn_links_accel);
      flag_update_accel_links = false;
    }
    if (flag_update_accel_rechts) {
      updateAccel(&rolgordijn_rechts, new_accel_rechts, publish_path_rolgordijn_rechts_accel);
      flag_update_accel_rechts = false;
    }
}
