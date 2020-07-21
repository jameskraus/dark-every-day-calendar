#include "EverydayCalendar_touch.h"
#include <Arduino.h>

typedef int16_t (*fxn_getChannelValue)(EverydayCalendar_touch * const cal_touch, uint8_t month, uint8_t day);

static const bool controllersEnabled[4] = {1,1,1,1};

// IQS550 Touch Controller configurations
static const int tcAddresses[4] = {0x74, 0x75, 0x76, 0x77};
static const int tcReadyPin[4] = {7, 5, 3, A0};
static const int tcResetPin[4] = {6, 4, 2, A1};

static const uint8_t tx_pinMappings[4][12] = {
  {0,1,2,3,4,5,11,10,9,8,7,6},
  {5,4,3,2,1,0,6,7,8,9,10,11},
  {0,1,2,3,4,5,11,10,9,8,7,6},
  {5,4,3,2,1,0,6,7,8,9,10,11},
};
static const uint8_t rx_pinMappings[4][10] = {
  {0,1,2,3,4,5,6,7,8,9},
  {9,8,7,6,5,4,3,2,1,0},
  {0,1,2,3,4,5,6,7,8,9},
  {9,8},
};

static const uint8_t tc3_enabledChannels[2][12] = {
  {1,0,1,0,1,0,1,1,0,1,0,1},
  {0,0,0,0,1,0,1,0,0,0,0,0},
};

void EverydayCalendar_touch::initialize(){
  // Initialize touch controllers, and hold in reset
  for(int i=0; i<4; i++){
    tc[i] = IQS(tcAddresses[i], tcReadyPin[i], tcResetPin[i]);
    tc[i].initialize();
    tc[i].holdReset(true);
  }
}

void EverydayCalendar_touch::configure(){
  // Initialize, if not done already
  initialize();

  // Configure touch controllers
  for(int i=0; i<4; i++){
    if(controllersEnabled[i] == false){ continue; }
    tc[i].holdReset(false); // disable reset signal
    delay(100);
    tc[i].acknowledgeReset();

    //Serial.println("Config Start");
    tc[i].setManualControl();
    tc[i].setTXPinMappings((uint8_t*)(tx_pinMappings[i]), 12);
    tc[i].setRXPinMappings((uint8_t*)(rx_pinMappings[i]), 10);
    tc[i].swapXY(true);
    tc[i].setAtiGlobalC(12);
    tc[i].setTouchMultipliers(60,30);
    tc[i].rxFloat(false); // rx pins grounded when inactive

    // Special configurations for lower touch sections
    if(i == 2){
      // Disable nonexistant Feb 29 & 30 channels
      tc[i].enableChannel(1, 8, false);
      tc[i].enableChannel(1, 9, false);
    }else if(i==3){
      // Disable day 31 on months that dont have them
      // Disable all channels in "day 32" row, except the brightness buttons
      tc[i].setTXRXChannelCount(12, 2);
      for(uint8_t rx=0; rx<2; rx++){
        for(uint8_t tx=0; tx<12; tx++){
          if(tc3_enabledChannels[rx][tx] == 0){
            tc[i].enableChannel(tx, rx, false);
          }
        }
      }
      // Special sensitivity adjustment for the moon brightness button
      tc[i].setChannel_ATI_C_Adjustment(4,1,-1);
    }

    //delay(100);
    tc[i].runAtiAlgorithm();
    delay(150);
    if(tc[i].atiErrorDetected()){
      Serial.print("ATI Error! Controller # ");
      Serial.println(i);
    }
    tc[i].reseed();
    //Serial.println("Config Done");
  }
}

void EverydayCalendar_touch::begin(){
  for(int i=0; i<4; i++){
    if(controllersEnabled[i] == false){ continue; }
    tc[i].setupComplete();
    tc[i].endSession();
  }
}

