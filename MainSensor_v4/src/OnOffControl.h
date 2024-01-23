#ifndef OnOffControl_H
#define OnOffControl_H
#include <Arduino.h>

int WakeTime = 60*30;  // sec
int SleepTime =30; // min

const byte SWPin = 1;
const byte ButPin = 0;
gpio_num_t ButGPIO = GPIO_NUM_0;

RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR int WakeUpMode = 3;
RTC_DATA_ATTR char SavePath[20];
RTC_DATA_ATTR int StartSleepTimeStemp;

void PowerSaveSleep(int DisplaceTime)
{
    oled.PowerSave(1);
    WakeUpMode = 5;
    digitalWrite(SWPin, LOW);
    Debug.println(" AutoSleep.");
    if (sdCard.Initialize)
    {
        sdCard.path.toCharArray(SavePath, sdCard.path.length() + 1);
    }
    StartSleepTimeStemp = Clock.NowSec();
    if(SleepTime * 60 < DisplaceTime){
        esp_sleep_enable_timer_wakeup(1000000);
    }else{
        esp_sleep_enable_timer_wakeup((SleepTime * 60 - DisplaceTime) * 1000000 /* second to micro seconds */);
    }
    
    esp_deep_sleep_start();
}

void SleepClockCB(void *pvParameter)
{
    for (;;)
    {
        vTaskDelay((int)pvParameter * 1000);
        PowerSaveSleep((int)pvParameter);
    }
}

void WakeUpDetect()
{
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
    // Begine with sleeping
    if (bootCount == 0)
    {
        bootCount++;
        pinMode(SWPin, OUTPUT);
        digitalWrite(SWPin, LOW);
        esp_deep_sleep_start();
    }
    pinMode(SWPin, OUTPUT);
    digitalWrite(SWPin, HIGH);
    pinMode(40, OUTPUT);
    digitalWrite(40, LOW);
    pinMode(41, OUTPUT);
    digitalWrite(41, LOW);
    pinMode(42, OUTPUT);
    digitalWrite(42, LOW);
    
    Serial.setRxBufferSize(256);
    Serial.begin(115200);
    Debug.Setup(sdCard);
    Clock.Initialize();
    Debug.SetRTC(&Clock);
    oled.PowerSave(1);

    if(esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER){
        Debug.printOnTop("ESP_SLEEP_WAKEUP_TIMER");
        if (String(SavePath) != "")
        {
            sdCard.path = SavePath;
            sdCard.Initialize = true;
        }
        xTaskCreatePinnedToCore(SleepClockCB, "Sleep Clock", 4096, (void *)WakeTime /*WakeUp Sec*/, 5, &SleepClock, 1);
        isAutoAdd = false;
    }else if(esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0){
        pinMode(ButPin, INPUT);
        while (millis() < 3000 && digitalRead(ButPin) == 0)
        {
            delay(100);
        }
        if (millis() < 3000)
        {
            if (WakeUpMode == 5)
            {
                esp_sleep_enable_timer_wakeup((SleepTime * 60 - WakeTime) * 1000000 - (Clock.NowSec() - StartSleepTimeStemp) * 1000000);
            }
            digitalWrite(40, HIGH);
            digitalWrite(41, HIGH);
            digitalWrite(42, HIGH);
            esp_deep_sleep_start();
        }
        Debug.printOnTop("--------------------------------ESP_SLEEP_WAKEUP_EXT0--------------------------------");
        oled.PowerSave(0);
    }else{
        Debug.println("Unknown Wakeup");
        oled.PowerSave(0);
    }
}



#endif