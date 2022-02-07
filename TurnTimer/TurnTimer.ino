/*
   TODO

   Switch to turn ON/OFF

   Mode for setting time.

   Way to set turn time...?
    Hmm... ESP32 webserver?
    Button press

   Button press starts turn.

   When turn starts, smooth fade from Green -> Blue -> Red -> When turn ends -> Racing Red

   MODES:

    Set Turn Time -> When setting turn time
    Turn mode -> when active
    loiter mode -> When not in Set Turn Time or Turn Mode or
*/


#include <FastLED.h>
#define DATA_PIN 6
#define NUM_LEDS 16

CRGB leds[NUM_LEDS];

const byte buttonPin = 3;

const unsigned long selectorBlinkInterval = 250; // ON/OFF interval for selector LED when setting time
const unsigned long holdToFinishInterval = 3 * 1000; // How long to hold button when making final selection for time

long turnTime = 0;


// Flag to reset counter
volatile boolean buttonPressed = false;
int counter = 0;

// Actions to take on button press
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



// Set all LEDs to one color
void changeAllColorTo(int h, int s = 255, int v = 255) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(h, s, v);
  }
}

void blinkRed() {

  changeAllColorTo(255);
  FastLED.show();
  delay(300);

//  changeAllColorTo(0, 0, 0);
//  FastLED.show();
//  delay(300);
}

void fadeall() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].nscale8(250);
  }
}

void signalTimeSelected() {
  static uint8_t hue = 0;

  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(hue++, 255, 255);
    FastLED.show();
    fadeall();
    delay(10);
  }

  for (int i = (NUM_LEDS) - 1; i >= 0; i--) {
    leds[i] = CHSV(hue++, 255, 255);
    FastLED.show();
    fadeall();
    delay(10);
  }
}


int selectTime(CHSV uncountedColor, CHSV countedColor) {

  int timeCounter = 0;
  boolean timeSet = false;
  boolean selectorLEDState = LOW;
  unsigned long previousMillis = 0;

  unsigned long previousButtonHoldTime = 0;

  while (!timeSet) {

    /*
       Set all LEDs one color to represent "counted",
       and another color to represent "not yet counted"
    */
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = i > timeCounter
                ? uncountedColor
                : countedColor;
    }

    /*
      Blink the selector LED
      Every interval set the "selector" LED to a different color
      The selector LED is at the current count.
    */
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

    // Exit while loop if button is held longer than 3 seconds
    if (!digitalRead(buttonPin)) {
      unsigned long buttonHoldTime = millis() - previousButtonHoldTime;

      Serial.print("buttonHoldTime = ");
      Serial.println(buttonHoldTime);

      Serial.print("previousButtonHoldTime = ");
      Serial.println(previousButtonHoldTime);

      if (buttonHoldTime > holdToFinishInterval) {
        timeSet = true;
 
        //Display cylon effect to show done
        for (int i = 0; i < 10; i++) {
          signalTimeSelected();
        }

        buttonPressed = false;
      }

    } else {
      previousButtonHoldTime = millis();
    }

  }

  return timeCounter;
}


void setup() {
  Serial.begin(115200);

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(84);

  pinMode(buttonPin, INPUT_PULLUP);
  attachInterrupt(buttonPin, buttonPress, RISING);

  CHSV secondsCountedColor = CHSV(0, 255, 255);
  CHSV secondsUncountedColor = CHSV(160, 255, 255);

  CHSV minutesCountedColor = CHSV(32, 255, 255);
  CHSV minutesUncountedColor = CHSV(192, 255, 255);

  CHSV hoursCountedColor = CHSV(64, 255, 255);
  CHSV hoursUncountedColor = CHSV(224, 255, 255);

  long secondsCount = 1000 * selectTime(secondsCountedColor, secondsUncountedColor); // 10 seconds for every count
  long minutesCount = 1000 * selectTime(minutesCountedColor, minutesUncountedColor);

  turnTime = secondsCount + minutesCount;// + minutesCount;

}

void loop() {

  //static long turnTime = timeSelection; // Time for each turn in seconds
  boolean overTime = false;

  // Show fade from Green to Blue to Red
  for (int hue = 85; hue <= 255; hue++) {
    changeAllColorTo(hue);
    FastLED.show();
    delay(turnTime / 170);

    // If button was pressed, Exit
    if (buttonPressed) {

      buttonPressed = false;
      counter++;

      break;
    }

    // When exiting for loop for last time, set overTime flag to signal blicking reds
    if (hue == 255) {
      overTime = true;
    }
  }

  // Start Blinking RED until next button press
  while (overTime) {

    blinkRed();

    // If button was pressed, Exit
    if (buttonPressed) {
      buttonPressed = false;
      counter++;
      break;
    }
  }

  Serial.print("Button Count: ");
  Serial.println(counter);

}
