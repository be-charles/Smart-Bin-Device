# Always-Available Bluetooth Configuration Guide

## Overview

The Smart Bin device firmware has been updated to support **always-available Bluetooth connectivity** for device settings and configuration. This allows users to connect to devices at any time to adjust settings, calibrate sensors, and monitor device status without the previous 5-minute timeout limitation.

## Key Changes Made

### 1. **Dual-Mode Bluetooth System**

The Bluetooth system now operates in two distinct modes:

#### **Provisioning Mode** (Initial Setup)
- **Purpose**: First-time device configuration
- **Timeout**: 5 minutes (`BLUETOOTH_PROVISIONING_TIMEOUT`)
- **Behavior**: Stops after setup completion or timeout
- **Commands**: WiFi setup, API configuration, initial provisioning

#### **Settings Mode** (Always Available)
- **Purpose**: Ongoing device configuration and management
- **Timeout**: Configurable (default: no timeout)
- **Behavior**: Remains active during normal operation
- **Commands**: Sensor calibration, scale factor updates, status queries

### 2. **Configuration Constants**

New timeout configurations in `include/config.h`:

```cpp
#define BLUETOOTH_PROVISIONING_TIMEOUT 300000    // 5 minutes for initial provisioning
#define BLUETOOTH_SETTINGS_TIMEOUT 0              // 0 = no timeout (always available)
#define BLUETOOTH_INACTIVITY_TIMEOUT 1800000      // 30 minutes of inactivity before power optimization
```

### 3. **Enhanced Bluetooth Provisioning Class**

#### **New Methods Added:**
- `startSettingsMode()` - Start Bluetooth in settings mode
- `isInProvisioningMode()` - Check if in provisioning mode
- `isInSettingsMode()` - Check if in settings mode

#### **Updated Behavior:**
- Activity tracking with `lastActivity` timestamp
- Mode-specific timeout handling
- Persistent connection management

## How It Works

### **Device Startup Flow**

1. **Initial Boot**: Device starts in provisioning mode if not configured
2. **First Setup**: 5-minute timeout for initial WiFi/API configuration
3. **Normal Operation**: Automatically switches to settings mode
4. **Always Available**: Bluetooth remains active for ongoing configuration

### **Settings Mode Activation**

In normal operation (`STATE_OPERATING`), the device automatically starts Bluetooth settings mode:

```cpp
void handleNormalOperation() {
    // Start Bluetooth settings mode if not already active
    if (!btProvisioning.isActive()) {
        Serial.println("Starting Bluetooth settings mode for device configuration...");
        btProvisioning.startSettingsMode();
    }
    // ... rest of normal operation
}
```

### **Timeout Management**

The system now handles different timeouts based on mode:

```cpp
void BluetoothProvisioning::update() {
    // Check for timeout based on mode
    unsigned long timeout = 0;
    if (isProvisioningMode) {
        timeout = BLUETOOTH_PROVISIONING_TIMEOUT;  // 5 minutes
    } else if (isSettingsMode) {
        timeout = BLUETOOTH_SETTINGS_TIMEOUT;      // 0 = no timeout
        if (timeout == 0) {
            timeout = UINT32_MAX; // Effectively no timeout
        }
    }
    // ... timeout handling
}
```

## Available Commands in Settings Mode

### **Sensor Management**
- `set_scale_factor` - Update individual sensor scale factors
- `get_scale_factor` - Retrieve specific sensor scale factor
- `get_all_scale_factors` - Get all sensor data at once
- `calibrate_sensor` - Calibrate sensor with known weight

### **Network Configuration**
- `set_wifi` - Update WiFi credentials
- `set_api` - Update API settings
- `get_status` - Get device status and information

