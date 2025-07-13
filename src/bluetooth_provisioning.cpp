#include "bluetooth_provisioning.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "esp_system.h"

BluetoothProvisioning::BluetoothProvisioning() {
    active = false;
    setupComplete = false;
    deviceConnected = false;
    startTime = 0;
    pServer = nullptr;
    pService = nullptr;
    pCommandCharacteristic = nullptr;
    pResponseCharacteristic = nullptr;
    pStatusCharacteristic = nullptr;
    pAdvertising = nullptr;
}

void BluetoothProvisioning::init() {
    preferences.begin(NVS_NAMESPACE, false);
    
    // Generate unique device name using MAC address
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    deviceName = String(BT_DEVICE_NAME_PREFIX) + mac.substring(6);
    
    Serial.printf("BLE device name: %s\n", deviceName.c_str());
    
    // Check if setup is already complete
    setupComplete = preferences.getBool(NVS_SETUP_COMPLETE, false);
    Serial.printf("Setup complete: %s\n", setupComplete ? "Yes" : "No");
}

void BluetoothProvisioning::start() {
    if (!active) {
        Serial.println("Starting BLE provisioning...");
        
        // Initialize BLE
        Serial.println("Initializing BLE...");
        BLEDevice::init(deviceName.c_str());
        
        // Set BLE power to low level for power efficiency
        esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, ESP_PWR_LVL_N3);
        Serial.println("BLE power set to -3dBm for power efficiency");
        
        setupBLEServer();
        Serial.println("BLE initialization completed successfully");
        
        active = true;
        startTime = millis();
        Serial.printf("BLE provisioning started: %s\n", deviceName.c_str());
        
        sendResponse("ready", "Device ready for provisioning");
    }
}

void BluetoothProvisioning::stop() {
    if (active) {
        if (pAdvertising) {
            pAdvertising->stop();
        }
        BLEDevice::deinit(false);
        active = false;
        deviceConnected = false;
        Serial.println("BLE provisioning stopped");
    }
}

bool BluetoothProvisioning::isActive() {
    return active;
}

bool BluetoothProvisioning::isSetupComplete() {
    return setupComplete;
}

void BluetoothProvisioning::update() {
    if (active) {
        // Check for timeout
        if (millis() - startTime > BLUETOOTH_TIMEOUT) {
            Serial.println("BLE provisioning timeout");
            stop();
            return;
        }
        
        // Restart advertising if disconnected
        if (!deviceConnected && pAdvertising) {
            pAdvertising->start();
        }
    }
}

void BluetoothProvisioning::setupBLEServer() {
    // Create BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(this);
    
    // Create BLE Service
    pService = pServer->createService(SERVICE_UUID);
    
    // Create Command Characteristic (Write)
    pCommandCharacteristic = pService->createCharacteristic(
        COMMAND_CHAR_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );
    pCommandCharacteristic->setCallbacks(this);
    
    // Create Response Characteristic (Read/Notify)
    pResponseCharacteristic = pService->createCharacteristic(
        RESPONSE_CHAR_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pResponseCharacteristic->addDescriptor(new BLE2902());
    
    // Create Status Characteristic (Notify)
    pStatusCharacteristic = pService->createCharacteristic(
        STATUS_CHAR_UUID,
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pStatusCharacteristic->addDescriptor(new BLE2902());
    
    // Start the service
    pService->start();
    
    // Start advertising
    pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    pAdvertising->start();
    
    Serial.println("BLE Server setup complete, advertising started");
}

// BLE Server Callbacks
void BluetoothProvisioning::onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("BLE Client connected");
}

void BluetoothProvisioning::onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("BLE Client disconnected");
    
    // Restart advertising
    if (active && pAdvertising) {
        delay(500);
        pAdvertising->start();
        Serial.println("BLE Advertising restarted");
    }
}

void BluetoothProvisioning::onWrite(BLECharacteristic* pCharacteristic) {
    if (pCharacteristic == pCommandCharacteristic) {
        String command = pCharacteristic->getValue().c_str();
        Serial.printf("Received BLE command: %s\n", command.c_str());
        processCommand(command);
    }
}

void BluetoothProvisioning::processCommand(const String& command) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, command);
    
    if (error) {
        sendResponse("error", "Invalid JSON format");
        return;
    }
    
    String cmd = doc["command"];
    
    if (cmd == "set_wifi") {
        handleWiFiCommand(doc);
    } else if (cmd == "set_api") {
        handleAPICommand(doc);
    } else if (cmd == "get_status") {
        handleStatusCommand();
    } else if (cmd == "complete_setup") {
        handleCompleteSetupCommand();
    } else {
        sendResponse("error", "Unknown command");
    }
}

void BluetoothProvisioning::sendResponse(const String& status, const String& message) {
    if (!deviceConnected || !pResponseCharacteristic) return;
    
    JsonDocument response;
    response["status"] = status;
    response["message"] = message;
    
    String responseStr;
    serializeJson(response, responseStr);
    
    pResponseCharacteristic->setValue(responseStr.c_str());
    pResponseCharacteristic->notify();
    
    Serial.printf("Sent BLE response: %s\n", responseStr.c_str());
}

