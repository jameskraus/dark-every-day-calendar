#include <EverydayCalendar_lights.h>

EverydayCalendar_lights cal_lights;

void setup() {
  Serial.begin(9600);
  Serial.println("Sketch setup started");
  
  // Initialize LED functionality
  cal_lights.configure();
  cal_lights.setBrightness(200);
  cal_lights.begin();

  static const uint8_t monthMaxDay[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  for(int month = 0; month <12; month++){
    for(int day=0; day< monthMaxDay[month]; day++){
      cal_lights.setLED(month, day, true);
    }
  }
  
  Serial.println("Sketch setup complete");
}

void loop() {
  // put your main code here, to run repeatedly:

}
