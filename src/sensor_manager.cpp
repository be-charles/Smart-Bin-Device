#include "sensor_manager.h"

SensorManager::SensorManager() {
    for (int i = 0; i < MAX_BINS; i++) {
        sensorEnabled[i] = true; // Enable all sensors by default
        lastReadings[i] = 0.0;
        lastReadTime[i] = 0;
        readings[i].bin_id = i;
        readings[i].weight = 0.0;
        readings[i].timestamp = 0;
        readings[i].valid = false;
    }
}

void SensorManager::init() {
    Serial.println("Initializing sensor manager...");
    
    for (int i = 0; i < MAX_BINS; i++) {
        if (sensorEnabled[i]) {
            sensors[i].begin(doutPins[i], clkPins[i]);
            sensors[i].set_scale(HX711_SCALE_FACTOR);
            sensors[i].tare(); // Reset the scale to 0
            
            Serial.printf("Sensor %d initialized on pins CLK:%d, DOUT:%d\n", 
                         i, clkPins[i], doutPins[i]);
        }
    }
    
    Serial.println("Sensor manager initialization complete");
}

void SensorManager::update() {
    unsigned long currentTime = millis();
    
    for (int i = 0; i < MAX_BINS; i++) {
        if (sensorEnabled[i]) {
            readings[i] = readSensor(i);
        }
    }
}

SensorReading SensorManager::readSensor(int binId) {
    SensorReading reading;
    reading.bin_id = binId;
    reading.timestamp = millis();
    reading.valid = false;
    
    if (binId < 0 || binId >= MAX_BINS || !sensorEnabled[binId]) {
        reading.weight = 0.0;
        return reading;
    }
    
    // For testing purposes, generate dummy data
    // In production, this would read from actual HX711 sensors
    reading.weight = generateDummyWeight(binId);
    
    // Uncomment below for actual HX711 reading:
    /*
    if (sensors[binId].is_ready()) {
        float rawWeight = sensors[binId].get_units(3); // Average of 3 readings
        reading.weight = smoothReading(binId, rawWeight);
        reading.valid = isValidReading(reading.weight);
    } else {
        reading.weight = lastReadings[binId]; // Use last known good reading
    }
    */
    
    reading.valid = isValidReading(reading.weight);
    
    if (reading.valid) {
        lastReadings[binId] = reading.weight;
        lastReadTime[binId] = reading.timestamp;
        readings[binId] = reading;
    }
    
    return reading;
}

SensorReading* SensorManager::getAllReadings() {
    return readings;
}

bool SensorManager::isSensorEnabled(int binId) {
    if (binId < 0 || binId >= MAX_BINS) return false;
    return sensorEnabled[binId];
}

void SensorManager::enableSensor(int binId, bool enabled) {
    if (binId >= 0 && binId < MAX_BINS) {
        sensorEnabled[binId] = enabled;
        Serial.printf("Sensor %d %s\n", binId, enabled ? "enabled" : "disabled");
    }
}

void SensorManager::calibrateSensor(int binId, float knownWeight) {
    if (binId < 0 || binId >= MAX_BINS || !sensorEnabled[binId]) {
        return;
    }
    
    Serial.printf("Calibrating sensor %d with known weight: %.2f kg\n", binId, knownWeight);
    
    // Uncomment for actual HX711 calibration:
    /*
    if (sensors[binId].is_ready()) {
        long reading = sensors[binId].get_value(10); // Average of 10 readings
        float scale = reading / knownWeight;
        sensors[binId].set_scale(scale);
        Serial.printf("Sensor %d calibrated with scale factor: %.2f\n", binId, scale);
    }
    */
    
    Serial.printf("Sensor %d calibration complete\n", binId);
}

float SensorManager::generateDummyWeight(int binId) {
    // Generate realistic dummy weight data for testing
    // Simulate bins with different fill levels that change over time
    
    unsigned long currentTime = millis();
    float baseWeight = 0.0;
    
    switch (binId) {
        case 0: // Bin 0: Slowly filling up
            baseWeight = 5.0 + (currentTime / 60000.0) * 0.1; // Increases by 0.1kg per minute
            break;
        case 1: // Bin 1: Medium fill level with small variations
            baseWeight = 15.0 + sin(currentTime / 30000.0) * 2.0; // Oscillates around 15kg
            break;
        case 2: // Bin 2: Nearly full
            baseWeight = 28.0 + sin(currentTime / 45000.0) * 1.0; // Oscillates around 28kg
            break;
        case 3: // Bin 3: Empty with occasional small additions
            baseWeight = 0.5 + (sin(currentTime / 120000.0) > 0.8 ? 2.0 : 0.0);
            break;
        case 4: // Bin 4: Gradually emptying
            baseWeight = fmax(0.0, 20.0 - (currentTime / 90000.0) * 0.05); // Decreases slowly
            break;
        case 5: // Bin 5: Random variations
            baseWeight = 10.0 + sin(currentTime / 20000.0) * 5.0 + cos(currentTime / 35000.0) * 3.0;
            break;
        default:
            baseWeight = 5.0;
    }
    
    // Add small random noise to make it more realistic
    float noise = (random(-100, 100) / 1000.0); // ±0.1kg noise
    baseWeight += noise;
    
    // Ensure weight is not negative
    return fmax(0.0, baseWeight);
}

float SensorManager::smoothReading(int binId, float newReading) {
    // Simple moving average for smoothing
    float smoothedReading = (lastReadings[binId] * (WEIGHT_SMOOTHING_SAMPLES - 1) + newReading) / WEIGHT_SMOOTHING_SAMPLES;
    return smoothedReading;
}

bool SensorManager::isValidReading(float reading) {
    // Check if reading is within reasonable bounds
    return (reading >= 0.0 && reading <= 50.0); // Assuming max bin capacity is 50kg
}
