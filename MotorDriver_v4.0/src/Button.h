#ifndef Button_H
#define Button_H

#include <Arduino.h>
#include "RGBLED.h"

TaskHandle_t TIMER;
const byte CL = 40;
const byte DT = 41;
const byte SW = 0;
int OldValue;
int Direction;
int OldPress = 1;
int Ct = 0;      // Button Turn count, renew every cycle
int Cr = 0;      // Button Turn Count, renew when SW Press/Release
bool Sw = false; // ShortPress, longPress

int LessShortPress;

void Press()
{
    int SWValue = digitalRead(SW);
    if (OldPress == SWValue)
    {
        return;
    }
    if (SWValue == 0)
    {
        // Press
        Cr = 0;
        Swich.Off_Clock_Start();
        LED.Write(2, 255, 255, 255, LED.LIGHT30, 1);
    }
    else
    {
        // Release;
        Swich.Off_Clock_Stop();
        LED.Write(2, 0, 0, 0, 0, 1);
        if (millis() - LessShortPress > 200)
        {
            Sw = true;
        }
        LessShortPress = millis();
    }
    OldPress = SWValue;
}

void UpdateEncoder()
{
    int CLValue = digitalRead(CL);
    if (CLValue != digitalRead(DT))
    {
        //Direction = CLValue;
        Direction = !CLValue;
    }
    else if (CLValue != OldValue)
    {
        OldValue = CLValue;
        if (!CLValue)
        {
            return;
        } // 11->00->11 = 1, for 11->00 = 1 & 00->11 = 1 type, command out this line.
        if (Direction == CLValue)
        {
            Ct++;
            Cr++;
        }
        else
        {
            Ct--;
            Cr--;
        }
    }
}

void MotorDriverSwich()
{
    if (digitalRead(IO_MD_Swich))
        MD.Emergency_Stop(0);
    else
        MD.Emergency_Stop(1);
    LastUpdate = -1;
}

void Button_Iitialize()
{
    pinMode(CL, INPUT);
    pinMode(DT, INPUT);
    pinMode(SW, INPUT);
    pinMode(IO_MD_Swich, INPUT);
    Direction = digitalRead(CL);
    OldValue = digitalRead(CL);
    attachInterrupt(digitalPinToInterrupt(CL), UpdateEncoder, CHANGE);
    attachInterrupt(digitalPinToInterrupt(DT), UpdateEncoder, CHANGE);
    attachInterrupt(digitalPinToInterrupt(SW), Press, CHANGE);
    attachInterrupt(digitalPinToInterrupt(IO_MD_Swich), MotorDriverSwich, CHANGE);
    oled.Page = &Page;
    oled.Cursor = &Cursor;
    oled.Encoder_Temp = &Ct;
}

void Button_Update()
{
    if (Cursor == 0)
    {
        if (Sw)
        {
            switch (Page)
            {
            case 2:
                break;
            case 3:
                if (MD.Swich && !isConnect[2])
                    Cursor = 1;
                break;
            default:
                Cursor = 1;
                break;
            }
        }
        else
        {
            Page = max(min(Page - Ct, 4), 0);
        }
    }
    else
    {
        switch (Page)
        {
        case 0:
            if (Ct != 0)
            {
                Cursor = 0;
            }
            else if (Sw)
            {
                isScanning = !isScanning;
                act = true;
                Cursor = 0;
            }
            break;
        case 1:
            if (Ct != 0)
            {
                Cursor = 0;
            }
            else if (Sw)
            {
                keepAdvertising = !keepAdvertising;
                act = true;
                Cursor = 0;
            }
            break;
        case 3:
            if (!MD.Swich || !MD.Check || isConnect[2])
            {
                Cursor = 0;
            }
            else if (Sw)
            {
                if (MD.u_out == 0)
                    Cursor = 0;
                else
                    MD.Manual(0);
            }
            else if (Ct != 0)
            {
                Cr = max(min(Cr, 100), -100);
                MD.Manual(Cr * 0.01);
            }
            break;
        case 4:
            if (Sw)
            {
                Cursor = !Cursor;
            }
            else if (Ct != 0 && Cursor == 1)
            {
                MD.H += Ct * 10;
                MD.H = max(min(MD.H, 2000), 200);
            }
            break;
        default:
            if (Sw)
            {
                Cursor = 0;
            }
            break;
        }
    }
    Ct = 0;
    Sw = false;
}

#endif