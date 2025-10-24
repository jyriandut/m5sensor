# Labor1 - M5Stack Atom Lite Projects

This folder contains experimental projects and tests for the M5Stack Atom Lite ESP32 development board.

## 📁 Projects

### 1. **Test1-LedColorChange**
LED frequency perception test for human visual response analysis.

**Purpose**: Determine the maximum frequency at which LED color changes appear smooth to the human eye.

**Features**:
- Tests frequencies: 1, 2, 5, 10, 20, 50 Hz
- Color cycle: Red → Green → Blue → Red
- 5-second test duration per frequency
- Interactive serial monitor interface
- Automatic frequency progression until operator reports flickering

**Hardware**: M5Stack Atom Lite (WS2812B RGB LED on GPIO 27)

**Dependencies**: 
- FastLED library

**Usage**:
1. Upload `Test1-LedColorChange.ino` to M5 Atom Lite
2. Open Serial Monitor (115200 baud)
3. Watch LED and answer 'y' (smooth) or 'n' (not smooth)
4. Test stops when flickering is detected
5. Press RESET button to run test again

**Files**:
- `Test1-LedColorChange.ino` - Main frequency test program
- `SimpleTest/SimpleTest.ino` - Simple LED blink test for hardware verification

---

### 2. **Test2-OverLoadandStability**
ESP32 overload and stability test for web API performance analysis using browser-based testing.

**Purpose**: Determine the maximum stable command frequency for LED color changes via HTTP API to establish throttling limits for web interfaces.

**Two Testing Approaches**:

#### **Approach 1: Sequential with Delays** (`Sequential/`)
- Sends requests one at a time with delays
- Good for baseline testing
- Shows minimum response times under no load
- Results typically show all green (no failures)

#### **Approach 2: Concurrent without Delays** (`Concurrent/`)
- Sends multiple requests simultaneously
- Real stress testing approach
- Finds actual performance limits
- Shows system behavior under heavy load

**Features**:
- Browser-based testing interface at `/test` endpoint
- Tests frequencies: 10, 50, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000 cmd/s
- 30-second test duration per frequency level
- Automatic response time measurement (min/avg/max)
- Success/failure rate tracking
- Color-coded results table
- Comprehensive final report with throttling recommendations

**Hardware**: M5Stack Atom Lite (WS2812B RGB LED on GPIO 27)

**Dependencies**: 
- M5Atom library
- WiFi library (built-in)

**Access**:
- **SSID**: M5Stack_Test
- **Password**: 12345678
- **IP**: http://192.168.4.1
- **Test Page**: http://192.168.4.1/test

**Usage**:
1. Choose version: Sequential or Concurrent
2. Upload code to M5 Atom Lite
3. Connect to WiFi: M5Stack_Test / 12345678
4. Open browser: http://192.168.4.1
5. Click "Launch Overload Test"
6. Click "Start Test" button
7. Wait ~6 minutes for completion
8. View color-coded results and recommendations

**Files**:
- `Sequential/Test2-OverLoadandStability-Seq.ino` - Sequential test version
- `Concurrent/Test2-OverLoadandStability-Con.ino` - Concurrent test version
- `README/README.md` - Detailed methodology, comparison, and implementation guide
- `img/` - Test result screenshots and comparison diagrams

**Recommendation**: Use **Concurrent version** for realistic performance testing and production throttling values.

---

### 3. **Pressure**
Analog pressure sensor reading and monitoring.

**Purpose**: Read analog pressure sensor values and display them via Serial Monitor for calibration and testing.

**Features**:
- Reads analog input from GPIO 32
- 12-bit ADC resolution (0-4095 values)
- 500ms sampling interval
- Continuous real-time monitoring
- Simple serial output for data logging

**Hardware**: 
- M5Stack Atom Lite
- Analog pressure sensor connected to GPIO 32

