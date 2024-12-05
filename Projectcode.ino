#include "HX711.h" //Including HX711 library for weight measurements


//Leds
#define RED_LED   26 // ESP32 pin 26 connected to LED's pin
#define YEL_LED   25 // ESP32 pin 25 connected to LED's pin

//Buzzer
#define BUZZER_PIN 27 // ESP32 pin 27 connected to BUZZER's pin

//Ultrasonic Sensor Definitions
#define TRIG_PIN  12 // ESP32 pin 12 connected to Ultrasonic Sensor's TRIG pin
#define ECHO_PIN  14 // ESP32 pin 14 connected to Ultrasonic Sensor's ECHO pin
#define DISTANCE_THRESHOLD 50
#define almostfull 10  // centimeters

// variables will change:
float duration_us, distance_cm;

//Gas Sensor Definitions
#define MQ2_PIN 33  // 33 Pin connected to gas sensor
int thresHold = 580;

//Load Cell Definitions
#define DT_PIN 4   // Pin connected to HX711  DT
#define SCK_PIN 5  // Pin connected to HX711  SCK
HX711 scale;
float weight = 0;               // Variable to store the weight
float weightThreshold = 1000;   // Weight threshold in grams
float calibration_factor = -7500;


void setup() {
  Serial.begin (9600);       // initialize serial port

  // Setup Ultrasonic Sensor and Leds 
  pinMode(TRIG_PIN, OUTPUT); // set ESP32 pin to output mode
  pinMode(ECHO_PIN, INPUT);  // set ESP32 pin to input mode
  pinMode(RED_LED, OUTPUT);
  pinMode(YEL_LED, OUTPUT);  // set ESP32 pin to output mode

  // Setup Buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // Setup HX711
  scale.begin(DT_PIN, SCK_PIN); // Initialize HX711 with the defined pins
  scale.set_scale(calibration_factor);            // Set scale 
  scale.tare();                 // Reset the scale to zero

 
  Serial.println("Warming up the MQ2 sensor\n");
  delay(20000); //Delay for warming up the mq2 sensor
}

void loop() {
  // generate pulse to TRIG pin
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  //Getting the gas value
  int gasValue = analogRead(MQ2_PIN);

    if (scale.is_ready()) {
      weight = scale.get_units(10); // Get the average of 10 readings for stability
    } else {
        Serial.println("HX711 not found. Check wiring.");
        weight = -1; // Error value
    }

    // measure duration of pulse from ECHO pin
    duration_us = pulseIn(ECHO_PIN, HIGH);
    // calculate the distance
    distance_cm = 0.017 * duration_us;

  if (distance_cm < DISTANCE_THRESHOLD)
  {

    if(distance_cm < almostfull)
    {
  
      digitalWrite(RED_LED, HIGH);
      delay(100);
      digitalWrite(RED_LED, LOW);
      delay(100);
      digitalWrite(RED_LED, HIGH);
    
    }

    digitalWrite(RED_LED, HIGH);
    digitalWrite(YEL_LED, LOW);
    
  } 
 
  else
  {
    digitalWrite(RED_LED, LOW);
    digitalWrite(YEL_LED, HIGH);
  }

  if(gasValue > thresHold)
  {
    Serial.println("Gas is Present\n");
  }
  else
  {
    Serial.println("Gas is not present\n");
  }

  // Handle weight threshold logic
  if (weight > weightThreshold) {
    Serial.println("Weight limit exceeded!");
    digitalWrite(BUZZER_PIN, HIGH); // Turn on buzzer
  }else{
    digitalWrite(BUZZER_PIN, LOW); // Turn on buzzer
  }

  Serial.println("MQ2 sensor AO value: \n");
  Serial.println(gasValue);
   

  // print the value to Serial Monitor
  Serial.print("distance: ");
  Serial.print(distance_cm);
  Serial.println(" cm");

  Serial.print("Weight: ");
  Serial.print(weight);
  Serial.println(" grams");

  delay(500);
}