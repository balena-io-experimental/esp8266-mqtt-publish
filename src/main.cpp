#include <Arduino.h>
/*
  To upload through terminal you can use: curl -F "image=@firmware.bin" <mac>.local/update
*/

// ENM deps
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>

// application deps
#include <PubSubClient.h>

#define INTERVAL 60000

const char* applicationUUID = "123456789";
const char* ssid = "resin-hotspot";
const char* password = "resin-hotspot";
const char* mqtt_server = "iot.eclipse.org";
const char* TOPIC = "sometopic";
char data[75];
bool newData = false;

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;

void setup(void) {
  Serial.begin(115200);
  Serial.println();

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(2, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(2, HIGH);

  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.hostname(applicationUUID);

  Serial.print("Connecting");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
      delay(500);
      Serial.print(".");
  }
  Serial.println("\nConnected");

  // set up mqtt stuff
  client.setServer(mqtt_server, 1883);

  httpUpdater.setup(&httpServer);
  httpServer.begin();
}

int connect_mqtt() {
  if (client.connected()) {
    return 0;
  }

  Serial.print("Attempting MQTT connection...");

  // Create a random client ID
  String clientId = "ESP8266Client-";
  clientId += String(random(0xffff), HEX);

  // Attempt to connect
  if (client.connect(clientId.c_str())) {
    Serial.println("connected");
    return 0;
  }

  Serial.print("failed, rc=");
  Serial.print(client.state());
  return 1;
}

void doSomething() {
  data = "some new piece of data";
  newData = true;
}

void transmit() {
  char topic[75];

  Serial.println("transmitting");

  snprintf(topic, 75, "%s/somemetric", TOPIC);
  Serial.print(topic);
  Serial.println(" ");
  Serial.println(data);
  client.publish(topic, hum);

  newData = false;
}

void loop(void) {
  if (WiFi.isConnected() == true) {
    // turn on the LED
    digitalWrite(2, LOW);

    if (newData == true) {
      digitalWrite(LED_BUILTIN, LOW);

      // only try to connect when there's a new reading
      connect();
      transmit();

      digitalWrite(LED_BUILTIN, HIGH);
    }

    // allow updates to happen
    httpServer.handleClient();

    return;
  }

  // turn the LED off
  digitalWrite(2, HIGH);

  // since we're not connected, take our action now
  long now = millis();
  if (now - lastMsg > INTERVAL) {
    lastMsg = now;
    doSomething();
  }
}
