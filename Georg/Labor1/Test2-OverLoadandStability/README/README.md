# ESP32 Overload and Stability Test

## Overview
This is an **ESP32 Overload and Stability Test** designed to find the practical limits of the system under intensive use to ensure stability and graceful degradation through the web interface.

Below is a step-by-step guide on how to perform this test.

---

## 1. Preparation and Setup

### 1.1 Define the Test Environment
- Ensure you have a working **ESP32 development board** (M5Stack Atom Lite) and a connected LED that can be controlled by commands
- Verify that the ESP32 is running API code that supports:
  - **SET** (color setting) commands
  - **GET** (state query) commands (likely over HTTP)

### 1.2 Create the Testing Tool
- You will need a script (e.g., Python, C++, etc.) capable of:
  - Sending repeated and rapid HTTP POST/PUT (SET) requests to the ESP32
  - Measuring the results and timing of those requests
  - **Note:** I will use Arduino IDE and `.ino` file for that
- The script must be able to:
  - Generate **random RGB values** for each SET command
  - Send **HTTP GET requests** to check the device's current state

### 1.3 Set Up Monitoring
- The script must measure the **response time (latency)** for every HTTP SET response
- If possible, set up internal logging on the ESP32 (e.g., via serial) to see if and when the device starts:
  - Dropping commands
  - Generating internal errors

---

## 2. Escalating Load Test (Stress Test)

**Goal:** Find the critical point where the ESP32 begins to behave erratically.

### 2.1 Baseline (10 commands/s)
- Run the testing script, sending **10 SET commands per second** with random RGB values
- **Monitor:**
  - **HTTP Response Times:** Check if they are fast, e.g., below 100ms
  - **LED Behavior:** Check if colors change instantly and correctly

### 2.2 Gradual Load Increase
- Increase the command sending frequency gradually (e.g., in increments of 5-10 commands/s):
  - **10 → 15 → 20 → 30 → 40 ... up to 100+ commands per second**
- At each load level (e.g., in **30-second intervals**), perform the following steps:
  - **Measure and Log:** Continue logging the response times for all SET commands
  - **Identify Slow/Failed Responses:** Note the first level where response times become slow (>500ms) or where responses fail to arrive completely (timeout)
  - **Check for Dropped Commands (GET Verification):** After each burst of SET commands, send one or more GET commands (state query) to check the LED's current value
  - **Compare:** Compare the value of the last SET command sent with the value returned by the GET command
  - **Result:** If these values do not match, the system has started dropping commands. Log this load level.

### 2.3 Identify the Critical Point
- Continue increasing the load until the system:
  - Completely **crashes** (stops responding to HTTP requests entirely), OR
  - Command dropping becomes the dominant problem
- This is the **critical instability point**

---

## 3. Recovery Test

**Goal:** Verify whether the system can automatically recover from an overload.

### 3.1 Induce Overload
- Push the ESP32 slightly above the critical point found in the previous test, so the device is clearly overloaded or "hung"
- **Note (log)** the color of the last SET command before the recovery test begins

### 3.2 Stop the Load
- Immediately **stop sending all SET commands** using the testing script

### 3.3 Measure Recovery Time
- Continue sending regular **GET requests** (e.g., once per second) to the device
- Measure the time from when command sending stopped until the device starts responding normally to the API (response time <100ms or another normal time)
- This is the **recovery time**

### 3.4 Verify State Persistence
- After the system recovers, check the actual LED color
- Perform a GET request and verify that the value returned by the device matches the color of the last SET command sent
- This confirms that the system **maintained its state** even during the overload

---

## 4. Practical Value Analysis and Results

### 4.1 Compile the Results
- **Maximum Stable Command Frequency:** The highest number of commands per second where response times were acceptable (e.g., <200ms) and no commands were dropped (GET verification was successful). This is the number the web interface must account for.
- **Graceful Degradation Limit:** The load level where response times became slow (e.g., >500ms) but the device was still working (i.e., not crashed)
- **Critical Failure Limit:** The load level where the system began dropping commands or crashed
- **Recovery Time:** The time taken to return to a normal state after the overload ceased

### 4.2 Provide Practical Value (Throttling Recommendations)
- The results are used to set the logic for the web interface's **Throttling** (command limitation)
- **Example:** If the maximum stable frequency is 35 commands per second, the web interface should be designed to send a maximum of 30-35 commands per second when the user drags the color wheel, to prevent stability issues

---

## Summary

This test will help determine:
- Maximum safe operating frequency
- System behavior under stress
- Recovery capabilities
- Optimal throttling settings for the web interface

---

## 5. Implementation Options for Overload and Stability Test

Based on the requirements and reference files, here are the approaches we can take:

### **Option 1: Self-Contained Test (Recommended)**
**M5 Atom Lite tests itself without external dependencies**

