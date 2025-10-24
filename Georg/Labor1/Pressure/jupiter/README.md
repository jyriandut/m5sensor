# MPX5700AP Pressure Sensor Calibration and Analysis

Digital pressure system calibration project using Jupyter Lab for data analysis and visualization.

## 📊 Project Overview

This project performs comprehensive calibration and analysis of the MPX5700AP pressure sensor system, including:
- System resolution and range determination
- ADC-to-Pascal scale calibration
- Linearity testing with regression analysis
- Noise analysis and effective resolution calculation

## 🎯 Objectives

1. **Determine Resolution and Range**: Calculate theoretical and effective resolution of the 12-bit ADC system
2. **Calibrate Pascal Scale**: Convert raw ADC readings to pressure (kPa) using MPX5700AP transfer function
3. **Test Linearity**: Analyze pressure response linearity across measurement range
4. **Calculate Effective Resolution**: Determine ENOB (Effective Number of Bits) based on noise analysis

## 🔬 Experiment Protocol

### Hardware Setup
- **Sensor**: MPX5700AP (15-700 kPa absolute pressure)
- **Microcontroller**: M5Stack Atom Lite (ESP32)
- **ADC**: 12-bit (0-4095), 3.3V reference
- **Connection**: Sensor analog output → GPIO 32

### Data Collection
Three pressure levels with 10 measurements each:
- **Low Pressure**: 1ml syringe volume (10 readings)
- **Medium Pressure**: 5ml syringe volume (10 readings)
- **High Pressure**: 9ml syringe volume (10 readings)
- **Total**: 30 measurements

## 📁 Project Files

### Jupyter Notebook
- **`pressure_calibration.ipynb`** - Main analysis notebook with:
  - Data collection functions (serial communication with ESP32)
  - ADC-to-Pascal calibration
  - Linear regression analysis
  - Noise and resolution calculations
  - Comprehensive visualizations
  - Summary report generation

### Arduino Sketch
- **`pressure_data_logger/pressure_data_logger.ino`** - ESP32 data logger:
  - Reads MPX5700AP sensor on GPIO 32
  - Outputs clean ADC values via serial (115200 baud)
  - 500ms sampling interval (2 Hz)
  - LED feedback (green=ready, blue=reading)

### Output Files (Generated)
- **`pressure_calibration_data.csv`** - All measurements with calibrated values
- **`calibration_summary.csv`** - Key metrics and results

## 🚀 Getting Started

### Prerequisites

**Python Libraries:**
```bash
pip install numpy pandas matplotlib seaborn scipy pyserial jupyter
```

**Arduino Setup:**
1. Install M5Atom library in Arduino IDE
2. Select board: "M5Atom"
3. Set baud rate: 115200

### Step-by-Step Procedure

#### 1. Hardware Setup
```
1. Connect MPX5700AP sensor to M5 Atom Lite GPIO 32
2. Connect M5 Atom Lite to computer via USB
3. Note the COM port (e.g., COM3 on Windows)
```

#### 2. Upload Arduino Sketch
```
1. Open pressure_data_logger.ino in Arduino IDE
2. Upload to M5 Atom Lite
3. Open Serial Monitor to verify readings
```

#### 3. Run Jupyter Notebook
```bash
# Start Jupyter Lab
jupyter lab

# Open pressure_calibration.ipynb
# Follow the notebook cells sequentially
```

#### 4. Data Collection
```python
# In Jupyter notebook, modify the COM port:
adc_1ml = read_pressure_from_esp32(port='COM3', num_readings=10)

# For each pressure level:
# 1. Push syringe to specified volume (1ml, 5ml, or 9ml)
# 2. Wait for pressure to stabilize
# 3. Run the data collection cell
# 4. Release pressure before next level
```

#### 5. Analysis
```
Run all notebook cells to:
- Calibrate ADC readings to kPa
- Perform linear regression
- Analyze residuals
- Calculate noise and effective resolution
- Generate visualizations
- Export results
```

## 📈 Analysis Methods

### 1. Calibration (ADC → Pascal)
Uses MPX5700AP transfer function:
```
Vout = Vs × (0.0012858 × P + 0.04)
```
Rearranged to solve for pressure P from ADC reading.

### 2. Linearity Test
- **Linear Regression**: Fits straight line to pressure vs. volume data
- **R² Analysis**: Measures goodness of fit (>0.95 = excellent)
- **Residual Analysis**: Checks for systematic errors

### 3. Resolution Calculation

**Theoretical Resolution:**
- 12-bit ADC = 4096 levels

**Effective Resolution (ENOB):**

Method 1 - Peak-to-Peak Noise:
```
ENOB = log₂(Full Scale Range / Noise Peak-to-Peak)
```

Method 2 - SNR:
```
ENOB = (SNR_dB - 1.76) / 6.02
```

### 4. Noise Analysis
- Standard deviation of repeated measurements at constant pressure
- Peak-to-peak variation
- Converted to both ADC counts and kPa

## 📊 Expected Results

### Typical Output
```
=== Linear Regression Results ===
Slope: 25.4321 kPa/ml
Intercept: 98.7654 kPa
R²: 0.9876
Standard error: 1.23 kPa

=== Noise Analysis ===
ADC noise (SD): 3.45 counts
Pressure noise (SD): 0.52 kPa

=== Effective Resolution ===
ENOB: 10.8 bits
Pressure resolution: ±0.52 kPa
```

### Visualizations
1. **Raw ADC vs Volume** - Scatter plot of measurements
2. **Calibrated Pressure vs Volume** - With regression line
3. **Residual Plot** - Error distribution analysis
4. **Noise Over Time** - Stability at constant pressure

## 🔧 Troubleshooting

### Serial Connection Issues
```python
# List available COM ports
import serial.tools.list_ports
ports = serial.tools.list_ports.comports()
for port in ports:
    print(port.device)
```

### Poor Linearity (R² < 0.90)
- Check sensor connections
- Verify pressure chamber is sealed
- Ensure syringe volumes are accurate
- Allow more time for pressure stabilization

### High Noise Levels
- Check power supply stability
- Add decoupling capacitors to sensor
- Increase number of measurements
- Average multiple readings

## 📚 Theory Background

### MPX5700AP Sensor
- **Type**: Piezoresistive pressure sensor
- **Range**: 15-700 kPa (absolute)
- **Output**: Analog voltage (0.2V-4.7V @ 5V supply)
- **Accuracy**: ±2.5% full scale span
- **Temperature compensated**: 0°C to 85°C

### ESP32 ADC
- **Resolution**: 12-bit (0-4095)
- **Reference**: 3.3V (internal)
- **Attenuation**: 11dB (0-3.3V range)
- **Non-linearity**: ±2 LSB typical

## 📖 References

1. MPX5700AP Datasheet - NXP Semiconductors
2. ESP32 Technical Reference Manual - Espressif Systems
3. "Effective Number of Bits (ENOB)" - Application Note AN-4
4. Linear Regression Analysis - Statistical Methods

## 👤 Author

Georg - University of Tartu Robotics Course

## 📄 License

Educational project for University of Tartu coursework.