**Usage**:
1. Connect pressure sensor to GPIO 32
2. Upload `pressure.ino` to M5 Atom Lite
3. Open Serial Monitor (115200 baud)
4. View real-time pressure readings (0-4095)
5. Use readings for calibration or data collection

**Files**:
- `pressure.ino` - Main pressure reading program
- `img/AtomM5_pressure.png` - Wiring diagram/reference image

---

### 4. **WebSitePressure**
WiFi Access Point with LED control and real-time pressure sensor monitoring web interface.

**Purpose**: Create a WiFi hotspot with web-based LED color picker and live pressure sensor data display from MPX5700AP sensor.

**Features**:
- **WiFi Access Point mode** (no router needed)
- **LED color picker** with live preview
- **Real-time pressure monitoring** with auto-refresh (2 Hz)
- **MPX5700AP sensor support** (15-700 kPa range)
- **Separate pages** for control panel, pressure monitor, and WiFi settings
- **JSON API endpoint** for pressure data
- **Secure WiFi configuration** with validation
- **Flash memory storage** for WiFi credentials
- **Professional UI** with modern styling

**Hardware**: 
- M5Stack Atom Lite (WS2812B RGB LED on GPIO 27)
- MPX5700AP Pressure Sensor connected to GPIO 32

**Dependencies**: 
- M5Atom library
- WiFi library (built-in)
- Preferences library (built-in)

**Access**:
- **SSID**: M5Stack_Ap
- **Password**: 66666666
- **URL**: http://192.168.4.1

**Endpoints**:
- `/` - Main control panel (color picker + navigation)
- `/set?value=%23RRGGBB` - Set LED color
- `/pressure` - Real-time pressure sensor monitoring page
- `/getpressure` - JSON API endpoint for current pressure reading
- `/wifisettings` - WiFi configuration page
- `/savewifi?ssid=...&pass=...` - Save WiFi credentials with validation

**Usage**:
1. Connect MPX5700AP sensor to GPIO 32
2. Upload `WebSitePressure.ino` to M5 Atom Lite
3. Connect to WiFi: **M5Stack_Ap** / **66666666**
4. Open browser: **http://192.168.4.1**
5. Use color picker to change LED
6. Click "📊 View Pressure Sensor Data" for live monitoring
7. Click "⚙️ Configure WiFi Settings" to save credentials

**Pressure Sensor Details**:
- **Sensor**: MPX5700AP
- **Range**: 15-700 kPa
- **Output**: 0.2V-4.7V (analog)
- **Update Rate**: 2 Hz (500ms)
- **Display**: kPa, raw ADC value, voltage

**Files**:
- `WebSitePressure/WebSitePressure.ino` - Main web server with pressure monitoring

**Use Case**: IoT pressure monitoring, remote sensor access, lab experiments with real-time data visualization.

---

### 5. **Pressure Calibration (Jupyter Lab)**
MPX5700AP pressure sensor calibration and analysis using Jupyter Lab for scientific data analysis.

**Purpose**: Perform comprehensive calibration of the digital pressure system, including resolution determination, linearity testing, and noise analysis.

**Features**:
- **Serial data collection** from ESP32 via Python
- **ADC-to-Pascal calibration** using MPX5700AP transfer function
- **Linear regression analysis** with R² calculation
- **Residual analysis** for error detection
- **Noise analysis** at constant pressure
- **Effective resolution calculation** (ENOB)
- **Professional visualizations** with matplotlib/seaborn
- **Automated report generation**
- **CSV export** for documentation

**Hardware**: 
- M5Stack Atom Lite
- MPX5700AP Pressure Sensor connected to GPIO 32
- Syringe for pressure generation (1ml, 5ml, 9ml volumes)

**Software Dependencies**: 
- Python 3.x
- Jupyter Lab
- numpy, pandas, matplotlib, seaborn
- scipy (for regression analysis)
- pyserial (for ESP32 communication)

