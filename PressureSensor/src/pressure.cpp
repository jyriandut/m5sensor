#include "pressure.h"
#include <cstdint>

const int PRESSURE_PIN = 33;  // G33 analog input

const float R1 = 970.0;
const float R2 = 2560.0;
const float DIVIDER_RATIO = R2 / (R1 + R2);

// MPX5700AP sensor specifications
// Vout = Vs * (0.0012858 * P + 0.04)
// P = (Vout/Vs - 0.04) / 0.0012858
const float VS = 5.0;             // Supply voltage
const float TF_SLOPE = 0.0012858; // Transfer function slope
const float TF_OFFSET = 0.04;     // Transfer function offset

// Calibration - set to 0 to use datasheet formula directly
const float CALIBRATION_OFFSET = 0.0;  // kPa
const bool USE_CALIBRATION = false;

// Filtering
const int NUM_SAMPLES = 100;       // Number of samples for averaging

int historyIndex = 0;


namespace pressure {
  void initPressureReader() {      
    Serial.println("MPX5700AP Pressure Sensor");
    Serial.println("========================");
    delay(1000);
  }

  uint16_t get_pressure_uint() {
    float pressure_read = getPressure();
    auto pressureInt = static_cast<uint16_t>(pressure_read);
    return pressureInt;
  }
  
  float getPressure() {
    uint32_t pressureHistoryMv[NUM_SAMPLES];
    float pressureSum = 0.0;

    for (size_t i = 0; i < NUM_SAMPLES; i++) {
      pressureHistoryMv[i] = analogReadMilliVolts(PRESSURE_PIN);
      delayMicroseconds(1000);
    }
    float avgMv = 0.0;
    for (size_t i = 0; i < NUM_SAMPLES; i++) {
      avgMv += pressureHistoryMv[i];
    }
    avgMv /= NUM_SAMPLES;

    float vMeasured = avgMv / 1000.0;
    Serial.printf("Average measured mv: %f \n", vMeasured);
    float vSensor = vMeasured / DIVIDER_RATIO;

    float voutRatio = vSensor / VS;
    float pressure = (voutRatio - TF_OFFSET) / TF_SLOPE;

    if (USE_CALIBRATION) {
      pressure += CALIBRATION_OFFSET;
    }

    Serial.printf("%lu, %.3f kPa\n", millis(), pressure);
    // float pressureBar = pressure / 100.0;
    // float pressurePsi = pressure * 0.145038;

    return pressure;
  }
}
