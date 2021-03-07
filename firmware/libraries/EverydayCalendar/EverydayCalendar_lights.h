#ifndef __EVERYDAYCALENDAR_LIGHTS_H
#define __EVERYDAYCALENDAR_LIGHTS_H
#include <stdint.h>


class EverydayCalendar_lights
{
private:
	void computeRenderedLEDs();
	bool getLED(uint8_t month, uint8_t day);
public:
	void configure();
	void begin();
	void loadLedStatesFromMemory();
	void saveLedStatesToMemory();
	void enableStreakMode();
	void disableStreakMode();
	void setLED(uint8_t month, uint8_t day, bool enable);
	void toggleLED(uint8_t month, uint8_t day);
	void clearAllLEDs();
	void setBrightness(uint8_t brightness);

};

#endif