**How it works:**
- The M5 Atom runs both the **web server** (from `WebServer_WifiSetup.ino`) AND the **test client** code
- Uses a timer/loop to generate HTTP requests internally to itself (localhost/127.0.0.1)
- Measures response times, tracks dropped commands, logs results via Serial

**Pros:**
- ✅ No external computer needed
- ✅ Self-contained, portable test
- ✅ Similar structure to `Test1-LedColorChange.ino` (user-friendly)
- ✅ Easy to run and repeat

**Cons:**
- ❌ Testing the device with itself may not reflect real-world network conditions
- ❌ Limited by single-core processing (server + client compete for resources)

---

### **Option 2: Two M5 Devices (Client-Server)**
**One M5 runs the server, another runs the test client**

**How it works:**
- **Device 1:** Runs the web server (modified `WebServer_WifiSetup.ino`)
- **Device 2:** Runs the test client that sends HTTP requests to Device 1's IP
- Client measures response times and logs results

**Pros:**
- ✅ More realistic network testing
- ✅ Separates server and client workloads
- ✅ Better stress testing

**Cons:**
- ❌ Requires 2 M5 Atom devices
- ❌ More complex setup

---

### **Option 3: Python Script on Computer**
**External Python script sends requests to M5 Atom**

**How it works:**
- M5 Atom runs the web server
- Python script on your computer sends HTTP requests at varying frequencies
- Script logs response times, dropped commands, etc.

**Pros:**
- ✅ Most realistic real-world scenario
- ✅ Powerful logging and analysis capabilities
- ✅ Easy to adjust test parameters

**Cons:**
- ❌ Requires Python setup on computer
- ❌ Not self-contained in Arduino IDE
- ❌ You mentioned preferring `.ino` files

---

### **Option 4: Hybrid Approach**
**M5 Atom runs server + basic client, with optional external monitoring**

**How it works:**
- M5 Atom runs web server and internal test client
- Optionally, you can also monitor via external tools (browser, Python script)
- Best of both worlds

**Pros:**
- ✅ Flexible
- ✅ Can run standalone or with external monitoring
- ✅ Good for iterative testing

**Cons:**
- ❌ More complex code

---

### **Recommended Implementation: Option 1**

**Rationale:**
1. Matches the `Test1-LedColorChange.ino` pattern (interactive, serial-based)
2. Uses Arduino IDE and `.ino` files exclusively
3. Easy to run, repeat, and share
4. For M5 Atom Lite, the goal is to find practical limits for web interface throttling

**Implementation Plan:**
1. **Base code:** Use `WebServer_WifiSetup.ino` as the web server foundation
2. **Add GET endpoint:** Implement `/get` to return current LED color
3. **Add test mode:** When button is pressed (or serial command), enter test mode:
   - Automatically send SET requests at increasing frequencies
   - Measure response times
   - Verify with GET requests
   - Log results to Serial Monitor
4. **User interaction:** Similar to Test1, ask user to confirm observations at each load level

---

## 6. Final Implementation Details

### **Key Features**

#### **Web Server (Built-in)**
- **`/get`** endpoint - Returns current LED color as JSON
- **`/set?color=%23RRGGBB`** endpoint - Sets LED color
- Runs on WiFi AP: `M5Stack_Test` / `12345678` at `192.168.4.1`

#### **HTTP Client (Self-Testing)**
- Sends SET requests at increasing frequencies (10-100 cmd/s)
- Measures response times for each request
- Verifies with GET requests to detect dropped commands
- Tracks success/failure rates

#### **Test Sequence**
1. **11 frequency levels**: 10, 15, 20, 30, 40, 50, 60, 70, 80, 90, 100 cmd/s
2. **30 seconds** per frequency level
3. **Automatic analysis** with recommendations

#### **Metrics Tracked**
- ✅ Total requests sent
- ✅ Successful/failed requests
- ✅ Response times (min/avg/max)
- ✅ Dropped commands (via GET verification)
- ✅ Success rate percentage

#### **Final Report Includes**
- **Maximum Stable Frequency** (>95% success, <200ms avg, no drops)
- **Graceful Degradation Point** (>80% success, <500ms avg)
- **Critical Failure Point** (<50% success or commands dropped)
- **Throttling recommendation** for web interface

---

### **How to Use**

1. **Upload** the code (`Test2-OverLoadandStability.ino`) to your M5 Atom Lite
2. **Open Serial Monitor** at 115200 baud
3. Wait for the green LED to blink 3 times (server ready)
4. **Press the button** on the M5 Atom to start the test
5. Watch the Serial Monitor for real-time progress:
   - `.` = successful request
   - `X` = failed request
6. After approximately **5.5 minutes**, view the complete results table and recommendations
7. LED blinks blue 5 times when test is complete
8. Press button again to run another test if needed

**Note:** The test is fully automated and self-contained. No external computer or scripts are needed beyond the Serial Monitor for viewing results.
