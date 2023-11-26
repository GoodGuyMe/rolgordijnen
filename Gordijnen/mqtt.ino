#include <Arduino.h>
#include "mqtt.h"
#include "motor.h"

char* createSubscribePath(const char* side, const char* extra_path) {
  char* subscribe_path = (char *)malloc(96);
  subscribe_path = strcpy(subscribe_path, publish_path_rolgordijnen);
  subscribe_path = strcat(subscribe_path, side);
  subscribe_path = strcat(subscribe_path, extra_path);
  return subscribe_path;
}

void callback(const char topic[], byte* payload, unsigned int length) {
  char* topicCopy = (char*)malloc(strlen(topic) + 1);
  topicCopy = strcpy(topicCopy, topic);

  byte* payloadCopy = (byte*)malloc(length);
  payloadCopy = (byte*)memcpy(payloadCopy, payload, length);

  StaticJsonDocument<128> doc;
  DeserializationError error = deserializeJson(doc, payloadCopy);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  flag_step_update = false;
  if (strcmp(topicCopy, subscribe_path_rolgordijn_rechts) == 0) {
    int percentage = doc["percentage"];
    rolgordijn_rechts.moveTo(percentage * total_steps_rolgordijn_rechts);
    flag_step_update = true;
  }
  else if (strcmp(topicCopy, subscribe_path_rolgordijn_links) == 0) {
    int percentage = doc["percentage"];
    rolgordijn_links.moveTo(percentage * total_steps_rolgordijn_links);
    flag_step_update = true;
  }
  else if (strcmp(topicCopy, subscribe_path_rolgordijnen) == 0) {
    int percentage = doc["percentage"];
    rolgordijn_rechts.moveTo(percentage * total_steps_rolgordijn_rechts);
    rolgordijn_links.moveTo(percentage * total_steps_rolgordijn_links);
    flag_step_update = true;
  }
  else if (strcmp(topicCopy, subscribe_path_rolgordijn_rechts_max) == 0) {
    total_steps_rolgordijn_rechts = ((float)doc["max_step"]) / 100.0;
  }
  else if (strcmp(topicCopy, subscribe_path_rolgordijn_links_max) == 0) {
    total_steps_rolgordijn_links = ((float)doc["max_step"]) / 100.0;
  }
  else if (strcmp(topicCopy, subscribe_path_rolgordijn_rechts_speed) == 0) {
    rolgordijn_rechts.setMaxSpeed(doc["speed"]);
    Serial.println(rolgordijn_rechts.maxSpeed());
  }
  else if (strcmp(topicCopy, subscribe_path_rolgordijn_links_speed) == 0) {
    rolgordijn_links.setMaxSpeed(doc["speed"]);
    Serial.println(rolgordijn_links.maxSpeed());
  }
  else if (strcmp(topicCopy, subscribe_path_rolgordijn_rechts_accel) == 0) {
    rolgordijn_rechts.setAcceleration(doc["accel"]);
  }
  else if (strcmp(topicCopy, subscribe_path_rolgordijn_links_accel) == 0) {
    rolgordijn_links.setAcceleration(doc["accel"]);
  }
  else if (strcmp(topicCopy, subscribe_path_rolgordijn_stop) == 0) {
    rolgordijn_links.stop();
    rolgordijn_rechts.stop();
  }
  flag_update = true;

  free(topicCopy);
  free(payloadCopy);

  Serial.println("Returning...");
}

void connectWiFi() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  int count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    if (count++ > 50) {
      count = 0;
      WiFi.disconnect();
      WiFi.begin(ssid, password);
      Serial.println();
    }
    delay(100);
    Serial.print(".");
  }
  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup_wifi() {
  delay(10);
  WiFi.begin(ssid, password);
  connectWiFi();
}

void reconnectWiFi() {
  WiFi.disconnect();
  WiFi.reconnect();
  connectWiFi();
}

void reconnectMQTT() {
  // Loop until we're reconnected
  while (!client.connected()) {
    if (WiFi.status() != WL_CONNECTED) {
      reconnectWiFi();
    }
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32Gordijnen-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(),MQTT_USER,MQTT_PASSWORD)) {
      Serial.println("connected");
      client.subscribe(subscribe_path_rolgordijn_rechts);
      client.subscribe(subscribe_path_rolgordijn_links);
      client.subscribe(subscribe_path_rolgordijnen);

      client.subscribe(subscribe_path_rolgordijn_rechts_max);
      client.subscribe(subscribe_path_rolgordijn_links_max);

      client.subscribe(subscribe_path_rolgordijn_rechts_speed);
      client.subscribe(subscribe_path_rolgordijn_links_speed);

      client.subscribe(subscribe_path_rolgordijn_stop);

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup_mqtt() {
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  subscribe_path_rolgordijn_rechts = createSubscribePath("/rechts", subsribe_path);
  subscribe_path_rolgordijn_links = createSubscribePath("/links", subsribe_path);
  subscribe_path_rolgordijnen = createSubscribePath("", subsribe_path);
  Serial.print("Subscribe path rolgordijnen: ");
  Serial.println(subscribe_path_rolgordijnen);

  subscribe_path_rolgordijn_rechts_max = createSubscribePath("/rechts", max_step_path);
  subscribe_path_rolgordijn_links_max = createSubscribePath("/links", max_step_path);

  subscribe_path_rolgordijn_rechts_speed = createSubscribePath("/rechts", speed_path);
  subscribe_path_rolgordijn_links_speed = createSubscribePath("/links", speed_path);

  subscribe_path_rolgordijn_rechts_accel = createSubscribePath("/rechts", accel_path);
  subscribe_path_rolgordijn_links_accel = createSubscribePath("/links", accel_path);

  subscribe_path_rolgordijn_stop = createSubscribePath("", stop_path);
  Serial.print("Subscribe path stop: ");
  Serial.println(subscribe_path_rolgordijn_stop);
  reconnectMQTT();
}

void sendUpdateMessage() {
  StaticJsonDocument<192> doc;

  JsonObject json_rechts = doc.createNestedObject("rolgordijn_rechts");
  json_rechts["percentage"] = rolgordijn_rechts.currentPosition() / total_steps_rolgordijn_rechts;
  json_rechts["speed"] = rolgordijn_rechts.maxSpeed();
  json_rechts["accel"] = rolgordijn_rechts.acceleration();
  json_rechts["max_step"] = (int)(total_steps_rolgordijn_rechts * 100.0);
  
  JsonObject json_links = doc.createNestedObject("rolgordijn_links");
  json_links["percentage"] = rolgordijn_links.currentPosition() / total_steps_rolgordijn_links;
  json_links["speed"] = rolgordijn_links.maxSpeed();
  json_links["accel"] = rolgordijn_links.acceleration();
  json_links["max_step"] = (int)(total_steps_rolgordijn_links * 100.0);
  
  serializeJson(doc, msg);
  client.publish(publish_path_rolgordijnen, msg);
}

void checkMQTTMessage() {
    if (WiFi.status() != WL_CONNECTED) {
      reconnectWiFi();
    } 
    if (!client.connected()) {
      reconnectMQTT();
    }
    client.loop();
    yield();
}
