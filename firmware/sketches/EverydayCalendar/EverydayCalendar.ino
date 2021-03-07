#include <EverydayCalendar_lights.h>
#include <EverydayCalendar_touch.h>

typedef struct {
  int8_t    x;
  int8_t    y;
} Point;

EverydayCalendar_touch cal_touch;
EverydayCalendar_lights cal_lights;
int16_t brightness = 128;

void setup() {
  Serial.begin(9600);
  Serial.println("Sketch setup started");

  // Initialize LED functionality
  cal_lights.configure();
  cal_lights.setBrightness(200);
  cal_lights.begin();

  // Perform startup animation
  honeyDrip();

  // Fade out
  for (int b = 200; b >= 0; b--) {
    cal_lights.setBrightness(b);
    delay(4);
  }

  // Initialize touch functionality
  cal_touch.configure();
  cal_touch.begin();
  cal_lights.loadLedStatesFromMemory();
  delay(1500);

  // Fade in
  for (int b = 0; b <= brightness; b++) {
    cal_lights.setBrightness(b);
    delay(4);
  }

  Serial.println("Sketch setup complete");
}

bool isDarkModeEnabled = false;

bool isTransitioningIntoDarkMode = false;
bool isTransitioningOutOfDarkMode = false;

int16_t brightnessBeforeDarkMode = brightness;
long darkModeCounter = 0;
long darkModeTransitionCounter = 0;

const int16_t DARK_MODE_BRIGHTNESS = 2;

const float DARK_MODE_TRANSITION_SECONDS = 2;
const float DARK_MODE_SECONDS = 60 * 60 * 8;

const long DARK_MODE_MAX_COUNT = (long) (DARK_MODE_SECONDS / (30.0 * 1e-3));
const long DARK_MODE_TRANSITION_COUNT = (long) (DARK_MODE_TRANSITION_SECONDS / (30.0 * 1e-3));

bool isRecentlyTouched = false;
long recentTouchCounter = 0;
const float RECENT_TOUCH_SECONDS = 3;
const long RECENT_TOUCH_COUNT = (long) (RECENT_TOUCH_SECONDS / (30.0 * 1e-3));

void toggleDarkMode(EverydayCalendar_touch touch) {
  if (
    !isDarkModeEnabled
    && !isTransitioningIntoDarkMode
    && !isTransitioningOutOfDarkMode
  ) {
    startTransitionIntoDarkMode();
  }

  if (isDarkModeEnabled) {
    startRecentTouch();
  }
}

void toggleDayAndSave(EverydayCalendar_touch touch) {
  cal_lights.toggleLED((uint8_t)touch.x, (uint8_t)touch.y);
  Serial.print("x: ");
  Serial.print(touch.x);
  Serial.print("\ty: ");
  Serial.println(touch.y);

  if (!isBrightnessButton(touch)) {
    toggleDarkMode(touch);
  }

  cal_lights.saveLedStatesToMemory();
}

void startTransitionIntoDarkMode() {
  Serial.println("Transitioning into dark mode");
  isTransitioningIntoDarkMode = true;
  brightnessBeforeDarkMode = brightness;
}

void finishTransitionIntoDarkMode() {
  Serial.println("Finishing transition into dark mode");
  isDarkModeEnabled = true;
  cal_lights.enableStreakMode();

  darkModeTransitionCounter = 0;
  isTransitioningIntoDarkMode = false;
}

void startRecentTouch() {
  Serial.println("Starting recent touch mode");
  isRecentlyTouched = true;
  recentTouchCounter = 0;
  cal_lights.disableStreakMode();
}

void endRecentTouch() {
  Serial.println("Ending recent touch mode");
  isRecentlyTouched = false;
  recentTouchCounter = 0;
  cal_lights.enableStreakMode();
}

void startTransitionOutOfDarkMode() {
  // Just in case there was a touch right before the transition
  endRecentTouch();

  Serial.println("Transitioning out of dark mode");
  isDarkModeEnabled = false;
  darkModeCounter = 0;
  cal_lights.disableStreakMode();

  isTransitioningOutOfDarkMode = true;
}

void finishTransitionOutOfDarkMode() {
  Serial.println("Finishing transition out of dark mode");
  isTransitioningOutOfDarkMode = false;
  darkModeTransitionCounter = 0;
}

void resetDarkModeStuff() {
  Serial.println("Resetting stuff related to dark mode");

  isDarkModeEnabled = false;
  isTransitioningIntoDarkMode = false;
  isTransitioningOutOfDarkMode = false;

  darkModeCounter = 0;
  darkModeTransitionCounter = 0;

  isRecentlyTouched = false;
  recentTouchCounter = 0;

  cal_lights.disableStreakMode();
  setBrightness(brightnessBeforeDarkMode);
}

void setTransitionBrightness(int16_t b, float percentTransitioned) {
  setBrightness(constrain(b * (percentTransitioned * percentTransitioned) + 1, 0, 200));
}