// This whole procedure should take about 100 milliseconds
bool EverydayCalendar_touch::scanForTouch(){
  // Wait until all touch controllers are ready
  for(int i=0; i<4; i++){
    if(controllersEnabled[i] == false){ continue; }
    if(tc[i].isReady() == false){
      i=0; // One of the controllers isn't ready. Start again from the top.
      continue;
    }
  }

  for(int i=0; i<4; i++){
    if(controllersEnabled[i] == false){ continue; }
    // Close session for controller
    tc[i].endSession();
    // Initiates a reading
    while(tc[i].isReady() == false);
    tc[i].readTouch();
    if(tc[i].atiErrorDetected()){
      Serial.print(" ATI Error! \tDevice Address: 0x");
      Serial.println(tcAddresses[i],16);
    }
    // Do not close the session.  Leave the controller hanging,
    // so it isn't sampling at the same time as others
  }

  x = -1;
  y = -1;

  for(int i=0; i<4; i++){
    if(controllersEnabled[i] == false){ continue; }
    if(tc[i].isTouched()){
      x = tc[i].x;
      y = tc[i].y + (10*i);
      return true;
    }
  }
  return false;
}

void _printChannelValues(EverydayCalendar_touch * const cal_touch, fxn_getChannelValue getter_fxn){
  int16_t value;
  Serial.println("\tjan\tfeb\tmar\tapr\tmay\tjun\tjul\taug\tsep\toct\tnov\tdec");

  for(int day=0; day<32; day++){
    Serial.print(day+1);
    Serial.print(":\t");
    for(int month=0; month<12; month++){
      if((month==1) && ((day==28) || (day==29))){
        Serial.print("\t");
        continue;
      }else if((day >= 30) && (tc3_enabledChannels[day%10][month]==0)){
        Serial.print("\t");
        continue;
      }
      value = getter_fxn(cal_touch, month, day);
      Serial.print(value);
      Serial.print("\t");
    }
    Serial.print("\n");
  }
}

int16_t _getAtiCAdjustmentValue(EverydayCalendar_touch * const cal_touch, uint8_t month, uint8_t day){
  int8_t value;
  if (month > 11 || day > 31){
    return 0xFFFF;
  }
  uint8_t controller = day / 10;
  uint8_t row = day % 10;
  cal_touch->tc[controller].registerRead(ATICAdjust_adr+(month*10)+(row), (uint8_t*)&value, 1);
  return (int16_t)value;
}

int16_t _getAtiCompensationValue(EverydayCalendar_touch * const cal_touch, uint8_t month, uint8_t day){
  uint8_t value;
  if (month > 11 || day > 31){
    return 0xFFFF;
  }
  uint8_t controller = day / 10;
  uint8_t row = day % 10;
  cal_touch->tc[controller].registerRead(ATICompensation_adr+(month*10)+(row), &value, 1);
  return (int16_t)value;
}

int16_t _getChannelCountValue(EverydayCalendar_touch * const cal_touch, uint8_t month, uint8_t day){
  uint16_t value;
  uint8_t bytes[2];
  if (month > 11 || day > 31){
    return 0xFFFF;
  }
  uint8_t controller = day / 10;
  uint8_t row = day % 10;
  cal_touch->tc[controller].registerRead(Counts_adr+(month*20)+(row*2), bytes, 2);
  value = (bytes[0] << 8) + bytes[1];
  return (int16_t)value;
}

void EverydayCalendar_touch::printAtiCAdjustmentValues(){
  _printChannelValues(this, _getAtiCAdjustmentValue);
}

void EverydayCalendar_touch::printAtiCompensationValues(){
  _printChannelValues(this, _getAtiCompensationValue);
}

void EverydayCalendar_touch::printChannelCountValues(){
  _printChannelValues(this, _getChannelCountValue);
}

int16_t EverydayCalendar_touch::getAtiCAdjustmentValue(uint8_t month, uint8_t day){
  return _getAtiCAdjustmentValue(this, month, day);
}
int16_t EverydayCalendar_touch::getAtiCompensationValue(uint8_t month, uint8_t day){
  return _getAtiCompensationValue(this, month, day);
}
int16_t EverydayCalendar_touch::getChannelCountValue(uint8_t month, uint8_t day){
  return _getChannelCountValue(this, month, day);
}
