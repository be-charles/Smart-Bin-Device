#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Device Configuration
#define DEVICE_NAME "SmartBin"
#define FIRMWARE_VERSION "1.0.0"
#define MAX_BINS 6

// Testing Configuration
#define TESTING_MODE false  // Set to false when actual sensors are connected

// GPIO Pin Definitions for HX711 Load Cells
// Each HX711 requires 2 pins: CLK (Clock) and DOUT (Data)
// Note: GPIO 2 is reserved for built-in LED
#define HX711_1_CLK_PIN 4
#define HX711_1_DOUT_PIN 5
#define HX711_2_CLK_PIN 12
#define HX711_2_DOUT_PIN 13
#define HX711_3_CLK_PIN 14
#define HX711_3_DOUT_PIN 15
#define HX711_4_CLK_PIN 16
#define HX711_4_DOUT_PIN 17
#define HX711_5_CLK_PIN 18
#define HX711_5_DOUT_PIN 19
#define HX711_6_CLK_PIN 21
#define HX711_6_DOUT_PIN 22

// Built-in LED Pin for Heartbeat
#define BUILTIN_LED_PIN 2  // Built-in blue LED on ESP32 WROOM-32 dev boards

// Heartbeat Timing Configuration
#define HEARTBEAT_SLOW_INTERVAL 2000    // Normal operation (2 seconds)
#define HEARTBEAT_MEDIUM_INTERVAL 1000  // Connecting/authenticating (1 second)
#define HEARTBEAT_FAST_INTERVAL 500     // Error state (0.5 seconds)
#define HEARTBEAT_PROVISION_INTERVAL 300 // Provisioning mode (0.3 seconds)
#define HEARTBEAT_PULSE_ON_TIME 100     // LED on time for pulse patterns

// Timing Configuration
#define SENSOR_READ_INTERVAL 1000  // 10 seconds in milliseconds
#define WIFI_CONNECT_TIMEOUT 30000  // 30 seconds
#define API_REQUEST_TIMEOUT 10000   // 10 seconds
#define BLUETOOTH_PROVISIONING_TIMEOUT 300000    // 5 minutes for initial provisioning
#define BLUETOOTH_SETTINGS_TIMEOUT 0              // 0 = no timeout for settings mode (always available)
#define BLUETOOTH_INACTIVITY_TIMEOUT 1800000      // 30 minutes of inactivity before power optimization

// WiFi Configuration
#define WIFI_MAX_RETRIES 3
#define WIFI_RETRY_DELAY 5000

// API Configuration
#define API_BASE_URL "https://smart-bins-api-uay7w.ondigitalocean.app/smart-bins-api2"
#define API_SENSOR_DATA_ENDPOINT "/api/v1/sensor-data"
#define MAX_API_RETRIES 3
#define API_RETRY_DELAY 2000

// Bluetooth Configuration
#define BT_DEVICE_NAME_PREFIX "SmartBin_"
#define BT_BUFFER_SIZE 512

// NVS Storage Keys
#define NVS_NAMESPACE "smartbin"
#define NVS_WIFI_SSID "wifi_ssid"
#define NVS_WIFI_PASSWORD "wifi_pass"
#define NVS_API_KEY "api_key"
#define NVS_API_URL "api_url"
#define NVS_DEVICE_ID "device_id"
#define NVS_SETUP_COMPLETE "setup_done"
#define NVS_SCALE_FACTOR_PREFIX "scale_"  // Will be used as "scale_0", "scale_1", etc.

// Sensor Configuration
#define HX711_DEFAULT_SCALE_FACTOR 1000.0  // Default calibration factor
#define WEIGHT_SMOOTHING_SAMPLES 3
#define MIN_WEIGHT_CHANGE 0.1      // Minimum weight change to consider significant (kg)
#define SENSOR_DETECTION_TIMEOUT 2000  // Timeout for sensor detection (ms)
#define MIN_REQUIRED_SENSORS 1     // Minimum number of sensors required to operate

// Default scale factors for each sensor (used if NVS is empty)
#define HX711_DEFAULT_SCALE_FACTORS { 140400, 1000.0, 1000.0, 1000.0, 1000.0, 1000.0 }

// Data Buffer Configuration
#define MAX_BUFFERED_READINGS 100
#define BUFFER_SAVE_INTERVAL 60000  // Save buffer to NVS every minute

// Device States
enum DeviceState {
    STATE_PROVISIONING,
    STATE_WIFI_CONNECTING,
    STATE_API_AUTHENTICATING,
    STATE_OPERATING,
    STATE_ERROR
};

// Sensor Data Structure
struct SensorReading {
    int bin_id;
    float weight;
    unsigned long timestamp;
    bool valid;
};

// API Response Structure
struct ApiResponse {
    bool success;
    int status_code;
    String message;
};

#endif // CONFIG_H
