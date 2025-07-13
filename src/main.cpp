#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include "config.h"
#include "status_led.h"
#include "bluetooth_provisioning.h"
#include "sensor_manager.h"
#include "api_client.h"

// Global objects
StatusLED statusLED;
BluetoothProvisioning btProvisioning;
SensorManager sensorManager;
APIClient apiClient;
Preferences preferences;

// State management
DeviceState currentState = STATE_PROVISIONING;
unsigned long lastSensorRead = 0;
unsigned long lastStateChange = 0;

// Function declarations
void initializeDevice();
void handleProvisioning();
void handleWiFiConnection();
void handleAPIAuthentication();
void handleNormalOperation();
void connectToWiFi();
void printDeviceInfo();
void changeState(DeviceState newState);

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n=== Smart Bin Device Starting ===");
    Serial.printf("Firmware Version: %s\n", FIRMWARE_VERSION);
    
    initializeDevice();
    printDeviceInfo();
    
    Serial.println("=== Setup Complete ===\n");
}

void loop() {
    // Update all modules
    statusLED.update();
    btProvisioning.update();
    
    // State machine
    switch (currentState) {
        case STATE_PROVISIONING:
            handleProvisioning();
            break;
            
        case STATE_WIFI_CONNECTING:
            handleWiFiConnection();
            break;
            
        case STATE_API_AUTHENTICATING:
            handleAPIAuthentication();
            break;
            
        case STATE_OPERATING:
            handleNormalOperation();
            break;
            
        case STATE_ERROR:
            Serial.println("Device in error state - restarting in 30 seconds...");
            delay(30000);
            ESP.restart();
            break;
    }
    
    delay(100); // Small delay to prevent watchdog issues
}

void initializeDevice() {
    // Initialize all modules
    statusLED.init();
    btProvisioning.init();
    sensorManager.init();
    apiClient.init();
    
    // Check if device is already configured
    if (btProvisioning.isSetupComplete()) {
        Serial.println("Device already configured, skipping provisioning");
        changeState(STATE_WIFI_CONNECTING);
    } else {
        Serial.println("Device not configured, starting provisioning mode");
        changeState(STATE_PROVISIONING);
    }
}

void handleProvisioning() {
    if (!btProvisioning.isActive()) {
        Serial.println("Starting Bluetooth provisioning...");
        btProvisioning.start();
        statusLED.setBluetoothStatus(true);
    }
    
    // Check if provisioning is complete
    if (btProvisioning.isSetupComplete()) {
        Serial.println("Provisioning completed, moving to WiFi connection");
        statusLED.setBluetoothStatus(false);
        changeState(STATE_WIFI_CONNECTING);
    }
    
    // Handle provisioning timeout
    if (millis() - lastStateChange > BLUETOOTH_TIMEOUT) {
        Serial.println("Provisioning timeout, restarting...");
        ESP.restart();
    }
}

void handleWiFiConnection() {
    static unsigned long lastAttempt = 0;
    static int attemptCount = 0;
    
    if (millis() - lastAttempt > WIFI_RETRY_DELAY) {
        if (attemptCount < WIFI_MAX_RETRIES) {
            Serial.printf("WiFi connection attempt %d/%d\n", attemptCount + 1, WIFI_MAX_RETRIES);
            connectToWiFi();
            attemptCount++;
            lastAttempt = millis();
            
            if (WiFi.status() == WL_CONNECTED) {
                Serial.printf("WiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());
                statusLED.setWiFiStatus(true);
                changeState(STATE_API_AUTHENTICATING);
                return;
            }
        } else {
            Serial.println("WiFi connection failed after max retries");
            statusLED.blinkWiFi(5);
            changeState(STATE_ERROR);
        }
    }
}

void handleAPIAuthentication() {
    static bool authAttempted = false;
    
    if (!authAttempted) {
        Serial.println("Attempting API authentication...");
        
        if (apiClient.authenticate()) {
            Serial.println("API authentication successful");
            statusLED.setAPIStatus(true);
            changeState(STATE_OPERATING);
        } else {
            Serial.println("API authentication failed");
            statusLED.blinkAPI(5);
            changeState(STATE_ERROR);
        }
        
        authAttempted = true;
    }
}

void handleNormalOperation() {
    // Read sensors every 10 seconds
    if (millis() - lastSensorRead >= SENSOR_READ_INTERVAL) {
        Serial.println("Reading sensors and submitting data...");
        
        // Update sensor readings
        sensorManager.update();
        SensorReading* readings = sensorManager.getAllReadings();
        
        // Print sensor data to serial
        Serial.println("=== Sensor Readings ===");
        for (int i = 0; i < MAX_BINS; i++) {
            if (sensorManager.isSensorEnabled(i)) {
                Serial.printf("Bin %d: %.2f kg (Valid: %s)\n", 
                             readings[i].bin_id, 
                             readings[i].weight, 
                             readings[i].valid ? "Yes" : "No");
            }
        }
        Serial.println("=====================");
        
        // Submit data to API
        if (apiClient.submitSensorData(readings, MAX_BINS)) {
            statusLED.blinkAPI(1); // Quick blink to indicate successful transmission
        } else {
            Serial.println("Failed to submit sensor data");
            statusLED.blinkAPI(3); // Multiple blinks to indicate error
        }
        
        lastSensorRead = millis();
    }
    
    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi connection lost, attempting reconnection...");
        statusLED.setWiFiStatus(false);
        changeState(STATE_WIFI_CONNECTING);
    }
}

void connectToWiFi() {
    preferences.begin(NVS_NAMESPACE, true);
    String ssid = preferences.getString(NVS_WIFI_SSID, "");
    String password = preferences.getString(NVS_WIFI_PASSWORD, "");
    preferences.end();
    
    if (ssid.length() == 0) {
        Serial.println("No WiFi credentials found");
        return;
    }
    
    Serial.printf("Connecting to WiFi: %s\n", ssid.c_str());
    WiFi.begin(ssid.c_str(), password.c_str());
    
    // Wait for connection with timeout
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < WIFI_CONNECT_TIMEOUT) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
}

void printDeviceInfo() {
    Serial.println("\n=== Device Information ===");
    Serial.printf("Device Name: %s\n", DEVICE_NAME);
    Serial.printf("Firmware Version: %s\n", FIRMWARE_VERSION);
    Serial.printf("MAC Address: %s\n", WiFi.macAddress().c_str());
    Serial.printf("Supported Bins: %d\n", MAX_BINS);
    Serial.printf("Current State: %d\n", currentState);
    Serial.println("========================\n");
}

void changeState(DeviceState newState) {
    if (currentState != newState) {
        Serial.printf("State change: %d -> %d\n", currentState, newState);
        currentState = newState;
        lastStateChange = millis();
    }
}