### **Setup Management**
- `complete_setup` - Finalize device setup (doesn't stop Bluetooth in settings mode)

## Power Consumption Considerations

### **Current Draw**
- **BLE Advertising**: ~15-20mA additional power consumption
- **Connected State**: ~10-15mA during active connection
- **Optimizations**: Low power BLE settings (-3dBm transmission power)

### **Power Optimization Features**

1. **Inactivity Timeout**: After 30 minutes of no activity, system can optimize power
2. **Low Power BLE**: Transmission power set to -3dBm for efficiency
3. **Smart Advertising**: Longer intervals during normal operation

### **Configuration Options**

To modify power behavior, adjust these constants in `config.h`:

```cpp
// Set to 0 for always available (no timeout)
#define BLUETOOTH_SETTINGS_TIMEOUT 0

// Adjust inactivity timeout (0 to disable)
#define BLUETOOTH_INACTIVITY_TIMEOUT 1800000  // 30 minutes

// Or set a specific timeout (in milliseconds)
#define BLUETOOTH_SETTINGS_TIMEOUT 3600000    // 1 hour timeout
```

## Usage Examples

### **Mobile App Integration**

The mobile app can now connect to devices at any time:

```csharp
// Connect to device anytime during normal operation
BLEManager.Instance.ConnectToDevice(device);

// Send settings commands
var command = ESP32Command.SetScaleFactor(binId, newScaleFactor);
BLEManager.Instance.SendCommand(command);
```

### **Sensor Calibration**

Users can calibrate sensors whenever needed:

```json
{
  "command": "calibrate_sensor",
  "bin_id": 0,
  "known_weight": 1.5
}
```

### **Scale Factor Updates**

Individual scale factors can be updated in real-time:

```json
{
  "command": "set_scale_factor",
  "bin_id": 2,
  "scale_factor": 1234.56
}
```

## Benefits

### **For Users**
- **No Time Pressure**: Connect and configure devices at any time
- **Real-time Adjustments**: Make sensor adjustments during operation
- **Convenient Maintenance**: No need to restart devices for configuration
- **Flexible Scheduling**: Configure devices when convenient

### **For Maintenance**
- **Remote Configuration**: Adjust settings without physical access
- **Live Calibration**: Calibrate sensors with actual weights
- **Status Monitoring**: Check device status anytime
- **Troubleshooting**: Access device information for support

### **For Development**
- **Testing Flexibility**: Test configurations without time constraints
- **Debugging Support**: Access device logs and status continuously
- **Iterative Development**: Make and test changes quickly

## Migration from Previous Version

### **Automatic Migration**
- Existing devices will automatically enable settings mode after firmware update
- No configuration changes required for basic operation
- Previous provisioning flow remains unchanged

### **Configuration Updates**
If you want to modify timeout behavior:

1. Update `BLUETOOTH_SETTINGS_TIMEOUT` in `config.h`
2. Rebuild and flash firmware
3. Device will use new timeout settings

## Troubleshooting

### **Connection Issues**
- **Device Not Found**: Ensure device is in normal operation state
- **Connection Timeout**: Check Bluetooth range and interference
- **Command Failures**: Verify device is not in error state

### **Power Concerns**
- **High Power Usage**: Consider enabling inactivity timeout
- **Battery Drain**: Monitor power consumption and adjust settings
- **Optimization**: Use longer advertising intervals if needed

### **Debug Information**

Enable debug output to monitor Bluetooth behavior:

```cpp
#define DEBUG_MODE true  // In config.h
```

Monitor serial output for:
- Bluetooth mode transitions
- Connection status
- Command processing
- Timeout events

## Future Enhancements

### **Planned Features**
1. **Dynamic Power Management**: Automatic power optimization based on usage
2. **Connection Priority**: Prioritize certain connections over others
3. **Scheduled Availability**: Configure specific availability windows
4. **Remote Control**: API-based Bluetooth control

### **Configuration Profiles**
Future versions may support:
- **Always On**: No timeouts, maximum availability
- **Power Saver**: Longer timeouts, optimized power usage
- **Scheduled**: Time-based availability windows
- **On-Demand**: Button-activated Bluetooth

## Conclusion

The always-available Bluetooth functionality provides a significant improvement in device usability and maintenance capabilities. Users can now connect to Smart Bin devices at any time to adjust settings, calibrate sensors, and monitor status without the previous 5-minute limitation.

The system maintains power efficiency through intelligent timeout management and low-power BLE settings while providing the flexibility needed for real-world deployment and maintenance scenarios.

For technical support or questions about implementation, refer to the device logs and this documentation for troubleshooting guidance.
