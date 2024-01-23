#ifndef Battery_H
#define Battery_H

#include <Arduino.h>
class Battery
{
private:
    byte p = -1;
    int TimeStamp = 0;
    int Count = 0;
    int Sum = 0;

public:
    int Percent = 0;
    void SetPin(byte Pin)
    {
        p = Pin;
        pinMode(p, INPUT);
    }
    void Update()
    {
        if (p == -1)
        {
            return;
        }
        if (TimeStamp == 0)
        {
            TimeStamp = millis();
            Sum = analogRead(p);
            Count = 1;
        }
        else if (millis() - TimeStamp > 500)
        {
            Serial.println(Sum/Count);
            Percent = (Sum / Count - 1850) / 4.75 + 0.5;
            Percent = max(min(Percent, 100),0);
            TimeStamp = millis();
            Sum = analogRead(p);
            Count = 1;
        }
        else
        {
            Sum += analogRead(p);
            Count ++;
        }
    }
};

#endif