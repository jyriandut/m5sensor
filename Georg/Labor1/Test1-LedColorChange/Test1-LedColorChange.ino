/*
 * LED Frequency Test for M5Stack Atom Lite
 * 
 * Tests LED color change smoothness at different frequencies
 * Cycles through: Red -> Green -> Blue -> Red...
 * 
 * Frequencies tested: 1, 2, 5, 10, 20, 50 Hz
 * Each test runs for 5 seconds
 * 
 * Hardware: M5Stack Atom Lite (WS2812B RGB LED on GPIO 27)
 * Serial Monitor: 115200 baud
 * 
 * Required Library: FastLED
 * Install via: Arduino IDE -> Tools -> Manage Libraries -> Search "FastLED"
 */

#include <FastLED.h>

// M5 Atom Lite LED configuration
#define LED_PIN     27        // GPIO 27 for M5 Atom Lite
#define NUM_LEDS    1         // Only 1 LED on Atom Lite
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define BRIGHTNESS  50        // 0-255, adjust brightness here

CRGB leds[NUM_LEDS];

// Test frequencies in Hz (color changes per second)
const float frequencies[] = {1, 2, 5, 10, 20, 50};
const int numFrequencies = 6;
const int testDuration = 5000; // 5 seconds per test in milliseconds

// Current color state
enum Color { RED, GREEN, BLUE };
Color currentColor = RED;

// Test results
float maxSmoothFrequency = 0;

void setup() {
  // Initialize Serial
  Serial.begin(115200);
  delay(1000); // Wait for serial to initialize
  
  // Initialize FastLED
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  
  // Turn off LED
  allOff();
  
  // Print welcome message
  Serial.println("\n\n=================================");
  Serial.println("   LED FREQUENCY TEST");
  Serial.println("   M5Stack Atom Lite");
  Serial.println("=================================");
  Serial.println("This test will cycle LED colors:");
  Serial.println("Red -> Green -> Blue -> Red...");
  Serial.println();
  Serial.println("After each test, answer:");
  Serial.println("  'y' = smooth");
  Serial.println("  'n' = not smooth");
  Serial.println("=================================\n");
  
  delay(2000);
  
  // Run the test
  runFrequencyTest();
  
  // Display results
  displayResults();
}

void loop() {
  // Test is complete, do nothing
  delay(1000);
}

void runFrequencyTest() {
  for (int i = 0; i < numFrequencies; i++) {
    float freq = frequencies[i];
    
    // Display test info
    Serial.println("=================================");
    Serial.print("Testing at ");
    Serial.print(freq);
    Serial.print(" Hz (");
    Serial.print(freq);
    Serial.println(" color changes/sec)");
    Serial.println("Duration: 5 seconds");
    Serial.println("Watch the LED...");
    Serial.println();
    
    // Run the LED test
    runLEDTest(freq, testDuration);
    
    // Turn off LED
    allOff();
    
    // Ask for feedback
    Serial.println("\nWas the color change smooth?");
    Serial.print("Enter 'y' for yes or 'n' for no: ");
    
    // Wait for user input
    char response = waitForResponse();
    
    if (response == 'y') {
      maxSmoothFrequency = freq;
      Serial.println("✓ Recorded as smooth\n");
    } else if (response == 'n') {
      Serial.println("✗ Not smooth - stopping test\n");
      break; // Stop testing
    } else {
      Serial.println("Invalid input - stopping test\n");
      break;
    }
    
    delay(1000); // Brief pause between tests
  }
}

void runLEDTest(float frequency, int duration) {
  // Calculate delay between color changes (in microseconds)
  unsigned long delayMicros = (unsigned long)(1000000.0 / frequency);
  
  unsigned long startTime = millis();
  unsigned long lastChange = micros();
  
  currentColor = RED;
  setColor(currentColor);
  
  while (millis() - startTime < duration) {
    unsigned long currentMicros = micros();
    
    // Check if it's time to change color
    if (currentMicros - lastChange >= delayMicros) {
      // Change to next color
      nextColor();
      setColor(currentColor);
      lastChange = currentMicros;
    }
  }
}

void nextColor() {
  switch (currentColor) {
    case RED:
      currentColor = GREEN;
      break;
    case GREEN:
      currentColor = BLUE;
      break;
    case BLUE:
      currentColor = RED;
      break;
  }
}

void setColor(Color color) {
  // Set the LED color using FastLED
  switch (color) {
    case RED:
      leds[0] = CRGB::Red;
      break;
    case GREEN:
      leds[0] = CRGB::Green;
      break;
    case BLUE:
      leds[0] = CRGB::Blue;
      break;
  }
  FastLED.show();
}

void allOff() {
  leds[0] = CRGB::Black;
  FastLED.show();
}

char waitForResponse() {
  // Clear any existing serial data
  while (Serial.available()) {
    Serial.read();
  }
  
  // Wait for user input
  while (!Serial.available()) {
    delay(10);
  }
  
  // Read the response
  char response = Serial.read();
  
  // Convert to lowercase
  if (response >= 'A' && response <= 'Z') {
    response = response + 32;
  }
  
  // Echo the response
  Serial.println(response);
  
  // Clear remaining serial data
  while (Serial.available()) {
    Serial.read();
  }
  
  return response;
}

void displayResults() {
  Serial.println("\n=================================");
  Serial.println("      TEST COMPLETE");
  Serial.println("=================================");
  
  if (maxSmoothFrequency > 0) {
    Serial.print("Maximum smooth frequency: ");
    Serial.print(maxSmoothFrequency);
    Serial.println(" Hz");
    Serial.println();
    Serial.println("This means the LED can change colors");
    Serial.print(maxSmoothFrequency);
    Serial.println(" times per second smoothly.");
  } else {
    Serial.println("No smooth frequency found.");
    Serial.println("Even 1 Hz was not smooth.");
  }
  
  Serial.println("=================================");
  Serial.println("\nTest finished. Reset to run again.");
}
