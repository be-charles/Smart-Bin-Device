#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include "config.h"
#include "bluetooth_provisioning.h"
#include "sensor_manager.h"
#include "api_client.h"

// Global objects
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
    delay(100);  // Let serial stabilize
    
    Serial.println("\n=== Smart Bin Device Starting ===");
    Serial.printf("Firmware Version: %s\n", FIRMWARE_VERSION);
    Serial.println("Power optimization: 80MHz CPU frequency enabled");
    
    initializeDevice();
    printDeviceInfo();
    
    Serial.println("=== Setup Complete ===\n");
}

void loop() {
    // Update all modules
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
            #ifdef DEBUG_MODE
            Serial.println("Device in error state - restarting in 30 seconds...");
            #endif
            delay(30000);
            ESP.restart();
            break;
    }
    
    delay(100); // Small delay to prevent watchdog issues
}

void initializeDevice() {
    Serial.println("Initializing device components with power optimization...");
    
    // Initialize low-power components first
    Serial.println("Step 1: Initializing sensor manager...");
    sensorManager.init();
    delay(200);  // Let sensors stabilize
    
    Serial.println("Step 2: Initializing API client...");
    apiClient.init();
    delay(200);  // Let API client stabilize
    
    // Initialize high-power components last
    Serial.println("Step 3: Initializing Bluetooth provisioning...");
    btProvisioning.init();
    delay(500);  // Extra time for Bluetooth to stabilize
    
    Serial.println("All components initialized successfully");
    
    // Check if device is already configured
    if (btProvisioning.isSetupComplete()) {
        #ifdef DEBUG_MODE
        Serial.println("Device already configured, skipping provisioning");
        #endif
        changeState(STATE_WIFI_CONNECTING);
    } else {
        #ifdef DEBUG_MODE
        Serial.println("Device not configured, starting provisioning mode");
        #endif
        changeState(STATE_PROVISIONING);
    }
}

void handleProvisioning() {
    if (!btProvisioning.isActive()) {
        #ifdef DEBUG_MODE
        Serial.println("Starting Bluetooth provisioning...");
        #endif
        btProvisioning.start();
        btProvisioning.broadcastDeviceStatus("disconnected", "not_authenticated", "idle");
    }
    
    // Check if provisioning is complete
    if (btProvisioning.isSetupComplete()) {
        #ifdef DEBUG_MODE
        Serial.println("Provisioning completed, moving to WiFi connection");
        #endif
        changeState(STATE_WIFI_CONNECTING);
    }
    
    // Handle provisioning timeout
    if (millis() - lastStateChange > BLUETOOTH_TIMEOUT) {
        #ifdef DEBUG_MODE
        Serial.println("Provisioning timeout, restarting...");
        #endif
        ESP.restart();
    }
}

void handleWiFiConnection() {
    static unsigned long lastAttempt = 0;
    static int attemptCount = 0;
    
    if (millis() - lastAttempt > WIFI_RETRY_DELAY) {
        if (attemptCount < WIFI_MAX_RETRIES) {
            #ifdef DEBUG_MODE
            Serial.printf("WiFi connection attempt %d/%d\n", attemptCount + 1, WIFI_MAX_RETRIES);
            #endif
            connectToWiFi();
            attemptCount++;
            lastAttempt = millis();
            
            if (WiFi.status() == WL_CONNECTED) {
                #ifdef DEBUG_MODE
                Serial.printf("WiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());
                #endif
                btProvisioning.broadcastDeviceStatus("connected", "not_authenticated", "idle");
                changeState(STATE_API_AUTHENTICATING);
                return;
            }
        } else {
            #ifdef DEBUG_MODE
            Serial.println("WiFi connection failed after max retries");
            #endif
            btProvisioning.broadcastDeviceStatus("failed", "not_authenticated", "error");
            changeState(STATE_ERROR);
        }
    }
}

void handleAPIAuthentication() {
    static bool authAttempted = false;
    
    if (!authAttempted) {
        #ifdef DEBUG_MODE
        Serial.println("Attempting API authentication...");
        #endif
        
        if (apiClient.authenticate()) {
            #ifdef DEBUG_MODE
            Serial.println("API authentication successful");
            #endif
            btProvisioning.broadcastDeviceStatus("connected", "authenticated", "idle");
            changeState(STATE_OPERATING);
        } else {
            #ifdef DEBUG_MODE
            Serial.println("API authentication failed");
            #endif
            btProvisioning.broadcastDeviceStatus("connected", "failed", "error");
            changeState(STATE_ERROR);
        }
        
        authAttempted = true;
    }
}

void handleNormalOperation() {
    // Read sensors every 10 seconds
    if (millis() - lastSensorRead >= SENSOR_READ_INTERVAL) {
        #ifdef DEBUG_MODE
        Serial.println("Reading sensors and submitting data...");
        #endif
        
        // Update sensor readings
        sensorManager.update();
        SensorReading* readings = sensorManager.getAllReadings();
        
        // Print sensor data to serial (only in debug mode)
        #ifdef DEBUG_MODE
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
        #endif
        
        // Submit data to API
        if (apiClient.submitSensorData(readings, MAX_BINS)) {
            btProvisioning.broadcastDeviceStatus("connected", "authenticated", "reading");
        } else {
            #ifdef DEBUG_MODE
            Serial.println("Failed to submit sensor data");
            #endif
            btProvisioning.broadcastDeviceStatus("connected", "authenticated", "error");
        }
        
        lastSensorRead = millis();
    }
    
    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
        #ifdef DEBUG_MODE
        Serial.println("WiFi connection lost, attempting reconnection...");
        #endif
        btProvisioning.broadcastDeviceStatus("disconnected", "not_authenticated", "error");
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
