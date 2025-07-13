#ifndef BLUETOOTH_PROVISIONING_H
#define BLUETOOTH_PROVISIONING_H

#include <Arduino.h>
#include <BluetoothSerial.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "config.h"

class BluetoothProvisioning {
public:
    BluetoothProvisioning();
    void init();
    void start();
    void stop();
    bool isActive();
    void handleIncomingData();
    bool isSetupComplete();
    void update(); // Call in main loop

private:
    BluetoothSerial SerialBT;
    Preferences preferences;
    bool active;
    bool setupComplete;
    String deviceName;
    String inputBuffer;
    unsigned long startTime;
    
    void processCommand(const String& command);
    void sendResponse(const String& status, const String& message);
    void handleWiFiCommand(JsonDocument& doc);
    void handleAPICommand(JsonDocument& doc);
    void handleStatusCommand();
    void handleCompleteSetupCommand();
    bool testWiFiConnection(const String& ssid, const String& password);
    bool testAPIConnection(const String& apiKey, const String& apiUrl);
    String generateDeviceId();
    void saveCredentials(const String& key, const String& value);
    String loadCredentials(const String& key);
};

#endif // BLUETOOTH_PROVISIONING_H
