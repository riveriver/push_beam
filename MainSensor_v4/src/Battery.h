/*
 * Not Complete: Change voltage into battery percentage
 */
#ifndef Battery_h
#define Battery_h

#include <Arduino.h>

class Battery
{
private:
    byte p;

public:
    int Percent = 0;
    void SetPin(byte Pin)
    {
        p = Pin;
        pinMode(p, INPUT);
    }
    void Update()
    {
        int TimeStamp = millis();
        int Count = 0;
        int Sum = 0;
        while (millis() - TimeStamp < 100)
        {
            int B = analogRead(p);
            if (B != 0)
            {
                Sum += B;
                Count++;
            }
            delay(1);
        }
        Percent = ((Sum / Count - 3060.0) / 410.0 * 100.0 + 0.5);
    }
};

#endif
