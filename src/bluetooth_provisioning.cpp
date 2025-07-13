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

// Placeholder implementations for remaining methods
void BluetoothProvisioning::handleWiFiCommand(JsonDocument& doc) {
    sendResponse("error", "Not implemented");
}

void BluetoothProvisioning::handleAPICommand(JsonDocument& doc) {
    sendResponse("error", "Not implemented");
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
    sendResponse("error", "Not implemented");
}

bool BluetoothProvisioning::testWiFiConnection(const String& ssid, const String& password) {
    return false;
}

bool BluetoothProvisioning::testAPIConnection(const String& apiKey, const String& apiUrl) {
    return false;
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
