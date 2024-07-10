#include <FastLED.h>

#define NUM_LEDS 64
#define DATA_PIN 3

CRGB leds[NUM_LEDS];

void setup()
{
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(84);
}

void loop()
{

  static int hue = 0;

  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CHSV(hue++, 255, 255);
    FastLED.show();
  }
}