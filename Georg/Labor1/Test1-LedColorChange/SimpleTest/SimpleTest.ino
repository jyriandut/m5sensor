/*
 * Simple LED Test for M5 Atom Lite
 * This will just blink the LED red to verify it works
 */

#include <FastLED.h>

#define LED_PIN     27
#define NUM_LEDS    1
#define BRIGHTNESS  100

CRGB leds[NUM_LEDS];

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("Starting M5 Atom Lite LED Test...");
  
  // Initialize FastLED
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  
  Serial.println("LED initialized!");
}

void loop() {
  // Red
  Serial.println("RED");
  leds[0] = CRGB::Red;
  FastLED.show();
  delay(1000);
  
  // Green
  Serial.println("GREEN");
  leds[0] = CRGB::Green;
  FastLED.show();
  delay(1000);
  
  // Blue
  Serial.println("BLUE");
  leds[0] = CRGB::Blue;
  FastLED.show();
  delay(1000);
  
  // Off
  Serial.println("OFF");
  leds[0] = CRGB::Black;
  FastLED.show();
  delay(1000);
}
