#include <WiFi.h>
#include <WebServer.h>
#include <HX711.h>
#include "SPIFFS.h"

// Pin definitions
#define RED_LED   16 
#define YEL_LED   23 
#define BUZZER_PIN 15
#define TRIG_PIN  12
#define ECHO_PIN  14
#define MQ2_PIN 33
#define DT_PIN 4   
#define SCK_PIN 5  

// Constants and variables
HX711 scale;
float weight = 0;               
float weightThreshold = 1000;   
float calibration_factor = -7500;
float duration_us, distance_cm;
int gasValue = 0;
const char* ssid = "Emre";  // Set your SSID
const char* password = "Surfyapiyom31.";  // Set your password

// Create a WebServer object on port 80
WebServer server(80);

// Define the request handlers
void handleRoot() {
  File file = SPIFFS.open("/smartw.html", "r");
  if (!file) {
    Serial.println("Error opening smartw.html");
    server.send(404, "text/plain", "File not found");
    return;
  }
  Serial.println("smartw.html served");
  server.send(file, "text/html");
  file.close();
}

void handleCSS() {
  File file = SPIFFS.open("/design.css", "r");
  if (!file) {
    Serial.println("Error opening design.css");
    server.send(404, "text/plain", "File not found");
    return;
  }
  Serial.println("design.css served");
  server.send(file, "text/css");
  file.close();
}

void handleSensorData() {
  String sensorData = "{\"distance\": " + String(distance_cm) + ", \"weight\": " + String(weight) + ", \"gasValue\": " + String(gasValue) + "}";
  server.send(200, "application/json", sensorData);
}

void setup() {
  Serial.begin(115200);

  // Setup pins
  pinMode(TRIG_PIN, OUTPUT); 
  pinMode(ECHO_PIN, INPUT);  
  pinMode(RED_LED, OUTPUT);
  pinMode(YEL_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An error occurred while mounting SPIFFS.");
    return;
  } else {
    Serial.println("SPIFFS mounted successfully.");
  }

  // Initialize WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());  // This prints the IP address

  // Initialize HX711
  scale.begin(DT_PIN, SCK_PIN); 
  scale.set_scale(calibration_factor);
  scale.tare();

  // Define routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/design.css", HTTP_GET, handleCSS);
  server.on("/getSensorData", HTTP_GET, handleSensorData);

  // Start the server
  server.begin();
}

unsigned long previousMillis = 0;  // Stores the last time the data was updated
const long interval = 30000;  // Interval to update data (30 seconds)

void loop() {
  unsigned long currentMillis = millis();  // Get the current time

  // Check if 30 seconds have passed
  if (currentMillis - previousMillis >= interval) {
    // Save the last time the data was updated
    previousMillis = currentMillis;

    // Ultrasonic Sensor (distance measurement)
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    duration_us = pulseIn(ECHO_PIN, HIGH);
    distance_cm = 0.017 * duration_us;

    // Gas Sensor reading
    gasValue = analogRead(MQ2_PIN);

    // HX711 load cell reading (weight)
    if (scale.is_ready()) {
      weight = scale.get_units(10);  // Get average of 10 readings
    }

    // Logic for LED and Buzzer
    if (distance_cm < 10) {
      digitalWrite(RED_LED, HIGH);
      digitalWrite(BUZZER_PIN, HIGH);
      digitalWrite(YEL_LED, LOW);
    } else if (distance_cm < 50) {
      digitalWrite(RED_LED, HIGH);
      digitalWrite(YEL_LED, LOW);
    } else {
      digitalWrite(RED_LED, LOW);
      digitalWrite(YEL_LED, HIGH);
    }

    if (gasValue > 580) {
      Serial.println("Gas is Present");
    } else {
      Serial.println("Gas is not Present");
    }

    if (weight > weightThreshold) {
      Serial.println("Weight limit exceeded!");
      digitalWrite(BUZZER_PIN, HIGH);
    } else {
      digitalWrite(BUZZER_PIN, LOW);
    }

    // Output data to Serial Monitor for debugging
    Serial.print("Distance: ");
    Serial.print(distance_cm);
    Serial.print(" cm, Weight: ");
    Serial.print(weight);
    Serial.print(" g, Gas Value: ");
    Serial.println(gasValue);
  }

  // Handle client requests
  server.handleClient();
}
