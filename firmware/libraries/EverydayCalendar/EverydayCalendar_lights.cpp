#include "EverydayCalendar_lights.h"
#include <Arduino.h>
#include <SPI.h>
#include <EEPROM.h>

#define UINT16_MSB(uint16) ((uint8_t)(uint16 >> 8))
#define UINT16_LSB(uint16) ((uint8_t)(uint16 & 0xFF))
#define ARRAY_SIZE(array) (sizeof((array))/sizeof((array[0])))
#define EEPROM_START_ADR  0x00000000

static const int csPin = 10;
static const int tickle_pin = 9;
static const int outputEnablePin = 8;
// set up the speed, data order and data mode
static SPISettings spiSettings(4000000, MSBFIRST, SPI_MODE0);
static const uint8_t maxBrightness = 255;
static uint8_t brightness = maxBrightness/2;

// Months are the integers in range [0,11], Days are bits within the integers, in range [0,31]
static uint32_t ledValues[12] = {0};
static uint32_t ledRenders[12] = {0};

static bool isStreakMode = false;

void EverydayCalendar_lights::configure(){
  // LED configurations
  SPI.begin();
  pinMode (csPin, OUTPUT);
  digitalWrite (csPin, HIGH);
  pinMode (tickle_pin, OUTPUT);
  digitalWrite(tickle_pin, HIGH);
  pinMode (outputEnablePin, OUTPUT);
  digitalWrite(outputEnablePin, HIGH);
  // Enable Timer2 interrupt.
  // We want CTC mode (mode 2) where timer resets after compare
  TCCR2A = (TCCR2A & ~0x03) | 0x00;
  TCCR2B = (TCCR2B & ~0x08) | 0x00;
  TCCR2B = (TCCR2B & ~0x07) | 0x02; // selects a clock prescaler of 8. That's a frequency of 31372.55
  OCR2A = brightness; //128
  clearAllLEDs();
}

void EverydayCalendar_lights::begin(){
  TIMSK2 = (1<<OCIE2A) | (1<<TOIE2); // Enable Timers
}

void EverydayCalendar_lights::clearAllLEDs(){
  memset(ledValues, 0, sizeof(ledValues));
  memset(ledRenders, 0, sizeof(ledRenders));
}

