#ifndef __EVERYDAYCALENDAR_TOUCH_H
#define __EVERYDAYCALENDAR_TOUCH_H
#include <stdint.h>
#include <IQS5xx.h>


class EverydayCalendar_touch
{
public:
  void initialize();
	void configure();
	void begin();
  bool scanForTouch();
  int8_t x;
  int8_t y;
  IQS tc[4];  // Touch Controllers

  int16_t getAtiCAdjustmentValue(uint8_t month, uint8_t day);
  int16_t getAtiCompensationValue(uint8_t month, uint8_t day);
  int16_t getChannelCountValue(uint8_t month, uint8_t day);

  void printAtiCAdjustmentValues();
  void printAtiCompensationValues();
  void printChannelCountValues();

};

#endif