**Analysis Methods**:
- **Calibration**: MPX5700AP transfer function (ADC → kPa)
- **Linearity Test**: Linear regression with residual analysis
- **Resolution**: ENOB calculation via SNR and peak-to-peak noise
- **Noise Analysis**: Standard deviation and peak-to-peak at constant pressure

**Experiment Protocol**:
1. Collect 10 ADC readings at 1ml syringe volume (low pressure)
2. Collect 10 ADC readings at 5ml syringe volume (medium pressure)
3. Collect 10 ADC readings at 9ml syringe volume (high pressure)
4. Total: 30 measurements for comprehensive analysis

**Usage**:
1. Install Python dependencies: `pip install numpy pandas matplotlib seaborn scipy pyserial jupyter`
2. Upload `pressure_data_logger.ino` to M5 Atom Lite
3. Connect sensor to GPIO 32
4. Start Jupyter Lab: `jupyter lab`
5. Open `pressure_calibration.ipynb`
6. Update COM port in notebook (e.g., 'COM3')
7. Run cells sequentially to collect and analyze data
8. View visualizations and export results

**Output**:
- **Calibration equation**: Linear relationship between volume and pressure
- **R² value**: Linearity quality metric
- **ENOB**: Effective number of bits (noise-limited resolution)
- **Pressure resolution**: ±X.XX kPa measurement precision
- **CSV files**: Raw data and summary statistics
- **Plots**: Linearity, residuals, noise analysis

**Files**:
- `jupiter/pressure_calibration.ipynb` - Main Jupyter notebook with analysis
- `jupiter/pressure_data_logger/pressure_data_logger.ino` - Arduino data logger
- `jupiter/README.md` - Detailed methodology and theory
- `jupiter/pressure_calibration_data.csv` - Generated measurement data
- `jupiter/calibration_summary.csv` - Generated summary statistics

**Use Case**: Scientific sensor calibration, measurement system validation, lab coursework, understanding ADC limitations and noise characteristics.

---

### 6. **WebServer**
WiFi Access Point with LED color control web interface (Simple Version).

**Purpose**: Create a WiFi hotspot on M5 Atom Lite with a web-based LED color picker and WiFi configuration storage.

**Features**:
- **WiFi Access Point mode** (no router needed)
- **LED color picker** with live preview
- **Single-page interface** - everything on one page
- **Flash memory storage** for WiFi credentials
- **Minimal validation** - quick setup for testing
- Shows saved passwords in plain text (for easy debugging)

**Hardware**: M5Stack Atom Lite (WS2812B RGB LED on GPIO 27)

**Dependencies**: 
- M5Atom library
- WiFi library (built-in)
- Preferences library (built-in)

**Access**:
- **SSID**: M5Stack_Ap
- **Password**: 66666666
- **URL**: http://192.168.4.1

**Endpoints**:
- `/` - Main page (color picker + WiFi settings)
- `/set?value=%23RRGGBB` - Set LED color
- `/wifi?ssid=...&pass=...&token=...` - Save WiFi credentials

**Usage**:
1. Upload `WebServer.ino` to M5 Atom Lite
2. Connect to WiFi: **M5Stack_Ap** / **66666666**
3. Open browser: **http://192.168.4.1**
4. Use color picker to change LED
5. Save WiFi credentials for future STA mode

**Use Case**: Quick testing, prototyping, or when security is not critical.

---

### 7. **WebServer_WifiSetup**
WiFi Access Point with LED color control and secure configuration (Advanced Version).

**Purpose**: Professional web interface with secure WiFi configuration, password validation, and better UX.

**Features**:
- **Separate pages** for main interface and settings
- **LED color picker** with live preview
- **Secure WiFi configuration**:
  - Password fields hidden (type="password")
  - Minimum 8 characters validation
  - Professional error/success messages
- **Better UX** with styled forms and navigation
- **Flash memory storage** for persistent settings