static const uint8_t monthMaxDay[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

void EverydayCalendar_lights::computeRenderedLEDs() {
  memset(ledRenders, 0, sizeof(ledRenders));
  
  // 365 + 1 day on start to make math easy
  bool dayIsStreakBorder[366];

  for (int i = 0; i < 366; i++) {
    dayIsStreakBorder[i] = false;
  }

  bool isOnStreak = false;

  int lastStreakStartDay = INT16_MAX;
  int lastStreakStartMonth = INT16_MAX;

  int dayPtr = 1;
  for (int month = 0; month <= 11; month++) {
    for (int day = 0; day <= monthMaxDay[month]-1; day++) {      
      if (isOnStreak) {
        if (!getLED(month, day)) {
          dayIsStreakBorder[dayPtr - 1] = true;
          isOnStreak = false;
        }
      } else {
        if (getLED(month, day)) {
          dayIsStreakBorder[dayPtr] = true;
          isOnStreak = true;
          lastStreakStartDay = day;
          lastStreakStartMonth = month;
        }
      }
      dayPtr++;
    }
  }

  // Fix for last day of year
  if (isOnStreak) {
    dayIsStreakBorder[365] = true;
  }

  dayPtr = 1;

  for (int month = 0; month <= 11; month++) {
    for (int day = 0; day <= monthMaxDay[month]-1; day++) {      
      bool startOrEndsStreak = dayIsStreakBorder[dayPtr];
      bool isLastStreak = month >= lastStreakStartMonth && day >= lastStreakStartDay;
      
      if (startOrEndsStreak && isLastStreak) {
        ledRenders[month] = ledRenders[month] | ((uint32_t)1 << day);
      }
      
      dayPtr++;
    }
  }
}

// Month is in range [0,11]
// Day is in range [0,30]
void EverydayCalendar_lights::setLED(uint8_t month, uint8_t day, bool enable){
  if (month > 11 || day > 30){
    return;
  }

  if (enable){
      ledValues[month] = ledValues[month] | ((uint32_t)1 << day);
  }else{
      ledValues[month] = ledValues[month] & ~((uint32_t)1 << day);
  }

  computeRenderedLEDs();
}

bool EverydayCalendar_lights::getLED(uint8_t month, uint8_t day){
   return *(ledValues+month) & ((uint32_t)1 << (day));
}

void EverydayCalendar_lights::toggleLED(uint8_t month, uint8_t day){
   bool ledState = getLED(month, day);
   setLED(month, day, !ledState);
}

void EverydayCalendar_lights::enableStreakMode() {
  isStreakMode = true;
}

void EverydayCalendar_lights::disableStreakMode() {
  isStreakMode = false;
}

void EverydayCalendar_lights::saveLedStatesToMemory(){
  for(uint8_t month=0; month<12; month++){
    int addr = EEPROM_START_ADR + (month * sizeof(uint32_t));
    EEPROM.put(addr, ledValues[month]);
  }
}

void EverydayCalendar_lights::loadLedStatesFromMemory(){
  // If the first month is completely set, then this is the first time we're running the code.
  // Clear the calendar memory
  uint32_t firstInt;
  EEPROM.get(EEPROM_START_ADR, firstInt);
  if(firstInt == 0xFFFFFFFF){
    Serial.println("First time running everyday calendar.  Clearing all days.");
    for(int i=0; i<sizeof(ledValues); i++){
      EEPROM.write(EEPROM_START_ADR + i, 0x00);
    }
  }
  // Load calendar from memory
  for(int i=0; i<sizeof(ledValues); i++){
    *((byte*)ledValues + i) = EEPROM.read(EEPROM_START_ADR + i);
  }

  for(int i=0; i<12; i++){
    Serial.print("LED Column ");
    Serial.print(i);
    Serial.print(" = ");
    Serial.println(ledValues[i]);
  }

  computeRenderedLEDs();
}

void EverydayCalendar_lights::setBrightness(uint8_t b){
  if(b > 200){
    b = 200;
  }
  b = ~b;
  if((brightness == 255) && (b != 255)){
    TIMSK2 |= (1<<OCIE2A);
  }
  brightness = b;
  if(brightness == 255){
    TIMSK2 &= ~(1<<OCIE2A);
  }
}


// Code to drive the LED multiplexing.
// This code is called at a very fast period, activating each LED column one by one
// This is done faster than the eye can perceive, and it appears that all twelve columns are illuminated at once.
ISR(TIMER2_COMPA_vect) {
  digitalWrite(outputEnablePin, LOW); // Enable
}
ISR(TIMER2_OVF_vect) {
    static uint16_t activeColumn = 0;
    static byte spiBuf[6];

    digitalWrite(outputEnablePin, HIGH); // Disable

    OCR2A = brightness;

    // "Tickle" the watchdog circuit to keep the LEDs enabled
    digitalWrite(tickle_pin, !digitalRead(tickle_pin));

    // update the next column
    uint16_t month = (1 << activeColumn);

    uint8_t * pDays = isStreakMode
      ? (uint8_t*) (ledRenders + activeColumn)
      : (uint8_t*) (ledValues + activeColumn);

    // Send the LED control values into the shift registers
    digitalWrite (csPin, LOW);
    SPI.beginTransaction(spiSettings);
    memcpy(spiBuf, &month, 2);
    spiBuf[2] = pDays[3];
    spiBuf[3] = pDays[2];
    spiBuf[4] = pDays[1];
    spiBuf[5] = pDays[0];
    SPI.transfer(spiBuf, sizeof(spiBuf));
    SPI.endTransaction();
    digitalWrite (csPin, HIGH);

    activeColumn++;
    activeColumn %= 12;
}
