# Smart Bin Device - ESP32 Weight-Based Inventory Tracking

This project implements a smart bin system using ESP32 microcontrollers with HX711 load cell amplifiers to track inventory levels based on weight measurements.

## Features

- **Bluetooth Provisioning**: Configure WiFi credentials and API settings via Bluetooth from a Unity mobile application
- **Multi-Sensor Support**: Supports up to 6 HX711 load cell amplifiers per device
- **Real-time Data Transmission**: Sends sensor data to remote API every 10 seconds
- **Status Indicators**: LED indicators for WiFi, API, and Bluetooth status
- **Dummy Data Generation**: Built-in test data generation for development and testing
- **Robust Error Handling**: Automatic reconnection and error recovery mechanisms

## Hardware Requirements

- ESP32 Development Board
- Up to 6 HX711 Load Cell Amplifiers
- Load cells compatible with HX711
- Status LEDs (3x)

## Pin Configuration

### HX711 Load Cell Amplifiers
- **Sensor 1**: CLK=2, DOUT=3
- **Sensor 2**: CLK=4, DOUT=5
- **Sensor 3**: CLK=12, DOUT=13
- **Sensor 4**: CLK=14, DOUT=15
- **Sensor 5**: CLK=16, DOUT=17
- **Sensor 6**: CLK=18, DOUT=19

### Status LEDs
- **WiFi Status**: Pin 25
- **API Status**: Pin 26
- **Bluetooth Status**: Pin 27

## Building and Flashing

1. Install PlatformIO IDE or CLI
2. Clone this repository
3. Open the project in PlatformIO
4. Build and upload to ESP32:
   ```bash
   pio run --target upload
   ```

## Bluetooth Provisioning Protocol

The device communicates with a Unity mobile application using JSON commands over Bluetooth Serial.

### Commands from Unity App:
```json
{"command": "set_wifi", "ssid": "YourWiFiNetwork", "password": "YourPassword"}
{"command": "set_api", "api_key": "your-device-api-key"}
{"command": "get_status"}
{"command": "complete_setup"}
```

## API Integration

The device integrates with the Smart Bins API at `https://smart-bins-api-uay7w.ondigitalocean.app`

Sensor data is submitted every 10 seconds in JSON format with weight measurements from all enabled bins.

## Testing

The device includes dummy data generation for testing without physical sensors. Each bin simulates different fill patterns for realistic testing scenarios.
