#include <EverydayCalendar_touch.h>

EverydayCalendar_touch cal_touch;

void setup() {
  IQS_Version version;
  Serial.begin(9600);
  Serial.println("Sketch setup started");
  delay(100);
  cal_touch.initialize();
  delay(100);
  
  cal_touch.tc[0].i2cDeviceAddr = 0x74;
  int8_t retval = cal_touch.tc[0].programFirmware(0x74);
  if(retval != 0){
    Serial.print("Programming failure. Error code: ");
    Serial.println(retval);
    return;
  }


  cal_touch.tc[0].holdReset(true);
  delay(20);
  cal_touch.tc[0].holdReset(false);
  delay(200);
  while(!cal_touch.tc[0].isReady()){};
  cal_touch.tc[0].checkVersion(&version);
  Serial.println("Programming success");
  Serial.print("Product:");
  Serial.println(version.product);
  Serial.print("Project:");
  Serial.println(version.project);
  Serial.print("Version:");
  Serial.print(version.major_version);
  Serial.print(".");
  Serial.println(version.minor_version);


}

void loop() {
  // put your main code here, to run repeatedly:
  
}
