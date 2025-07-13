#include "api_client.h"
#include <WiFi.h>

APIClient::APIClient() {
    authenticated = false;
    apiKey = "";
    apiUrl = "";
    deviceId = "";
}

void APIClient::init() {
    preferences.begin(NVS_NAMESPACE, true); // Read-only mode
    loadCredentials();
    Serial.println("API Client initialized");
}

bool APIClient::authenticate() {
    if (apiKey.length() == 0 || apiUrl.length() == 0) {
        Serial.println("API credentials not configured");
        return false;
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected, cannot authenticate");
        return false;
    }
    
    Serial.println("Authenticating with API...");
    
    // Test connection with a simple health check
    authenticated = testConnection();
    
    if (authenticated) {
        Serial.println("API authentication successful");
    } else {
        Serial.println("API authentication failed");
    }
    
    return authenticated;
}

bool APIClient::submitSensorData(SensorReading* readings, int count) {
    if (!authenticated) {
        Serial.println("Not authenticated, cannot submit data");
        return false;
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected, cannot submit data");
        return false;
    }
    
    String payload = createSensorDataPayload(readings, count);
    String response;
    
    Serial.printf("Submitting sensor data: %s\n", payload.c_str());
    
    bool success = makeRequest(API_SENSOR_DATA_ENDPOINT, "POST", payload, response);
    
    if (success) {
        Serial.printf("Sensor data submitted successfully: %s\n", response.c_str());
    } else {
        Serial.printf("Failed to submit sensor data: %s\n", response.c_str());
    }
    
    return success;
}

bool APIClient::isAuthenticated() {
    return authenticated;
}

String APIClient::getDeviceId() {
    return deviceId;
}

String APIClient::getApiKey() {
    return apiKey;
}

String APIClient::getApiUrl() {
    return apiUrl;
}

void APIClient::setCredentials(const String& newApiKey, const String& newApiUrl, const String& newDeviceId) {
    apiKey = newApiKey;
    apiUrl = newApiUrl;
    deviceId = newDeviceId;
    authenticated = false; // Reset authentication status
}

bool APIClient::makeRequest(const String& endpoint, const String& method, const String& payload, String& response) {
    http.begin(apiUrl + endpoint);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + apiKey);
    http.setTimeout(API_REQUEST_TIMEOUT);
    
    int httpResponseCode;
    
    if (method == "POST") {
        httpResponseCode = http.POST(payload);
    } else if (method == "GET") {
        httpResponseCode = http.GET();
    } else {
        http.end();
        return false;
    }
    
    if (httpResponseCode > 0) {
        response = http.getString();
        http.end();
        return (httpResponseCode >= 200 && httpResponseCode < 300);
    } else {
        response = "HTTP Error: " + String(httpResponseCode);
        http.end();
        return false;
    }
}

String APIClient::createSensorDataPayload(SensorReading* readings, int count) {
    JsonDocument doc;
    JsonArray dataArray = doc["sensor_data"].to<JsonArray>();
    
    // Add device information
    doc["device_id"] = deviceId;
    doc["timestamp"] = millis();
    
    // Add sensor readings
    for (int i = 0; i < count; i++) {
        if (readings[i].valid) {
            JsonObject reading = dataArray.add<JsonObject>();
            reading["bin_id"] = readings[i].bin_id;
            reading["weight"] = readings[i].weight;
            reading["timestamp"] = readings[i].timestamp;
            reading["unit"] = "kg";
        }
    }
    
    String payload;
    serializeJson(doc, payload);
    return payload;
}

void APIClient::loadCredentials() {
    apiKey = preferences.getString(NVS_API_KEY, "");
    apiUrl = preferences.getString(NVS_API_URL, API_BASE_URL);
    deviceId = preferences.getString(NVS_DEVICE_ID, "");
    
    Serial.printf("Loaded API credentials - URL: %s, Device ID: %s\n", 
                 apiUrl.c_str(), deviceId.c_str());
}

bool APIClient::testConnection() {
    String response;
    bool success = makeRequest("/health", "GET", "", response);
    
    if (success) {
        Serial.printf("API health check successful: %s\n", response.c_str());
    } else {
        Serial.printf("API health check failed: %s\n", response.c_str());
    }
    
    return success;
}
