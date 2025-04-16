#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <HX711.h>

// Pin definitions
#define RED_LED   22 
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
float calibration_factor = -414.687;
float duration_us, distance_cm;
int gasValue = 350;
const char* ssid = "Emre";
const char* password = "Surfyapiyom31.";

AsyncWebServer server(80);

// HTML content
const char* htmlContent = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>SMART WASTE SYSTEM</title>
  <link rel="stylesheet" href="/design.css">
  <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.5.1/css/all.min.css">
</head>
<body>
  <div class="container">
    <h1>SMART WASTE ATU</h1>
    <div class="sensor-data">
      <div class="data" id="distance">Distance: -- cm</div>
      <div class="data" id="weight">Weight: -- g</div>
      <div class="data" id="gas">Gas Value: --</div>
    </div>
    <div class="status">
      <div id="statusMessage" class="status-message">Waiting for data...</div>
    </div>
    <div class="status">
      <div id="gasStatusMessage" class="status-message">Waiting for data...</div>
    </div>
    <div>
      <div id="weightStatusMessage" class="status-message">Waiting for data...</div>
    </div>
  </div>
  <script>
    setInterval(() => {
      fetch('/getSensorData')
        .then(response => response.json())
        .then(data => {
          document.getElementById('distance').innerHTML = `<i class="fa-solid fa-arrows-up-down"></i>   Distance: ${data.distance} cm`;
          document.getElementById('weight').innerHTML = `<i class="fa-solid fa-weight-hanging"></i>   Weight: ${data.weight} g`;
          document.getElementById('gas').innerHTML = `<i class="fa-solid fa-fire"></i>   Gas Value: ${data.gasValue}`;
          
          // Update status message based on sensor data
          let statusMessage = '';
          if (data.distance < 10) {
            statusMessage = 'Almost Full!';
            document.getElementById('statusMessage').style.backgroundColor = '#ff6666'; // Red
          } else if (data.distance < 50) {
            statusMessage = 'Bin %70 FULL';
            document.getElementById('statusMessage').style.backgroundColor = '#ffcc00'; // Yellow
          } else {
            statusMessage = 'Bin is Empty';
            document.getElementById('statusMessage').style.backgroundColor = '#66cc66'; // Green
          }
          
          document.getElementById('statusMessage').innerText = statusMessage;


        let gasStatusMessage = '';
          if (data.gasValue < 300) {
            gasStatusMessage = 'Gas is detected';
            document.getElementById('gasStatusMessage').style.backgroundColor = '#ff6666'; // Red
          }else {
            gasStatusMessage = 'No gas in the bin';
            document.getElementById('gasStatusMessage').style.backgroundColor = '#66cc66'; // Green
          }
          
          document.getElementById('gasStatusMessage').innerText = gasStatusMessage;

          let weightStatusMessage = '';
          if (data.weight > 100) {
            weightStatusMessage = 'Bin is too HEAVY';
            document.getElementById('weightStatusMessage').style.backgroundColor = '#ff6666'; // Red
          }else {
            weightStatusMessage = 'Bin is light.';
            document.getElementById('weightStatusMessage').style.backgroundColor = '#66cc66'; // Green
          }
          
          document.getElementById('weightStatusMessage').innerText = weightStatusMessage;


        });
    }, 1000); // Update every second
  </script>
</body>
</html>
)rawliteral";

//CSS content
const char* cssContent = R"rawliteral(
body {
  font-family: 'Arial', sans-serif;
  background-color: #f1f1f1;
  color: #333;
  margin: 0;
  padding: 20px;
}
.container {
  text-align: center;
  max-width: 900px;
  margin: auto;
  padding: 20px;
  background-color: #ffffff;
  border-radius: 8px;
  box-shadow: 0px 0px 15px rgba(0, 0, 0, 0.1);
}
h1 {
  color: #4CAF50;
  font-size: 36px;
  margin-bottom: 20px;
}
.sensor-data {
  margin: 20px 0;
}
.data {
  font-size: 24px;
  margin: 15px 0;
  padding: 10px;
  background-color: #f9f9f9;
  border-radius: 5px;
  box-shadow: 0 2px 5px rgba(0, 0, 0, 0.1);
}

/* Icon styles */
.data i {
  margin-right: 10px; 
  font-size: 24px; 
  color: #007bff; 
}

/* Change icon colors based on type */
#distance i {
  color: #28a745; /* Green */
}

#weight i {
  color: #ff9800; /* Orange */
}

#gas i {
  color: #e63946; /* Red */
}

.status-message {
  font-size: 22px;
  padding: 15px;
  color: #ffffff;
  margin-top: 20px;
  border-radius: 5px;
  box-shadow: 0 2px 5px rgba(0, 0, 0, 0.1);
  transition: background-color 0.3s ease;
}
)rawliteral";

void setup() {
  Serial.begin(115200);

  // Setup pins
  pinMode(TRIG_PIN, OUTPUT); 
  pinMode(ECHO_PIN, INPUT);  
  pinMode(RED_LED, OUTPUT);
  pinMode(YEL_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Initialize WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());  // printing IP address

  // Initialize HX711
  scale.begin(DT_PIN, SCK_PIN); 
  scale.set_scale(calibration_factor);
  scale.tare();

  // Serving the HTML page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", htmlContent);
  });

  // Serving the CSS file
  server.on("/design.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/css", cssContent);
  });

  // API to get sensor data (distance, weight, gas)
  server.on("/getSensorData", HTTP_GET, [](AsyncWebServerRequest *request){
    String sensorData = "{\"distance\": " + String(distance_cm) + ", \"weight\": " + String(weight) + ", \"gasValue\": " + String(gasValue) + "}";
    request->send(200, "application/json", sensorData);
  });

  // Start the server
  server.begin();
}

void loop() {
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

  // Realtime LED and Buzzer reactions
  if (distance_cm < 10) {
    digitalWrite(RED_LED, HIGH);   // Turn on red LED
    digitalWrite(YEL_LED, LOW);    // Turn off yellow LED
    digitalWrite(BUZZER_PIN, HIGH); // Turn on buzzer
    delay(20);
    digitalWrite(RED_LED, LOW);
    delay(100);

  } else if (distance_cm < 50) {
    digitalWrite(RED_LED, HIGH);   // Turn on red LED
    digitalWrite(YEL_LED, LOW);   // Turn on yellow LED
    digitalWrite(BUZZER_PIN, LOW);// Turn off buzzer
    delay(100); 
  } else {
    digitalWrite(RED_LED, LOW);    // Turn off red LED
    digitalWrite(YEL_LED, HIGH);   // Turn on yellow LED
    digitalWrite(BUZZER_PIN, LOW);// Turn off buzzer
    delay(100); 
  }

  if (gasValue > 580) {
    Serial.println("Gas is Present");
  } else {
    Serial.println("Gas is not Present");
  }

  if (weight > weightThreshold) {
    Serial.println("Weight limit exceeded!");
    digitalWrite(BUZZER_PIN, HIGH); // Turn on buzzer if weight exceeds limit
  } else {
    digitalWrite(BUZZER_PIN, LOW); // Turn off buzzer if weight is normal
  }

  // Output data 
  Serial.print("Distance: ");
  Serial.print(distance_cm);
  Serial.print(" cm, Weight: ");
  Serial.print(weight);
  Serial.print(" g, Gas Value: ");
  Serial.println(gasValue);

   // Short delay 
}

