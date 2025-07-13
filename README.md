# Smart Bin Device - ESP32 Weight-Based Inventory Tracking

This project implements a smart bin system using ESP32 microcontrollers with HX711 load cell amplifiers to track inventory levels based on weight measurements.

## Features

- **Bluetooth Provisioning**: Configure WiFi credentials and API settings via Bluetooth from a Unity mobile application
- **Multi-Sensor Support**: Supports up to 6 HX711 load cell amplifiers per device
- **Real-time Data Transmission**: Sends sensor data to remote API every 10 seconds
- **Built-in LED Status Indicator**: Visual feedback using ESP32's built-in blue LED
- **Dummy Data Generation**: Built-in test data generation for development and testing
- **Power Optimization**: Optimized for external USB power with low-power settings
- **Robust Error Handling**: Automatic reconnection and error recovery mechanisms

## Hardware Requirements

- **ESP32 WROOM-32 Development Board** (with built-in blue LED on GPIO 2)
- **External USB Power Supply** (2A+ recommended for stable operation)
- **Up to 6 HX711 Load Cell Amplifiers**
- **Load cells compatible with HX711**

## Pin Configuration

### HX711 Load Cell Amplifiers
- **Sensor 1**: CLK=4, DOUT=5
- **Sensor 2**: CLK=12, DOUT=13
- **Sensor 3**: CLK=14, DOUT=15
- **Sensor 4**: CLK=16, DOUT=17
- **Sensor 5**: CLK=18, DOUT=19
- **Sensor 6**: CLK=21, DOUT=22

### Built-in LED Status Indicator
- **Status LED**: GPIO 2 (Built-in blue LED on ESP32 dev board)

## LED Status Patterns

The built-in blue LED provides visual feedback about the device's operational state:

| Device State | LED Pattern | Description |
|--------------|-------------|-------------|
| **Provisioning** | Fast blink (300ms) | Device waiting for Bluetooth setup |
| **WiFi Connecting** | Medium blink (1000ms) | Attempting to connect to WiFi |
| **API Authenticating** | Medium blink (1000ms) | Authenticating with remote API |
| **Normal Operation** | Slow pulse (2000ms) | Device operating normally, reading sensors |
| **Error State** | LED OFF | Critical error occurred, device will restart |

### LED Pattern Guide
- **Fast Blink**: Rapid on/off cycles - device needs configuration
- **Medium Blink**: Steady on/off cycles - device connecting to services
- **Slow Pulse**: Brief flash every 2 seconds - normal operation
- **LED Off**: No light at all - error condition detected

## Device States

The device operates in five distinct states:

1. **STATE_PROVISIONING**: Waiting for Bluetooth configuration from mobile app
2. **STATE_WIFI_CONNECTING**: Attempting to connect to configured WiFi network
3. **STATE_API_AUTHENTICATING**: Authenticating with the remote API server
4. **STATE_OPERATING**: Normal operation - reading sensors and transmitting data
5. **STATE_ERROR**: Critical error state - device will restart after 30 seconds

## Power Management

- **External USB Power Required**: Device optimized for stable external power supply
- **CPU Frequency**: 80MHz (reduced from 240MHz for power efficiency)
- **WiFi Power**: 8.5dBm (reduced for power efficiency)
- **BLE Power**: -3dBm (reduced for power efficiency)
- **Brownout Detection**: Uses ESP32 default levels (~2.77V)

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

### Device Information
- **Device Name**: `SmartBin_[MAC_ADDRESS]` (e.g., SmartBin_A1B2C3)
- **Service UUID**: Custom BLE service for provisioning
- **Timeout**: 5 minutes for provisioning mode

### Commands from Unity App:
```json
{"command": "set_wifi", "ssid": "YourWiFiNetwork", "password": "YourPassword"}
{"command": "set_api", "api_key": "your-device-api-key", "api_url": "optional-custom-url"}
{"command": "get_status"}
{"command": "complete_setup"}
```

### Response Format:
```json
{"status": "success|error|wifi_connected|api_connected", "message": "description"}
```

## API Integration

The device integrates with the Smart Bins API at:
```
https://smart-bins-api-uay7w.ondigitalocean.app/smart-bins-api2
```

### Data Transmission
- **Frequency**: Every 10 seconds
- **Format**: JSON with weight measurements from all enabled bins
- **Authentication**: Bearer token authentication
- **Retry Logic**: Automatic retry on failure with exponential backoff

### Sample Data Format:
```json
{
  "device_id": "smartbin_a1b2c3d4e5f6",
  "timestamp": 1642678800,
  "sensors": [
    {"bin_id": 0, "weight": 2.45, "valid": true},
    {"bin_id": 1, "weight": 1.23, "valid": true}
  ]
}
```

## Testing Mode

The device includes dummy data generation for testing without physical sensors:

- **Enable**: Set `TESTING_MODE true` in `config.h`
- **Behavior**: Each bin simulates different fill patterns
- **Data**: Realistic weight variations with noise simulation
- **Validation**: All dummy readings marked as valid

## Troubleshooting

### LED Pattern Diagnosis

| Issue | LED Pattern | Solution |
|-------|-------------|----------|
| Device not configured | Fast blink | Use mobile app to configure WiFi and API |
| WiFi connection failed | Medium blink (stuck) | Check WiFi credentials, signal strength |
| API authentication failed | Medium blink → LED off | Verify API key and internet connection |
| Normal operation | Slow pulse every 2 seconds | Device working correctly |
| Critical error | LED completely off | Check power supply, device will restart |

### Common Issues

1. **LED not blinking at all**
   - Check external USB power supply (2A+ recommended)
   - Verify ESP32 is properly connected and powered
   - Check if device is in error state (LED off = error)

2. **Fast blinking won't stop**
   - Device needs Bluetooth provisioning
   - Use Unity mobile app to configure WiFi and API settings
   - Ensure Bluetooth is enabled on mobile device

3. **Medium blinking stuck**
   - WiFi connection issues: Check SSID/password, signal strength
   - API authentication issues: Verify API key and internet connectivity

4. **No sensor data**
   - Check HX711 wiring and connections
   - Verify load cells are properly connected
   - Enable testing mode for dummy data validation

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## Support

For issues and questions:
- Check the troubleshooting section above
- Review LED patterns for device status
- Check serial output for detailed debugging information
