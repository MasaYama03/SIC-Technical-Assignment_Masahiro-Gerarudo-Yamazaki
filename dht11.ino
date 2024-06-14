#include <WiFi.h>
#include <WiFiAP.h>
#include <WiFiClient.h>
#include <WiFiGeneric.h>
#include <WiFiMulti.h>
#include <WiFiSTA.h>
#include <WiFiScan.h>
#include <WiFiServer.h>
#include <WiFiType.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include "DHT.h"
#define DHT11PIN 4

// Create a DHT object with the specified pin and sensor type
DHT dht(DHT11PIN, DHT11);

// WiFi and MQTT settings
const char* ssid = "ALDI";
const char* password = "Tujuhsatu";
const char* mqtt_server = "test.mosquitto.org";
const int port = 1883;

WiFiClient espClient; // Create a WiFi client
PubSubClient client(espClient); // Create a PubSubClient using the WiFi client
unsigned long lastMsg = 0; // Store the last message time
float temp = 0; // Variable to store temperature
float hum = 0; // Variable to store humidity

// MQTT topics
const char* topic_temperature = "/sensor/data/temperature";
const char* topic_humidity = "/sensor/data/humidity";
const char* topic_command = "/sic/command/mqtt";

void setup_wifi() { 
  // Function to connect to the WiFi network
  delay(200); // Delay for stability
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA); // Set WiFi mode to station
  WiFi.begin(ssid, password); // Connect to the WiFi network

  // Wait until connected to the WiFi network
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP()); // Print the local IP address
}

void callback(char* topic, byte* payload, unsigned int length) { 
  // Function to handle incoming MQTT messages
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) { // Print the message payload
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Control the LED based on the received message
  if ((char)payload[0] == '1') {
    digitalWrite(2, LOW);   // Turn the LED on (active low on the ESP-01)
  } else {
    digitalWrite(2, HIGH);  // Turn the LED off
  }
}

void reconnect() { 
  // Function to reconnect to the MQTT broker
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("Connected");
      // Once connected, publish an announcement...
      client.publish("/sic/mqtt", "Hello!"); // Publish a message to the broker
      // ... and resubscribe
      client.subscribe(topic_command); // Subscribe to the command topic
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000); // Wait 5 seconds before retrying
    }
  }
}

void setup() {
  pinMode(2, OUTPUT); // Initialize pin 2 as output (for LED control)
  Serial.begin(115200); // Start serial communication
  setup_wifi(); // Connect to WiFi
  client.setServer(mqtt_server, port); // Connect to the MQTT broker
  client.setCallback(callback); // Set the callback function for incoming messages
  dht.begin(); // Initialize the DHT sensor
}

void loop() {
  if (!client.connected()) {
    reconnect(); // Reconnect to the MQTT broker if disconnected
  }
  client.loop(); // Process incoming messages

  unsigned long now = millis();
  if (now - lastMsg > 2000) { // Publish data every 2 seconds
    lastMsg = now;
    float humi = dht.readHumidity(); // Read humidity
    float temp = dht.readTemperature(); // Read temperature

    String temp_string = String(temp, 2); // Convert temperature to string
    client.publish(topic_temperature, temp_string.c_str()); // Publish temperature data
    
    String hum_string = String(humi, 1); // Convert humidity to string
    client.publish(topic_humidity, hum_string.c_str()); // Publish humidity data

    // Print temperature and humidity to the serial monitor
    Serial.print("Temperature: ");
    Serial.println(temp);
    Serial.print("Humidity: ");
    Serial.println(humi);
  }
}
