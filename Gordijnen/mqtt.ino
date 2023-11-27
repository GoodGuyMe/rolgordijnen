#include <Arduino.h>
#include "mqtt.h"
#include "motor.h"

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
  if (first_mqtt != NULL) {
    first_mqtt->compare(topicCopy, doc);
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
      if (first_mqtt != NULL) {
        first_mqtt->subscribe();
      }
      Serial.println("connected");
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
  reconnectMQTT();
}

void checkMQTTMessage() {
    if (WiFi.status() != WL_CONNECTED) {
      reconnectWiFi();
    } 
    if (!client.connected()) {
      reconnectMQTT();
    }
    client.loop();
}
