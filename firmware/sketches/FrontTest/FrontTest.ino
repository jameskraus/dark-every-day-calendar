#include <EverydayCalendar_lights.h>
#include <EverydayCalendar_touch.h>

EverydayCalendar_touch cal_touch;
EverydayCalendar_lights cal_lights;

static const uint8_t monthMaxDay[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static const int16_t channelCountTarget = 500;  
static const int16_t channelCountTolerableDeviation = 50;  

void setup() {
  IQS_Version version;
  Serial.begin(9600);
//  Serial.begin(19200);/
  Serial.println("Sketch setup started");
  delay(100);
  cal_touch.initialize();
  delay(100);
  
  for(uint8_t i=0; i<4; i++){
    if(programTouchController(i) == 0){
      Serial.print("Programmed touch controller #");
      Serial.println(i);
      cal_touch.tc[i].holdReset(true);
      delay(15);
    }
  }

  for(uint8_t i=0; i<4; i++){
    cal_touch.tc[i].holdReset(false);
  }
  delay(20);

  // Initialize LED functionality
  cal_lights.configure();
  cal_lights.setBrightness(100);
  cal_lights.begin();

  cal_touch.configure();
  cal_touch.begin();
  delay(100);
  
  cal_touch.printChannelCountValues();
  Serial.println("\n");
  cal_touch.printAtiCompensationValues();
  Serial.println("\n");

  cal_touch.scanForTouch();
  
  for(int month=0; month<12; month++){
    for(int day=0; day<monthMaxDay[month]; day++){
      for(int retry=0; retry<3; retry++){
        int16_t channelCount = cal_touch.getChannelCountValue(month, day);
        int16_t difference = channelCount - channelCountTarget;
        uint16_t deviation = abs(difference);
        if(deviation < channelCountTolerableDeviation){
          cal_lights.setLED(month, day, true);
          break;
        }
        cal_touch.scanForTouch(); // re-scan
        Serial.print("Retry # ");
        Serial.print(retry);
        Serial.print("\t for month: ");
        Serial.print(month);
        Serial.print("\t day: ");
        Serial.print(day);
        Serial.print("\t count: ");
        Serial.print(channelCount);
        Serial.print("\n");
      }
    }
  }
  
    
  
  Serial.println("Sketch setup complete");
}

void loop() {
  // put your main code here, to run repeatedly:

}

uint8_t programTouchController(uint8_t c){
  static const uint8_t targetAddresses[] = {0x74, 0x75, 0x76, 0x77};
  int8_t retval[4];
  for(uint8_t addy=0x74; addy<=0x77; addy++){
    cal_touch.tc[c].i2cDeviceAddr = addy;
    retval[c] = cal_touch.tc[c].programFirmware(targetAddresses[c]);
    if( retval[c] == 0){
      return 0;
    }
  }
  return retval[0];
}
