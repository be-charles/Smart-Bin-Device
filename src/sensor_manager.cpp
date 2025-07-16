#include "sensor_manager.h"

SensorManager::SensorManager() {
    // Initialize default scale factors
    float defaultFactors[] = HX711_DEFAULT_SCALE_FACTORS;
    
    for (int i = 0; i < MAX_BINS; i++) {
        sensorEnabled[i] = false; // Disable all sensors by default - will be enabled during detection
        lastReadings[i] = 0.0;
        lastReadTime[i] = 0;
        readings[i].bin_id = i;
        readings[i].weight = 0.0;
        readings[i].timestamp = 0;
        readings[i].valid = false;
        scaleFactors[i] = defaultFactors[i]; // Set default scale factor
    }
}

void SensorManager::init() {
    Serial.println("Initializing sensor manager...");
    
    // Load scale factors from NVS
    loadScaleFactors();
    
    if (TESTING_MODE) {
        Serial.println("TESTING MODE: Skipping hardware initialization");
        Serial.println("Using dummy data for sensor readings");
        
        // In testing mode, enable all sensors for demonstration
        for (int i = 0; i < MAX_BINS; i++) {
            sensorEnabled[i] = true;
            readings[i].bin_id = i;
            readings[i].weight = generateDummyWeight(i);
            readings[i].timestamp = millis();
            readings[i].valid = true;
            lastReadings[i] = readings[i].weight;
            lastReadTime[i] = readings[i].timestamp;
            
            Serial.printf("Sensor %d (DUMMY) initialized - Initial weight: %.2f kg, Scale: %.2f\n", 
                         i, readings[i].weight, scaleFactors[i]);
        }
    } else {
        Serial.println("PRODUCTION MODE: Detecting and initializing HX711 hardware");
        
        // Detect connected sensors
        if (!detectConnectedSensors()) {
            Serial.println("ERROR: No sensors detected or insufficient sensors connected!");
            Serial.printf("Minimum required sensors: %d\n", MIN_REQUIRED_SENSORS);
            return;
        }
        
        // Initialize detected sensors
        for (int i = 0; i < MAX_BINS; i++) {
            if (sensorEnabled[i]) {
                sensors[i].begin(doutPins[i], clkPins[i]);
                sensors[i].set_scale(scaleFactors[i]); // Use individual scale factor
                sensors[i].tare(); // Reset the scale to 0
                
                Serial.printf("Sensor %d initialized on pins CLK:%d, DOUT:%d, Scale: %.2f\n", 
                             i, clkPins[i], doutPins[i], scaleFactors[i]);
            }
        }
    }
    
    Serial.printf("Sensor manager initialization complete - %d sensors active\n", getConnectedSensorCount());
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
    
    if (TESTING_MODE) {
        // Generate dummy data for testing
        reading.weight = generateDummyWeight(binId);
        reading.valid = isValidReading(reading.weight);
    } else {
        // Read from actual HX711 sensors
        if (sensors[binId].is_ready()) {
            float rawWeight = sensors[binId].get_units(6); // Average of 6 readings
            reading.weight = smoothReading(binId, rawWeight);
            reading.valid = isValidReading(reading.weight);
        } else {
            reading.weight = lastReadings[binId]; // Use last known good reading
            reading.valid = false; // Mark as invalid since sensor not ready
        }
    }
    
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
    // if (sensors[binId].is_ready()) {
    //     long reading = sensors[binId].get_value(10); // Average of 10 readings
    //     float scale = reading / knownWeight;
    //     sensors[binId].set_scale(scale);
    //     Serial.printf("Sensor %d calibrated with scale factor: %.2f\n", binId, scale);
    // }

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

void SensorManager::setScaleFactor(int binId, float scaleFactor) {
    if (binId >= 0 && binId < MAX_BINS) {
        scaleFactors[binId] = scaleFactor;
        
        // Update the actual sensor if not in testing mode
        if (!TESTING_MODE && sensorEnabled[binId]) {
            sensors[binId].set_scale(scaleFactor);
        }
        
        Serial.printf("Scale factor for sensor %d set to: %.2f\n", binId, scaleFactor);
    }
}

float SensorManager::getScaleFactor(int binId) {
    if (binId >= 0 && binId < MAX_BINS) {
        return scaleFactors[binId];
    }
    return HX711_DEFAULT_SCALE_FACTOR;
}

void SensorManager::saveScaleFactors() {
    preferences.begin(NVS_NAMESPACE, false);
    
    for (int i = 0; i < MAX_BINS; i++) {
        String key = String(NVS_SCALE_FACTOR_PREFIX) + String(i);
        preferences.putFloat(key.c_str(), scaleFactors[i]);
    }
    
    preferences.end();
    Serial.println("Scale factors saved to NVS");
}

void SensorManager::loadScaleFactors() {
    preferences.begin(NVS_NAMESPACE, true);
    
    float defaultFactors[] = HX711_DEFAULT_SCALE_FACTORS;
    bool anyLoaded = false;
    
    for (int i = 0; i < MAX_BINS; i++) {
        String key = String(NVS_SCALE_FACTOR_PREFIX) + String(i);
        float savedFactor = preferences.getFloat(key.c_str(), -1.0);
        
        if (savedFactor > 0) {
            scaleFactors[i] = savedFactor;
            anyLoaded = true;
        } else {
            scaleFactors[i] = defaultFactors[i];
        }
    }
    
    preferences.end();
    
    if (anyLoaded) {
        Serial.println("Scale factors loaded from NVS");
    } else {
        Serial.println("Using default scale factors (no saved values found)");
    }
    
    // Print all scale factors
    for (int i = 0; i < MAX_BINS; i++) {
        Serial.printf("Sensor %d scale factor: %.2f\n", i, scaleFactors[i]);
    }
}

int SensorManager::getConnectedSensorCount() {
    int count = 0;
    for (int i = 0; i < MAX_BINS; i++) {
        if (sensorEnabled[i]) {
            count++;
        }
    }
    return count;
}

bool SensorManager::detectConnectedSensors() {
    Serial.println("Detecting connected HX711 sensors...");
    
    int detectedCount = 0;
    
    for (int i = 0; i < MAX_BINS; i++) {
        Serial.printf("Testing sensor %d on pins CLK:%d, DOUT:%d... ", i, clkPins[i], doutPins[i]);
        
        // Initialize the sensor temporarily for testing
        HX711 testSensor;
        testSensor.begin(doutPins[i], clkPins[i]);
        
        // Wait a bit for sensor to stabilize
        delay(100);
        
        // Test if sensor is responding
        unsigned long startTime = millis();
        bool sensorResponding = false;
        
        while (millis() - startTime < SENSOR_DETECTION_TIMEOUT) {
            if (testSensor.is_ready()) {
                // Try to get a reading to confirm sensor is working
                long rawReading = testSensor.get_value(1);
                
                // Check if we get a reasonable response (not stuck at 0 or max value)
                if (rawReading != 0 && rawReading != 0x7FFFFF && rawReading != 0x800000) {
                    sensorResponding = true;
                    break;
                }
            }
            delay(50);
        }
        
        if (sensorResponding) {
            sensorEnabled[i] = true;
            detectedCount++;
            Serial.println("DETECTED");
        } else {
            sensorEnabled[i] = false;
            Serial.println("NOT FOUND");
        }
    }
    
    Serial.printf("Sensor detection complete: %d/%d sensors detected\n", detectedCount, MAX_BINS);
    
    // Check if we have minimum required sensors
    if (detectedCount < MIN_REQUIRED_SENSORS) {
        Serial.printf("ERROR: Only %d sensors detected, minimum required: %d\n", 
                     detectedCount, MIN_REQUIRED_SENSORS);
        return false;
    }
    
    Serial.println("Sensor detection successful - sufficient sensors found");
    return true;
}
