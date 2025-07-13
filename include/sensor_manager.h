#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <HX711.h>
#include "config.h"

class SensorManager {
public:
    SensorManager();
    void init();
    void update();
    SensorReading readSensor(int binId);
    SensorReading* getAllReadings();
    bool isSensorEnabled(int binId);
    void enableSensor(int binId, bool enabled);
    void calibrateSensor(int binId, float knownWeight);
    float generateDummyWeight(int binId); // For testing without actual sensors

private:
    HX711 sensors[MAX_BINS];
    bool sensorEnabled[MAX_BINS];
    float lastReadings[MAX_BINS];
    unsigned long lastReadTime[MAX_BINS];
    SensorReading readings[MAX_BINS];
    
    // Pin mappings for each sensor
    int clkPins[MAX_BINS] = {
        HX711_1_CLK_PIN, HX711_2_CLK_PIN, HX711_3_CLK_PIN,
        HX711_4_CLK_PIN, HX711_5_CLK_PIN, HX711_6_CLK_PIN
    };
    
    int doutPins[MAX_BINS] = {
        HX711_1_DOUT_PIN, HX711_2_DOUT_PIN, HX711_3_DOUT_PIN,
        HX711_4_DOUT_PIN, HX711_5_DOUT_PIN, HX711_6_DOUT_PIN
    };
    
    float smoothReading(int binId, float newReading);
    bool isValidReading(float reading);
};

#endif // SENSOR_MANAGER_H
