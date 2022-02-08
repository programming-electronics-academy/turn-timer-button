#include <FastLED.h>

//pins
const byte BUTTON_PIN = 3;
const byte DATA_PIN = 6; 

// LED array
const byte NUM_LEDS = 16; 
CRGB leds[NUM_LEDS];

// Colors for setting time https://github.com/FastLED/FastLED/wiki/FastLED-HSV-Colors
const CHSV uncountedColor = CHSV(0, 255, 255); //Red
const CHSV secondsCountedColor = CHSV(160, 255, 255); //Blue
const CHSV minutesCountedColor = CHSV(96, 255, 255); // Green
const CHSV hoursCountedColor = CHSV(64, 255, 255); // Yellow

// 
const unsigned long selectorBlinkInterval = 250; // ON/OFF interval for selector LED when setting time
const unsigned long holdToFinishInterval = 2 * 1000; // How long to hold button when making final selection for time
long turnTime = 0;


// Flag to reset counter
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
void signalTimeSelected(CHSV color) {
  static uint8_t hue = 0;

  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = color;
    FastLED.show();
    fadeall();
    delay(10);
  }

}

/************************************************************
   Select Turn Time Routine
 ***********************************************************/
int selectTime(CHSV uncountedColor, CHSV countedColor) {

  int timeCounter = 0;
  boolean timeSet = false;
  boolean selectorLEDState = LOW;
  unsigned long previousMillis = 0;

  unsigned long previousButtonHoldTime = 0;

  while (!timeSet) {


    // Set all LEDs to color based on counted or uncounted
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = i > timeCounter
                ? uncountedColor
                : countedColor;
    }

    // The "selector" or "cursor" is the LED after the last counted LED.  It blinks.
    unsigned long currentMillis = millis(); // Check current time

    if (currentMillis - previousMillis >= selectorBlinkInterval) {
      previousMillis = currentMillis; // save the last time you blinked
      selectorLEDState = ! selectorLEDState; // switch the LED state flag
    }

    // Change the selector LED to Red
    // (note we do not need to set Blue, because the for loop above takes care of that)
    if (selectorLEDState == LOW) {
      leds[timeCounter] = CHSV(0, 255, 255);
    }

    FastLED.show();

    // Increment count when button pressed
    if (buttonPressed) {

      buttonPressed = false;
      timeCounter++;

      Serial.print("Time Counter = ");
      Serial.println(timeCounter);

      //Rollover timeCounter if max reached
      if (timeCounter >= NUM_LEDS) {
        timeCounter = 0;
      }

    }

    // Exit while loop on long button hold
    if (!digitalRead(BUTTON_PIN)) {
      unsigned long buttonHoldTime = millis() - previousButtonHoldTime;

      Serial.print("buttonHoldTime = ");
      Serial.println(buttonHoldTime);

      Serial.print("previousButtonHoldTime = ");
      Serial.println(previousButtonHoldTime);

      if (buttonHoldTime > holdToFinishInterval) {
        timeSet = true;

        //Display cylon effect to show done
        for (int i = 0; i < 10; i++) {
          signalTimeSelected(countedColor);
        }

        buttonPressed = false;
      }

    } else {
      previousButtonHoldTime = millis();
    }

  }

  return timeCounter; //Returns the number of times the button was pressed (less the long hold)
}


/************************************************************
   Computer Turn Time
 ***********************************************************/
long computeTurnTime(int s, int m, int h) {

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

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(84);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(BUTTON_PIN, buttonPress, RISING);

  long secondsCount = selectTime(uncountedColor, secondsCountedColor);
  long minutesCount = selectTime(uncountedColor, minutesCountedColor);
  long hoursCount = selectTime(uncountedColor, hoursCountedColor);

  turnTime = computeTurnTime(secondsCount, minutesCount, hoursCount);
}

void loop() {

  boolean overTime = false;

  // As turn time elapses, show a fade from Green to Blue to Red
  for (int hue = 85; hue <= 255; hue++) {
    changeAllColorTo(hue);
    FastLED.show();
    delay(turnTime / 170);

    // If button flag was set in ISR then exit
    if (buttonPressed) {
      buttonPressed = false; // Reset button flag
      break;
    }

    // If button is not pressed before turn ends, LEDs go into "overtime" mode
    if (hue == 255) {
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
