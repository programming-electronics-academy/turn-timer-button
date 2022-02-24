#include <FastLED.h>

//pins
const byte BUTTON_PIN = 3;
const byte DATA_PIN = 6;

// LED array
const byte NUM_LEDS = 16;
CRGB leds[NUM_LEDS];

// Colors used when setting time.  Color ref: https://github.com/FastLED/FastLED/wiki/FastLED-HSV-Colors
const CHSV UNCOUNTED_COLOR = CHSV(0, 255, 255); //Red
const CHSV SECONDS_COUNTED_COLOR = CHSV(160, 255, 255); //Blue
const CHSV MINUTES_COUNTED_COLOR = CHSV(96, 255, 255); // Green
const CHSV HOURS_COUNTED_COLOR = CHSV(64, 255, 255); // Yellow

// Colors / "Hue" used durring each turn (0-255)
const int START_HUE = 85; //Green(ish) MUST be a number < END_HUE
const int END_HUE = 255; //Red
const int HUE_INCREMENT = END_HUE - START_HUE; // Used to change color based on selcted turn time

// Timing settings
const unsigned long HOLD_TO_FINISH_INTERVAL = 2 * 1000; // How long to hold button when making final selection for time
long turnTime = 0; // Manages the turnTime - determined in setup(), used in loop()

// Flag set in ISR to indicate a button press
volatile boolean buttonPressed = false;


/************************************************************
   ISR: Actions to take on button press
 ***********************************************************/
void ARDUINO_ISR_ATTR buttonPress() {

  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  // If interrupts come faster than 200ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 200)
  {
    buttonPressed = true; //flag that button was pressed
  }
  last_interrupt_time = interrupt_time;
}

/************************************************************
   LED Effect: Set all LEDs same color
 ***********************************************************/
void changeAllColorTo(int h, int s = 255, int v = 255) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(h, s, v);
  }
}

/************************************************************
   LED Effect: Blink Red
 ***********************************************************/
void blinkRed() {

  changeAllColorTo(255);
  FastLED.show();
  delay(300);

  changeAllColorTo(0, 0, 0);
  FastLED.show();
  delay(300);
}

/************************************************************
   LED Effect: Fade All
 ***********************************************************/
void fadeall() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].nscale8(100);
  }
}

/************************************************************
   LED Effect: Half Cyclon
 ***********************************************************/
void halfCylon(CHSV color) {
  static uint8_t hue = 0;

  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = color;
    FastLED.show();
    fadeall();
    delay(10);
  }
}

/************************************************************
   Signal a time unit has been selected
 ***********************************************************/
void signalTimeSelected(CHSV color) {
  for (int i = 0; i < 10; i++) {
    halfCylon(color);
  }
}

/************************************************************
   Select Turn Time Routine
 ***********************************************************/
int selectTime(CHSV uncountedColor, CHSV countedColor) {

  int timeCounter = 0; // This tracks the button presses, each button press is a time unit
  unsigned long previousButtonHoldTime = 0; // Used for determining long button hold time
  boolean update = true; 

  while (true) {

    if (update) {
      // Set color of each LED based on counted or uncounted
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = i < timeCounter
                  ? countedColor
                  : uncountedColor;
      }

      FastLED.show();
      update = false;
    }

    // Increment count when button pressed
    if (buttonPressed) {

      buttonPressed = false; // Reset ISR button flag
      timeCounter++;
      update = true;
      
      //Rollover timeCounter if max reached
      if (timeCounter >= NUM_LEDS) {
        timeCounter = 0;
      }
    }

    // Check if button held
    if (!digitalRead(BUTTON_PIN)) {

      // Exit while if button has been held "long" time
      if (millis() - previousButtonHoldTime > HOLD_TO_FINISH_INTERVAL) {

        signalTimeSelected(countedColor); //Display cylon effect to show selection has been made
        buttonPressed = false; // reset ISR button flag
        break;
      }
      
    } else {
      previousButtonHoldTime = millis();
    }
  }

  return timeCounter; //Returns the number of times the button was pressed (less the long hold)
}


/************************************************************
   Compute Turn Time
 ***********************************************************/
long computeTurnTime(int s = 0, int m = 0, int h = 0) {

  s = s * 5 * 1000; // 5 seconds for every count

  if ( m <= 11 ) {
    m = m * 60 * 1000; // 1 minute for every count <= 10
  } else {
    m = m * 5 * 60 * 1000; // 5 min for every count over 10
  }

  h = h * 60 * 60 * 1000; // 1 hour for every count

  return s + m + h; // in milliseconds
}

void setup() {
  Serial.begin(115200);

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS); //adds the
  FastLED.setBrightness(84);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(BUTTON_PIN, buttonPress, RISING);

  // Set turn time.  Select seconds, then minutes.
  long secondsCount = selectTime(UNCOUNTED_COLOR, SECONDS_COUNTED_COLOR);
  long minutesCount = selectTime(UNCOUNTED_COLOR, MINUTES_COUNTED_COLOR);
  //long hoursCount = selectTime(UNCOUNTED_COLOR, HOURS_COUNTED_COLOR); //Add this is you have REALLY long turns

  turnTime = computeTurnTime(secondsCount, minutesCount);
}

void loop() {

  boolean overTime = false;

  // As turn time elapses, show a fade from Green to Blue to Red
  for (int hue = START_HUE; hue <= END_HUE; hue++) {
    changeAllColorTo(hue);
    FastLED.show();
    delay(turnTime / HUE_INCREMENT);

    // If button flag was set in ISR then exit
    if (buttonPressed) {
      buttonPressed = false; // Reset button flag
      break;
    }

    // If button is not pressed before turn ends, LEDs go into "overtime" mode
    if (hue == END_HUE) {
      overTime = true;
    }
  }

  // Over Time Mode, All LEDs blink
  while (overTime) {

    blinkRed();

    // If button flag was set in ISR then exit
    if (buttonPressed) {
      buttonPressed = false;
      break;
    }
  }
} // End loop()