**Hardware**: M5Stack Atom Lite (WS2812B RGB LED on GPIO 27)

**Dependencies**: 
- M5Atom library
- WiFi library (built-in)
- Preferences library (built-in)

**Access**:
- **SSID**: M5Stack_Ap
- **Password**: 66666666
- **URL**: http://192.168.4.1

**Endpoints**:
- `/` - Main page (color picker + link to settings)
- `/set?value=%23RRGGBB` - Set LED color
- `/wifisettings` - WiFi configuration page
- `/savewifi?ssid=...&pass=...` - Save WiFi credentials with validation

**Usage**:
1. Upload `WebServer_WifiSetup.ino` to M5 Atom Lite
2. Connect to WiFi: **M5Stack_Ap** / **66666666**
3. Open browser: **http://192.168.4.1**
4. Use color picker to change LED
5. Click "⚙️ Configure WiFi Settings"
6. Enter and save WiFi credentials securely

**Use Case**: Production use, demos, or when security and UX matter.

---

### 8. **Qr-code**
WiFi QR code generator for easy M5Stack connection.

**Purpose**: Generate QR codes for automatic WiFi connection and web interface access - no manual typing needed!

**Features**:
- **WiFi QR code** - Auto-connects phone to M5Stack AP
- **URL QR code** - Opens web interface directly
- **Combined instruction card** - Printable setup guide
- Supports Android 10+ and iOS 11+ native camera apps
- Professional layout with step-by-step instructions

**Output Files**:
- `m5stack_wifi_qr.png` - WiFi connection QR code
- `m5stack_url_qr.png` - Web interface QR code  
- `m5stack_setup_card.png` - Combined printable card

**Dependencies**: 
- Python 3.x
- qrcode library
- Pillow (PIL)

**Configuration**:
- **SSID**: M5Stack_Ap
- **Password**: 66666666
- **URL**: http://192.168.4.1

**Usage**:
1. Install dependencies: `pip install qrcode[pil]`
2. Open `wifi_qr_generator.ipynb` in Jupyter Notebook
3. Run all cells
4. Scan WiFi QR code with phone camera
5. Phone auto-connects to M5Stack
6. Scan URL QR code to open web interface

**Use Case**: Demos, workshops, sharing projects - makes setup instant and foolproof!

---

## 🔧 Hardware Requirements

- **M5Stack: Atom M5 Lite** (ESP32-based development board)
- USB-C cable for programming and power
- Computer with Arduino IDE

## 📚 Software Requirements

- **Arduino IDE** (1.8.x or 2.x)
- **ESP32 Board Support** (via Board Manager)
- **FastLED Library** (for LED projects)

### Installing ESP32 Libary:
Go to Tools → Manage Libraries
2. Search "M5Atom by M5Stack"
3. Click Install

### Installing FastLED Library:
1. Go to Tools → Manage Libraries
2. Search "FastLED"
3. Click Install

## ⚙️ Board Configuration

**Board Settings** (for M5 Atom Lite):
- **Board**: "M5Atom"
- **Upload Speed**: 115200 or 921600
- **CPU Frequency**: 240MHz
- **Flash Frequency**: 80MHz
- **Flash Mode**: QIO
- **Flash Size**: 4MB
- **Partition Scheme**: Default

## 🚀 Getting Started

1. Clone this repository
2. Install required libraries (see Software Requirements)
3. Open desired project `.ino` file in Arduino IDE
4. Select correct board and port
5. Upload to M5 Atom Lite
6. Open Serial Monitor (115200 baud) to interact with the program

## 📝 Notes

- Each Arduino project must be in its own folder with matching filename
- Serial Monitor baud rate: **115200** (unless specified otherwise)
- M5 Atom Lite LED is on **GPIO 27**
- Press RESET button on device to restart programs

## 👤 Author

Georg - University of Tartu Robotics Course

## 📄 License

Educational project for University of Tartu coursework.
