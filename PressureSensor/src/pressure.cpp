#include "pressure.h"


// Pin configuration
const int PRESSURE_PIN = 33;  // G33 analog input

// Voltage divider configuration
const float R1 = 970.0;      // 1kΩ top resistor
const float R2 = 2560.0;      // 2.2kΩ bottom resistor
const float DIVIDER_RATIO = R2 / (R1 + R2);  // 0.6875

// MPX5700AP sensor specifications
// Official transfer function: Vout = Vs * (0.0012858 * P + 0.04)
// Solving for P: P = (Vout/Vs - 0.04) / 0.0012858
const float VS = 5.0;             // Supply voltage
const float TF_SLOPE = 0.0012858; // Transfer function slope
const float TF_OFFSET = 0.04;     // Transfer function offset

// Calibration - set to 0 to use datasheet formula directly
const float CALIBRATION_OFFSET = 0.0;  // kPa
const bool USE_CALIBRATION = false;

// Filtering
const int NUM_SAMPLES = 100;       // Number of samples for averaging
float pressureHistory[NUM_SAMPLES];
int historyIndex = 0;


namespace pressure {
void initPressureReader() {
  for (int i = 0; i < NUM_SAMPLES; i++) {
    pressureHistory[i] = 0;
  }
  
  Serial.println("MPX5700AP Pressure Sensor");
  Serial.println("========================");
  delay(1000);
}

float getPressure() {
  uint32_t vMeasuredMv = analogReadMilliVolts(PRESSURE_PIN);
  float vMeasured = vMeasuredMv / 1000.0;
  
  float vSensor = vMeasured / DIVIDER_RATIO;
  
  float voutRatio = vSensor / VS;
  float pressure = (voutRatio - TF_OFFSET) / TF_SLOPE;
  
    if (USE_CALIBRATION) {
    pressure += CALIBRATION_OFFSET;
  }
    pressureHistory[historyIndex] = pressure;
  historyIndex = (historyIndex + 1) % NUM_SAMPLES;
    
  float pressureFiltered = 0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    pressureFiltered += pressureHistory[i];
  }
  pressureFiltered /= NUM_SAMPLES;
  Serial.printf("%lu,%.3f\n", millis(), pressureFiltered);

  float pressureBar = pressureFiltered / 100.0;
  float pressurePsi = pressureFiltered * 0.145038;
  delay(10); // Read every 100ms

  return pressureFiltered;
}

  
  
}
