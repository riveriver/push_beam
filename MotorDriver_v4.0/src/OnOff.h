#ifndef OnOff_H
#define OnOff_H
#include <Arduino.h>
#include "MotorDriver.h"
#include "SerialDebug.h"
#include "OLED.h"
#include "RGBLED.h"
#include "esp_bt.h"

RTC_DATA_ATTR int bootCount = -1;

class OnOff
{
private:
    byte ButPin;
    OLED oledb;
    int OffClock = 0;

public:
    MotorDriver *pMD;
    void On(gpio_num_t WakeUpPin)
    {
        ButPin = WakeUpPin;
        LED.SetUp();
        oledb.PowerSave(1);
        esp_sleep_enable_ext0_wakeup(WakeUpPin, 0);
        bootCount++;
        // Begine from sleeping
        pinMode(ButPin, INPUT);
        LED.Write(2, 255, 255, 255, LED.LIGHT30);
        LED.Update();

        if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0)
        {
            // Detect 3 min long press
            while (millis() < 1000 && digitalRead(ButPin) == 0)
            {
                delay(100);
            }
            LED.Off();
            if (millis() < 1000 && bootCount != 0)
            {
                esp_deep_sleep_start();
            }
        }
        oledb.Initialize();
        if (bootCount != 0)
        {
            while (digitalRead(ButPin) == 0)
                delay(100);
        }
        Serial.begin(115200);
        Serial.print("wake up ");
        Serial.println(bootCount);
    }

    void Off_Clock_Start()
    {
        OffClock = millis();
    }

    void Off_Clock_Stop()
    {
        OffClock = 0;
    }

    void OffCheck()
    {
        if (OffClock == 0)
        {
            return;
        }
        if (millis() - OffClock > 3000)
        {
            if (pMD)
                pMD->Emergency_Stop(1);
            Debug.println("Function Off");
            detachInterrupt(digitalPinToInterrupt(ButPin));
            oledb.PowerSave(1);
            LED.Off();
            LED.Write(2, 255, 255, 255, LED.LIGHT30);
            LED.Update();
            esp_bt_controller_disable();
            while (digitalRead(ButPin) == 0)
            {
            }
            LED.Off();
            esp_deep_sleep_start();
        }
        else
        {
            // LED.Write(2,255,255,255,LED.LIGHT30);
        }
    }
};

#endif