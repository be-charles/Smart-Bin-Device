#ifndef STATUS_LED_H
#define STATUS_LED_H

#include <Arduino.h>
#include "config.h"

class StatusLED {
public:
    StatusLED();
    void init();
    void setWiFiStatus(bool connected);
    void setAPIStatus(bool connected);
    void setBluetoothStatus(bool active);
    void blinkWiFi(int times = 3);
    void blinkAPI(int times = 3);
    void blinkBluetooth(int times = 3);
    void update(); // Call in main loop for blinking effects

private:
    bool wifiState;
    bool apiState;
    bool bluetoothState;
    unsigned long lastBlinkTime;
    int blinkCount;
    int targetBlinks;
    int currentLED;
    bool blinkState;
};

#endif // STATUS_LED_H
