#ifndef API_CLIENT_H
#define API_CLIENT_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "config.h"

class APIClient {
public:
    APIClient();
    void init();
    bool authenticate();
    bool submitSensorData(SensorReading* readings, int count);
    bool isAuthenticated();
    String getDeviceId();
    String getApiKey();
    String getApiUrl();
    void setCredentials(const String& apiKey, const String& apiUrl, const String& deviceId);

private:
    Preferences preferences;
    String apiKey;
    String apiUrl;
    String deviceId;
    bool authenticated;
    HTTPClient http;
    
    bool makeRequest(const String& endpoint, const String& method, const String& payload, String& response);
    String createSensorDataPayload(SensorReading* readings, int count);
    void loadCredentials();
    bool testConnection();
};

#endif // API_CLIENT_H
