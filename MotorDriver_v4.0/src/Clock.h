#ifndef Clock_h
#define Clock_h
#include <Arduino.h>

class Clock
{
private:
    int InitTime = 0;

public:
    void SetTime(String &AdjustTime)
    {
        int Now_BIRTC = millis() - 500;
        int Now_Set[3];
        Now_Set[0] = AdjustTime.toInt();
        Now_Set[1] = AdjustTime.substring(3).toInt();
        Now_Set[2] = AdjustTime.substring(6).toInt();
        InitTime = (Now_Set[0] * 60 * 60 + Now_Set[1] * 60 + Now_Set[2]) * 1000 - Now_BIRTC;
    }

    String toString(int Millis)
    {
        if(Millis < 0 ){
            return "00:00:00";
        }
        int now = (InitTime + Millis) / 1000;
        int NOW[3];
        NOW[0] = (now / 60 / 60) % 24;
        NOW[1] = (now / 60) % 60;
        NOW[2] = now % 60;
        String Show = "";
        if (NOW[0] < 10)
        {
            Show += "0";
        }
        Show += String(NOW[0]) + ":";
        if (NOW[1] < 10)
        {
            Show += "0";
        }
        Show += String(NOW[1]) + ":";
        if (NOW[2] < 10)
        {
            Show += "0";
        }
        Show += String(NOW[2]);
        return Show;
    }
};
#endif