void BluetoothProvisioning::handleWiFiCommand(JsonDocument& doc) {
    String ssid = doc["ssid"];
    String password = doc["password"];
    
    if (ssid.length() == 0) {
        sendResponse("error", "SSID is required");
        return;
    }
    
    Serial.printf("Testing WiFi connection to: %s\n", ssid.c_str());
    
    if (testWiFiConnection(ssid, password)) {
        saveCredentials(NVS_WIFI_SSID, ssid);
        saveCredentials(NVS_WIFI_PASSWORD, password);
        
        JsonDocument response;
        response["status"] = "wifi_connected";
        response["ip_address"] = WiFi.localIP().toString();
        
        String responseStr;
        serializeJson(response, responseStr);
        
        if (pResponseCharacteristic) {
            pResponseCharacteristic->setValue(responseStr.c_str());
            pResponseCharacteristic->notify();
        }
        
        Serial.println("WiFi credentials saved successfully");
    } else {
        sendResponse("error", "WiFi connection failed");
    }
}

void BluetoothProvisioning::handleAPICommand(JsonDocument& doc) {
    String apiKey = doc["api_key"];
    String apiUrl = doc["api_url"];
    
    if (apiKey.length() == 0) {
        sendResponse("error", "API key is required");
        return;
    }
    
    // Use default API URL if not provided
    if (apiUrl.length() == 0) {
        apiUrl = API_BASE_URL;
    }
    
    Serial.printf("Testing API connection with key: %s...\n", apiKey.substring(0, 8).c_str());
    
    if (testAPIConnection(apiKey, apiUrl)) {
        saveCredentials(NVS_API_KEY, apiKey);
        saveCredentials(NVS_API_URL, apiUrl);
        
        String deviceId = generateDeviceId();
        saveCredentials(NVS_DEVICE_ID, deviceId);
        
        JsonDocument response;
        response["status"] = "api_connected";
        response["device_id"] = deviceId;
        
        String responseStr;
        serializeJson(response, responseStr);
        
        if (pResponseCharacteristic) {
            pResponseCharacteristic->setValue(responseStr.c_str());
            pResponseCharacteristic->notify();
        }
        
        Serial.println("API credentials saved successfully");
    } else {
        sendResponse("error", "API authentication failed");
    }
}

void BluetoothProvisioning::handleStatusCommand() {
    JsonDocument response;
    response["status"] = "device_info";
    response["device_name"] = deviceName;
    response["setup_complete"] = setupComplete;
    response["mac_address"] = WiFi.macAddress();
    response["wifi_connected"] = (WiFi.status() == WL_CONNECTED);
    
    if (WiFi.status() == WL_CONNECTED) {
        response["ip_address"] = WiFi.localIP().toString();
    }
    
    String responseStr;
    serializeJson(response, responseStr);
    
    if (pResponseCharacteristic) {
        pResponseCharacteristic->setValue(responseStr.c_str());
        pResponseCharacteristic->notify();
    }
}

void BluetoothProvisioning::handleCompleteSetupCommand() {
    // Verify all required credentials are present
    if (loadCredentials(NVS_WIFI_SSID).length() == 0 ||
        loadCredentials(NVS_API_KEY).length() == 0) {
        sendResponse("error", "WiFi and API credentials required");
        return;
    }
    
    setupComplete = true;
    preferences.putBool(NVS_SETUP_COMPLETE, true);
    
    sendResponse("success", "Setup completed successfully");
    
    // Stop BLE after a short delay
    delay(1000);
    stop();
}

bool BluetoothProvisioning::testWiFiConnection(const String& ssid, const String& password) {
    WiFi.disconnect();
    WiFi.begin(ssid.c_str(), password.c_str());
    
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < WIFI_CONNECT_TIMEOUT) {
        delay(500);
        Serial.print(".");
    }
    
    bool connected = (WiFi.status() == WL_CONNECTED);
    Serial.printf("\nWiFi test result: %s\n", connected ? "SUCCESS" : "FAILED");
    
    return connected;
}

bool BluetoothProvisioning::testAPIConnection(const String& apiKey, const String& apiUrl) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected, cannot test API");
        return false;
    }
    
    HTTPClient http;
    http.begin(apiUrl + "/health");
    http.addHeader("Authorization", "Bearer " + apiKey);
    http.setTimeout(API_REQUEST_TIMEOUT);
    
    int httpResponseCode = http.GET();
    bool success = (httpResponseCode == 200);
    
    Serial.printf("API test result: %d (%s)\n", httpResponseCode, success ? "SUCCESS" : "FAILED");
    
    http.end();
    return success;
}

String BluetoothProvisioning::generateDeviceId() {
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    mac.toLowerCase();
    return "smartbin_" + mac;
}

void BluetoothProvisioning::saveCredentials(const String& key, const String& value) {
    preferences.putString(key.c_str(), value);
    Serial.printf("Saved credential: %s\n", key.c_str());
}

String BluetoothProvisioning::loadCredentials(const String& key) {
    return preferences.getString(key.c_str(), "");
}

void BluetoothProvisioning::broadcastDeviceStatus(const String& wifiStatus, const String& apiStatus, const String& sensorStatus) {
    if (!active || !deviceConnected || !pStatusCharacteristic) return;
    
    JsonDocument statusDoc;
    statusDoc["type"] = "device_status";
    statusDoc["wifi_status"] = wifiStatus;
    statusDoc["api_status"] = apiStatus;
    statusDoc["sensor_status"] = sensorStatus;
    statusDoc["ble_status"] = "active";
    statusDoc["timestamp"] = millis();
    
    String statusStr;
    serializeJson(statusDoc, statusStr);
    
    pStatusCharacteristic->setValue(statusStr.c_str());
    pStatusCharacteristic->notify();
    
    #ifdef DEBUG_MODE
    Serial.printf("BLE Status broadcast: %s\n", statusStr.c_str());
    #endif
}
