#include "status_led.h"

StatusLED::StatusLED() {
    wifiState = false;
    apiState = false;
    bluetoothState = false;
    lastBlinkTime = 0;
    blinkCount = 0;
    targetBlinks = 0;
    currentLED = -1;
    blinkState = false;
}

void StatusLED::init() {
    pinMode(WIFI_STATUS_LED_PIN, OUTPUT);
    pinMode(API_STATUS_LED_PIN, OUTPUT);
    pinMode(BLUETOOTH_STATUS_LED_PIN, OUTPUT);
    
    // Turn off all LEDs initially
    digitalWrite(WIFI_STATUS_LED_PIN, LOW);
    digitalWrite(API_STATUS_LED_PIN, LOW);
    digitalWrite(BLUETOOTH_STATUS_LED_PIN, LOW);
    
    Serial.println("Status LEDs initialized");
}

void StatusLED::setWiFiStatus(bool connected) {
    wifiState = connected;
    digitalWrite(WIFI_STATUS_LED_PIN, connected ? HIGH : LOW);
    Serial.printf("WiFi LED: %s\n", connected ? "ON" : "OFF");
}

void StatusLED::setAPIStatus(bool connected) {
    apiState = connected;
    digitalWrite(API_STATUS_LED_PIN, connected ? HIGH : LOW);
    Serial.printf("API LED: %s\n", connected ? "ON" : "OFF");
}

void StatusLED::setBluetoothStatus(bool active) {
    bluetoothState = active;
    digitalWrite(BLUETOOTH_STATUS_LED_PIN, active ? HIGH : LOW);
    Serial.printf("Bluetooth LED: %s\n", active ? "ON" : "OFF");
}

void StatusLED::blinkWiFi(int times) {
    currentLED = WIFI_STATUS_LED_PIN;
    targetBlinks = times * 2; // On and off counts as 2 blinks
    blinkCount = 0;
    lastBlinkTime = millis();
    blinkState = true;
}

void StatusLED::blinkAPI(int times) {
    currentLED = API_STATUS_LED_PIN;
    targetBlinks = times * 2;
    blinkCount = 0;
    lastBlinkTime = millis();
    blinkState = true;
}

void StatusLED::blinkBluetooth(int times) {
    currentLED = BLUETOOTH_STATUS_LED_PIN;
    targetBlinks = times * 2;
    blinkCount = 0;
    lastBlinkTime = millis();
    blinkState = true;
}

void StatusLED::update() {
    if (currentLED != -1 && blinkCount < targetBlinks) {
        unsigned long currentTime = millis();
        if (currentTime - lastBlinkTime >= 250) { // 250ms blink interval
            blinkState = !blinkState;
            digitalWrite(currentLED, blinkState ? HIGH : LOW);
            blinkCount++;
            lastBlinkTime = currentTime;
            
            if (blinkCount >= targetBlinks) {
                // Restore original state after blinking
                currentLED = -1;
                setWiFiStatus(wifiState);
                setAPIStatus(apiState);
                setBluetoothStatus(bluetoothState);
            }
        }
    }
}
