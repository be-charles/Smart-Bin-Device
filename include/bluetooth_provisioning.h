#ifndef BLUETOOTH_PROVISIONING_H
#define BLUETOOTH_PROVISIONING_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "config.h"

// Forward declaration
class SensorManager;

// BLE Service and Characteristic UUIDs
#define SERVICE_UUID        "12345678-1234-1234-1234-123456789abc"
#define COMMAND_CHAR_UUID   "12345678-1234-1234-1234-123456789abd"
#define RESPONSE_CHAR_UUID  "12345678-1234-1234-1234-123456789abe"
#define STATUS_CHAR_UUID    "12345678-1234-1234-1234-123456789abf"

class BluetoothProvisioning : public BLEServerCallbacks, public BLECharacteristicCallbacks {
public:
    BluetoothProvisioning();
    void init();
    void start();
    void startSettingsMode();
    void stop();
    bool isActive();
    bool isSetupComplete();
    bool isInProvisioningMode();
    bool isInSettingsMode();
    void update(); // Call in main loop
    void broadcastDeviceStatus(const String& wifiStatus, const String& apiStatus, const String& sensorStatus);
    void setSensorManager(SensorManager* sensorMgr);

    // BLE Callbacks
    void onConnect(BLEServer* pServer) override;
    void onDisconnect(BLEServer* pServer) override;
    void onWrite(BLECharacteristic* pCharacteristic) override;

private:
    BLEServer* pServer;
    BLEService* pService;
    BLECharacteristic* pCommandCharacteristic;
    BLECharacteristic* pResponseCharacteristic;
    BLECharacteristic* pStatusCharacteristic;
    BLEAdvertising* pAdvertising;
    
    Preferences preferences;
    bool active;
    bool setupComplete;
    bool deviceConnected;
    String deviceName;
    unsigned long startTime;
    unsigned long lastActivity;
    bool isProvisioningMode;
    bool isSettingsMode;
    SensorManager* pSensorManager;
    
    void setupBLEServer();
    void processCommand(const String& command);
    void sendResponse(const String& status, const String& message);
    void handleWiFiCommand(JsonDocument& doc);
    void handleAPICommand(JsonDocument& doc);
    void handleStatusCommand();
    void handleCompleteSetupCommand();
    void handleSetScaleFactorCommand(JsonDocument& doc);
    void handleGetScaleFactorCommand(JsonDocument& doc);
    void handleGetAllScaleFactorsCommand();
    void handleCalibrateSensorCommand(JsonDocument& doc);
    bool testWiFiConnection(const String& ssid, const String& password);
    bool testAPIConnection(const String& apiKey, const String& apiUrl);
    String generateDeviceId();
    void saveCredentials(const String& key, const String& value);
    String loadCredentials(const String& key);
};

#endif // BLUETOOTH_PROVISIONING_H
