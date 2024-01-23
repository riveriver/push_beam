#ifndef SerialDebug_H
#define SerialDebug_H
#include "RealTimeClock.h"
#include "SDCard.h"

#define Test_Version

#ifdef Test_Version
class SerialDebug
{
private:
    String *Buffer = nullptr;
    RealTimeClock *pRTC = nullptr;
    bool isEnd = true;

public:
    void Setup(SDCard &Save_to_Card)
    {
        Buffer = &Save_to_Card.DebugString;
    }

    void SetRTC(RealTimeClock *RTC)
    {
        pRTC = RTC;
    }

    void print(String Str)
    {
        if (isEnd)
        {
            if (pRTC)
            {
                Serial.print(pRTC->DateTimeStamp() + " -> ");
            }
        }
        Serial.print(Str);

        if (Buffer)
        {
            if (isEnd)
            {
                if (pRTC)
                {
                    *Buffer += pRTC->DateTimeStamp() + " -> ";
                }
            }
            *Buffer += Str;
            if ((*Buffer).length() > 1000000)
            {
                *Buffer = (*Buffer).substring(1000);
            }
        }
        isEnd = false;
    }

    void println(String Str)
    {
        print(Str);
        Serial.println("");
        if (Buffer)
        {
            *Buffer += "\n";
        }
        isEnd = true;
    }

    void printOnTop(String Str)
    {
        if (Buffer)
        {
            if ((*Buffer).length() < 1000000)
            {
                *Buffer = Str + "\n" + *Buffer;
            }
        }
    }
};

#else
class SerialDebug
{
public:
    void Setup(SDCard &Save_to_Card){}
    void SetRTC(RealTimeClock *RTC){}
    void print(String Str){}
    void println(String Str){}
    void printOnTop(String Str){}
};
#endif
#endif

extern SerialDebug Debug;