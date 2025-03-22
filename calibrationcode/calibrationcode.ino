#include "HX711.h"
const int LOADCELL_DOUT_PIN = 16;const int LOADCELL_SCK_PIN = 4;
HX711 scale;
void setup() 
{  Serial.begin(9600);  
scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);  
scale.tare();  
// reset scale
}
void loop() { 
   if (scale.is_ready())
    { 
      Serial.print("HX711 reading: ");  
      Serial.println(scale.get_value(5));
    // Display 5 readings average after subtracting the tare weight from ADC
    } else
     {    Serial.println("HX711 not found.");  }  
     delay(500);  }