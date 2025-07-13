#include "bluetooth_provisioning.h"
#include <WiFi.h>
#include <HTTPClient.h>

BluetoothProvisioning::BluetoothProvisioning() {
    active = false;
    setupComplete = false;
    startTime = 0;
    inputBuffer = "";
}

void BluetoothProvisioning::init() {
    preferences.begin(NVS_NAMESPACE, false);
    
    // Generate unique device name using MAC address
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    deviceName = String(BT_DEVICE_NAME_PREFIX) + mac.substring(6);
    
    Serial.printf("Bluetooth device name: %s\n", deviceName.c_str());
    
    // Check if setup is already complete
    setupComplete = preferences.getBool(NVS_SETUP_COMPLETE, false);
    Serial.printf("Setup complete: %s\n", setupComplete ? "Yes" : "No");
}

void BluetoothProvisioning::start() {
    if (!active) {
        SerialBT.begin(deviceName);
        active = true;
        startTime = millis();
        Serial.printf("Bluetooth provisioning started: %s\n", deviceName.c_str());
        
        sendResponse("ready", "Device ready for provisioning");
    }
}

void BluetoothProvisioning::stop() {
    if (active) {
        SerialBT.end();
        active = false;
        Serial.println("Bluetooth provisioning stopped");
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
            Serial.println("Bluetooth provisioning timeout");
            stop();
            return;
        }
        
        // Handle incoming data
        handleIncomingData();
    }
}

void BluetoothProvisioning::handleIncomingData() {
    while (SerialBT.available()) {
        char c = SerialBT.read();
        
        if (c == '\n' || c == '\r') {
            if (inputBuffer.length() > 0) {
                Serial.printf("Received command: %s\n", inputBuffer.c_str());
                processCommand(inputBuffer);
                inputBuffer = "";
            }
        } else {
            inputBuffer += c;
            
            // Prevent buffer overflow
            if (inputBuffer.length() > BT_BUFFER_SIZE) {
                inputBuffer = "";
                sendResponse("error", "Command too long");
            }
        }
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
    JsonDocument response;
    response["status"] = status;
    response["message"] = message;
    
    String responseStr;
    serializeJson(response, responseStr);
    
    SerialBT.println(responseStr);
    Serial.printf("Sent response: %s\n", responseStr.c_str());
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
        SerialBT.println(responseStr);
        
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
        response["status"] = "api_authenticated";
        response["device_id"] = deviceId;
        
        String responseStr;
        serializeJson(response, responseStr);
        SerialBT.println(responseStr);
        
        Serial.println("API credentials saved successfully");
    } else {
        sendResponse("error", "API authentication failed");
    }
}

void BluetoothProvisioning::handleStatusCommand() {
    JsonDocument response;
    response["status"] = "status";
    response["device_name"] = deviceName;
    response["firmware_version"] = FIRMWARE_VERSION;
    response["setup_complete"] = setupComplete;
    response["mac_address"] = WiFi.macAddress().c_str();
    
    String responseStr;
    serializeJson(response, responseStr);
    SerialBT.println(responseStr);
}

void BluetoothProvisioning::handleCompleteSetupCommand() {
    // Verify all required credentials are present
    if (loadCredentials(NVS_WIFI_SSID).length() == 0 ||
        loadCredentials(NVS_API_KEY).length() == 0) {
        sendResponse("error", "Missing required credentials");
        return;
    }
    
    preferences.putBool(NVS_SETUP_COMPLETE, true);
    setupComplete = true;
    
    sendResponse("success", "Setup completed successfully");
    Serial.println("Device setup completed");
    
    // Stop Bluetooth after a short delay
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
