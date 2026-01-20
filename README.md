# Smart Irrigation System

An automated plant watering system with remote control via a mobile app.

## Hardware

### Components
- **ESP32 microcontroller** - WiFi-enabled board for wireless communication
- **DC water pump** - Controlled via GPIO pin 23
- **3x DC solenoid valves** - Water distribution to individual plants (GPIO 19, 21, 22)
- **3x capacitive soil moisture sensors** - Analog readings on GPIO 15, 5, 18
- **3x IRLZ44N MOSFETs** - Low-side switching for pump and valves
- **2x 9V batteries** - Power supply (connected in series for 18V or parallel for extended runtime)
- **LEDs** - Visual indicators

### Circuit Design
- MOSFETs drive the pump and valves, with ESP32 GPIO pins controlling the gates
- Soil moisture sensors use analog-to-digital conversion (ADC) to measure soil conductivity
- Sensors are calibrated with `airValue = 3500` (dry) and `waterValue = 1500` (wet), mapped to 0-100% moisture scale

## Software

### ESP32 Firmware (`Mobile_Irigation_No1.ino`)
Written in C++ using the Arduino framework.

**Key Features:**
- **WiFi connectivity** - Creates or connects to a network using credentials defined in code
- **HTTP web server** - Listens on port 80 for control commands
- **Moisture monitoring** - Polls sensors every 20 seconds, converts raw ADC values to percentage
- **Valve control logic** - Activates specific valves and automatically manages pump state
- **RESTful API endpoints:**
  - `/status` - Returns JSON with current moisture levels and valve/pump states
  - `/valve1on`, `/valve1off` - Control valve 1 (and similarly for valves 2 and 3)

**Operation:**
- Pump runs only when at least one valve is open
- When all valves close, the pump automatically shuts off
- Serial output at 115200 baud displays IP address and diagnostic information

### Mobile App
Built with **.NET MAUI** (cross-platform framework) in C#.

**Functionality:**
- Connects to the ESP32 via HTTP using the IP address displayed in serial monitor
- Displays real-time soil moisture percentages for each plant
- Provides buttons to manually trigger watering for individual plants
- Polls `/status` endpoint periodically to update UI

## Setup

1. **Flash the firmware:** Upload `Mobile_Irigation_No1.ino` to the ESP32 using Arduino IDE
2. **Note the IP address:** Open Serial Monitor (115200 baud) to see the ESP32's IP
3. **Configure the app:** Update the IP address in the C# code (MAUI app) to match
4. **Build and deploy:** Compile the MAUI app and install on Android/iOS/Windows
5. **Calibrate sensors:** Adjust `airValue` and `waterValue` in the `.ino` file based on your sensors' readings in dry soil and water

## Notes
- Sensor readings are raw ADC values (0-4095 on ESP32) mapped to percentage
- WiFi reconnection logic attempts to restore connectivity if the connection drops
- The system supports up to 3 plants but can be expanded by adding more pins and endpoints