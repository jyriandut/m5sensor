/*
*******************************************************************************
* M5Stack Atom Lite - Pressure Sensor Data Logger for Jupyter Lab
* 
* PURPOSE:
* Reads MPX5700AP pressure sensor and outputs ADC values via serial port
* for data collection and analysis in Jupyter Lab.
* 
* FEATURES:
* - Continuous ADC reading from GPIO 32
* - Clean serial output (ADC values only)
* - 500ms sampling interval
* - LED feedback (green = ready, blue = reading)
* 
* HARDWARE:
* - M5Stack Atom Lite
* - MPX5700AP Pressure Sensor on GPIO 32
* 
* SERIAL OUTPUT FORMAT:
* ADC: 1234
* ADC: 1235
* ...
* 
* USAGE:
* 1. Upload to M5 Atom Lite
* 2. Open Serial Monitor (115200 baud)
* 3. Use Python serial library to collect data in Jupyter Lab
*******************************************************************************
*/

#include <M5Atom.h>

const int analogPin = 32;  // MPX5700AP connected to GPIO 32
const int sampleDelay = 500;  // 500ms between readings

void setup() {
  M5.begin(true, false, true);
  delay(50);
  
  Serial.begin(115200);
  delay(1000);
  
  // Green LED = ready
  M5.dis.drawpix(0, CRGB::Green);
  
  Serial.println("===========================================");
  Serial.println("  MPX5700AP Pressure Data Logger");
  Serial.println("===========================================");
  Serial.println("Sensor: MPX5700AP (15-700 kPa)");
  Serial.println("ADC: 12-bit (0-4095)");
  Serial.println("Sample rate: 2 Hz (500ms)");
  Serial.println("===========================================");
  Serial.println();
  Serial.println("Starting data collection...");
  Serial.println();
  
  delay(2000);
}

void loop() {
  // Blue LED = reading
  M5.dis.drawpix(0, CRGB::Blue);
  
  // Read ADC value
  int adcValue = analogRead(analogPin);
  
  // Output in clean format for Python parsing
  Serial.print("ADC: ");
  Serial.println(adcValue);
  
  // Green LED = ready
  M5.dis.drawpix(0, CRGB::Green);
  
  delay(sampleDelay);
}
