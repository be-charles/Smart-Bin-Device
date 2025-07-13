#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Device Configuration
#define DEVICE_NAME "SmartBin"
#define FIRMWARE_VERSION "1.0.0"
#define MAX_BINS 6

// Testing Configuration
#define TESTING_MODE true  // Set to false when actual sensors are connected

// GPIO Pin Definitions for HX711 Load Cells
// Each HX711 requires 2 pins: CLK (Clock) and DOUT (Data)
#define HX711_1_CLK_PIN 2
#define HX711_1_DOUT_PIN 3
#define HX711_2_CLK_PIN 4
#define HX711_2_DOUT_PIN 5
#define HX711_3_CLK_PIN 12
#define HX711_3_DOUT_PIN 13
#define HX711_4_CLK_PIN 14
#define HX711_4_DOUT_PIN 15
#define HX711_5_CLK_PIN 16
#define HX711_5_DOUT_PIN 17
#define HX711_6_CLK_PIN 18
#define HX711_6_DOUT_PIN 19

// Status LED Pins - REMOVED for size optimization
// Status now sent via Bluetooth instead of LEDs

// Timing Configuration
#define SENSOR_READ_INTERVAL 10000  // 10 seconds in milliseconds
#define WIFI_CONNECT_TIMEOUT 30000  // 30 seconds
#define API_REQUEST_TIMEOUT 10000   // 10 seconds
#define BLUETOOTH_TIMEOUT 300000    // 5 minutes for provisioning

// WiFi Configuration
#define WIFI_MAX_RETRIES 3
#define WIFI_RETRY_DELAY 5000

// API Configuration
#define API_BASE_URL "https://smart-bins-api-uay7w.ondigitalocean.app"
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

// Sensor Configuration
#define HX711_SCALE_FACTOR 1000.0  // Calibration factor (adjust based on actual calibration)
#define WEIGHT_SMOOTHING_SAMPLES 5
#define MIN_WEIGHT_CHANGE 0.1      // Minimum weight change to consider significant (kg)

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