void darkModeLoop() {
  if (isTransitioningIntoDarkMode) {
    if (darkModeTransitionCounter < DARK_MODE_TRANSITION_COUNT) {
      darkModeTransitionCounter++;

      float percentTransitioned = (darkModeTransitionCounter * 1.0) / (DARK_MODE_TRANSITION_COUNT * 1.0);
      setTransitionBrightness(brightnessBeforeDarkMode, (1 - percentTransitioned));
    } else {
      finishTransitionIntoDarkMode();
    }
  }

  if (isTransitioningOutOfDarkMode) {
    if (darkModeTransitionCounter < DARK_MODE_TRANSITION_COUNT) {
      darkModeTransitionCounter++;

      float percentTransitioned = (darkModeTransitionCounter * 1.0) / (DARK_MODE_TRANSITION_COUNT * 1.0);
      setTransitionBrightness(brightnessBeforeDarkMode, percentTransitioned);
    } else {
      finishTransitionOutOfDarkMode();
    }
  }

  if (isDarkModeEnabled) {
    if (darkModeCounter < DARK_MODE_MAX_COUNT) {
      darkModeCounter++;
    } else {
      startTransitionOutOfDarkMode();
    }
  }

  if (isRecentlyTouched) {
    if (recentTouchCounter < RECENT_TOUCH_COUNT) {
      recentTouchCounter++;
    } else {
      endRecentTouch();
    }
  }
}

void setBrightness(int16_t b) {
  brightness = constrain(b, 0, 200);
  Serial.print("Brightness: ");
  Serial.println(brightness);
  cal_lights.setBrightness((uint8_t)brightness);
}

bool isBrightnessButton(EverydayCalendar_touch touch) {
  return cal_touch.y == 31;
}

bool isDecreaseBrightness(EverydayCalendar_touch touch) {
  return isBrightnessButton(touch) && cal_touch.x == 4;
}


bool isIncreaseBrightness(EverydayCalendar_touch touch) {
  return isBrightnessButton(touch) && cal_touch.x == 6;
}

void loop() {
  static Point previouslyHeldButton = {0xFF, 0xFF}; // 0xFF and 0xFF if no button is held
  static uint16_t touchCount = 1;
  static const uint8_t debounceCount = 3;
  static const uint16_t clearCalendarCount = 1300; // ~40 seconds.  This is in units of touch sampling interval ~= 30ms.
  Point buttonPressed = {0xFF, 0xFF};
  bool touch = cal_touch.scanForTouch();

  darkModeLoop();

  // Handle a button press
  if (touch)
  {
    // Brightness Buttons
    if (isBrightnessButton(cal_touch)) {
      if (isDecreaseBrightness(cal_touch)) {
        brightness -= 3;
      } else if (isIncreaseBrightness(cal_touch)) {
        brightness += 2;
      }

      if (isDarkModeEnabled) {
        resetDarkModeStuff();
      }

      setBrightness(brightness);
    }
    // If all buttons aren't touched, reset debounce touch counter
    if (previouslyHeldButton.x == -1) {
      touchCount = 0;
    }

    // If this button is been held, or it's just starting to be pressed and is the only button being touched
    if (((previouslyHeldButton.x == cal_touch.x) && (previouslyHeldButton.y == cal_touch.y))
        || (debounceCount == 0))
    {
      // The button has been held for a certain number of consecutive checks
      // This is called debouncing
      if (touchCount == debounceCount) {
        toggleDayAndSave(cal_touch);
      }

      // Check if the special "Reset" January 1 button is being held
      if ((cal_touch.x == 0) && (cal_touch.y == 0) && (touchCount == clearCalendarCount)) {
        Serial.println("Resetting all LED states");
        resetDarkModeStuff();
        clearAnimation();
      }

      if (touchCount < 65535) {
        touchCount++;
        Serial.println(touchCount);
      }
    }
  }

  previouslyHeldButton.x = cal_touch.x;
  previouslyHeldButton.y = cal_touch.y;
}

void honeyDrip() {
  uint16_t interval_ms = 10;
  static const uint8_t monthDayOffset[12] = {0, 3, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0};
  // Turn on all LEDs one by one in the span of a few second
  for (int day = 0; day < 31; day++) {
    for (int month = 0; month < 12; month++) {
      int8_t adjustedDay = day - monthDayOffset[month];
      if (adjustedDay >= 0 ) {
        cal_lights.setLED(month, adjustedDay, true);
      }
    }
    delay(interval_ms);
    interval_ms = interval_ms + 2;
  }
}

void clearAnimation() {
  uint16_t interval_ms = 25;
  static const uint8_t monthMaxDay[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  for (int month = 11; month >= 0; month--) {
    for (int day = monthMaxDay[month] - 1; day >= 0; day--) {
      cal_lights.setLED(month, day, false);
      delay(interval_ms);
    }
  }
  cal_lights.saveLedStatesToMemory();
}
