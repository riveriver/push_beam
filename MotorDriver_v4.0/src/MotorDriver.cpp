#include "MotorDriver.h"
#include "RGBLED.h"
#include "AS5600.h"
#include <Wire.h>
#include <Adafruit_ADS1X15.h>

MotorDriverSpeed MDSpeed;
Adafruit_ADS1115 ads;
AS5600 as5600;

void IRAM_ATTR MD_Speed_ISR()
{
    unsigned int Te = micros();
    if (MDSpeed.last_Rising == 0) // after reset
    {
        MDSpeed.last_Rising = Te;
    }
    else if (Te < MDSpeed.last_Rising || Te - MDSpeed.last_Rising > 500000) // if micros() reset or motor reset
    {
        MDSpeed.last_Rising = 0; // Don't count
    }
    else if (Te - MDSpeed.last_Rising > 1000)
    {
        MDSpeed.Period[MDSpeed.C] = Te - MDSpeed.last_Rising;
        MDSpeed.last_Rising = Te;
        MDSpeed.C++;
        MDSpeed.C %= 200;
    }
}

void MotorDriver::Initialize(byte IO_V, byte IO_Dir, byte IO_Brak, byte IO_V_FB, byte IO_SW, byte IO_EN_Dir)
{
    MD_Brak = IO_Brak;
    MD_Dir = IO_Dir;
    MD_V = IO_V;
    SpeedFB = &MDSpeed;
    pinMode(IO_Dir, OUTPUT);
    pinMode(IO_V, OUTPUT);
    pinMode(IO_Brak, OUTPUT);
    pinMode(IO_V_FB, INPUT);
    Swich = digitalRead(IO_SW);
    digitalWrite(IO_Brak, LOW);
    digitalWrite(MD_Dir, LOW);
    attachInterrupt(digitalPinToInterrupt(IO_V_FB), MD_Speed_ISR, RISING);
    // as5600
    as5600.begin((IO_EN_Dir == -1) ? 255 : IO_EN_Dir);
    as5600.setDirection(AS5600_CLOCK_WISE); // default
    isAS5600Begin = as5600.isConnected();
    if(!isAS5600Begin)
        Serial.println("AS5600 not connect.");
    // ads 1115
    if (!ads.begin())
        Serial.println("ADS1115 begin error.");
    ReadBattery();
}

void MotorDriver::Check_Connect()
{
    if (Check || !Swich)
        return;
    analogWrite(MD_V, 10);
    delay(200);
    if (SpeedFB->F_Hz() != -1)
    {
        Check = true;
        digitalWrite(MD_Dir, HIGH);
        analogWrite(MD_V, 10);
        delay(200);
    }
    analogWrite(MD_V, 0);
}

void MotorDriver::AccControl()
{
    Update_Feedback();
    if (Vc != 0)
    {
        u_out = Vc * VrtoVl / 4.0 + 0.5;
        /*
        if (Speed == -1)
        {
            u_out = Vc * VrtoVl / 4.0 + 0.5;
        }
        else if (Vc > 0)
        {
            u_out += ((abs(Vc) * VrtoVl - Speed) / 4.0 * 0.3 + 0.5);
        }
        else
        {
            u_out -= ((abs(Vc) * VrtoVl - Speed) / 4.0 * 0.1 + 0.5);
        }
        */
        u_out = max(min(255, u_out), -255);
    }
    if (!Swich)
    {
        LED.Write(2, 256, 0, 0, LED.LIGHT30);
        u_out = 0;
    }
    else if (!Check)
    {
        LED.Write(2, 256, 0, 0, LED.BLINK30);
        u_out = 0;
    }
    else if (u_out == 0)
    {
        LED.Write(2, 0, 256, 0, LED.LIGHT30);
    }
    if (u_out != u_t0)
    {
        if (u_out == 0)
            digitalWrite(MD_Brak, HIGH);
        else if (u_t0 == 0)
            digitalWrite(MD_Brak, LOW);

        int u_t1 = u_out; //*(4095/255);

        // Max Acceleration control
        /*
        if (u_t1 > u_t0 + Max_Acc)
        {
            u_t1 = u_t0 + Max_Acc;
        }
        else if (u_t1 < u_t0 - Max_Acc)
        {
            u_t1 = u_t0 - Max_Acc;
        }
        */

        if (u_t1 > 0)
        {
            digitalWrite(MD_Dir, HIGH);
        }
        else
        {
            digitalWrite(MD_Dir, LOW);
        }
        analogWrite(MD_V, abs(u_t1));
        if (abs(u_t0 - u_t1) > 10)
        {
            SpeedFB->Period[SpeedFB->C] = 0;
            SpeedFB->C++;
            SpeedFB->C %= 200;
        }
        u_t0 = u_t1;
    }
}

bool MotorDriver::Output(float AngularVelocity)
{
    LED.Write(2, 0, 0, 256, LED.BLINK30);
    Vc = -AngularVelocity * H * cos(45 * PI / 180); // if MountedAngle is a constant. (Assume Angle[0] ~ 90)
    // Vc = -AngularVelocity * H * sin((WallAngle - *MountedAngle) * PI / 180); // if MountedAngle can be measure
    if (Vc == 0 || abs(Vc) < MinimumVelocity)
    {
        u_out = 0;
        Vc = 0;
        LED.Write(2, 0, 256, 0, LED.LIGHT30);
    }
    else if (Vc < 0)
    {
        Vc = (Vc < -MaximumVelocity) ? -MaximumVelocity : Vc;
        // u_out = LinearVelocity * 32.5 - 2;
        // u_out = LinearVelocity * 8 - 10;
    }
    else
    {
        Vc = (Vc > -MaximumVelocity) ? MaximumVelocity : Vc;
        // u_out = LinearVelocity * 32.5 + 2;
        // u_out = LinearVelocity * 8 + 11;
    }
    return 1;
}

bool MotorDriver::Manual(double Speed)
{
    LED.Write(2, 128, 128, 0, LED.BLINK30);
    if (Speed == 0)
    {
        u_out = 0;
        LED.Write(2, 0, 256, 0, LED.LIGHT30);
    }
    else if (Speed < 0)
        u_out = -(255 - 10) * min(1.0, abs(Speed)) - 10;
    else if (Speed > 0)
        u_out = (255 - 10) * min(1.0, abs(Speed)) + 10;
    return 1;
}

void MotorDriver::ReadBattery()
{
    float MDCurrent = ads.computeVolts(adc2);
    Battery = ads.computeVolts(adc3);
    BatteryPercent = (Battery - 1.78) / 0.5 * 100.0;
    BatteryPercent = min(max(BatteryPercent, 0), 100);
    /*
    Serial.print("adc1: ");
    Serial.print(Current);
    Serial.print(" ,adc2: ");
    Serial.print(MDCurrent);
    Serial.print(" ,adc3(Battery): ");
    Serial.println(Battery);
    //*/
}

void MotorDriver::CurrentFB()
{
    // Velocity Feed Back (Encoder)
    if (!isAS5600Begin)
    {
        SpeedR = -1;
    }
    else if (Speed == -1)
    {
        memset(VFB_Read, 0, sizeof(VFB_Read));
        memset(VFB_BW, 0, sizeof(VFB_BW));
        SpeedR = 0;
    }
    else
    {
        memcpy(&VFB_Read[1], &VFB_Read[0], sizeof(VFB_Read[0]) * 4);
        memcpy(&VFB_BW[1], &VFB_BW[0], sizeof(VFB_BW[0]) * 4);
        VFB_Read[0] = as5600.getAngularSpeed(AS5600_MODE_RPM);
        // butterworth
        float a_i[4] = {2.41959115, -2.39452813, 1.1034105, -0.19778313};
        float b_i[5] = {0.00433185, 0.0173274, 0.02599111, 0.0173274, 0.00433185};
        VFB_BW[0] = 0;
        for (int i = 0; i < 4; i++)
        {
            VFB_BW[0] += a_i[i] * VFB_BW[i + 1];
        }
        for (int i = 0; i < 5; i++)
        {
            VFB_BW[0] += b_i[i] * VFB_Read[i];
        }
        //SpeedR = VFB_BW[0];
        SpeedR = VFB_Read[0];
    }
    // Current Feed Back
    // Average filter with windows of 10 have a similar performance with 5th Butterworth filter.
    memcpy(&CFB_Read[1], &CFB_Read[0], sizeof(CFB_Read[0]) * 9);
    adc1 = ads.readADC_SingleEnded(1);
    adc2 = ads.readADC_SingleEnded(2);
    adc3 = ads.readADC_SingleEnded(3);
    float CFB_temp = ads.computeVolts(adc1);
    // 
    if (CFB_temp > 1.5)
        CFB_Read[0] = CFB_Read[1];
    else if (CFB_temp < 0.029)
        CFB_Read[0] = 0.029;
    else
        CFB_Read[0] = CFB_temp;
    //
    float CFB_sum = 0;
    for (int i = 0; i < 10; i++)
    {
        CFB_sum += CFB_Read[i];
    }
    Current = CFB_sum / 10;
    // MD Current Feedback

    /*
    Serial.print("Velocity:");
    Serial.println(VFB_BW[0]);
    /*
    Serial.print(",Current_x_10:");
    Serial.println(Current * 10);
    //*/
}

void MotorDriver::Emergency_Stop(bool isStop)
{
    if (isStop)
    {
        Swich = false;
        AccControl();
    }
    else
    {
        Swich = true;
    }
}

void MotorDriver::Update_Feedback()
{
    if (SpeedFB->last_Rising == 0)
    {
        Speed = -1;
    }
    else if (micros() - SpeedFB->last_Rising > 500000)
    {
        SpeedFB->last_Rising = 0;
        Speed = -1;
    }
    else
    {
        int C = (SpeedFB->C + 199) % 200;
        int n = 0;
        int Pn = SpeedFB->Period[C % 200]; // Period(C-n)
        int Ps = 0;                        // Period(C) + Period(C-1) + ... + Period(C-n)
        while (Ps < 200000 && n < 200 && Pn != 0)
        {
            Ps += Pn;
            n++;
            Pn = SpeedFB->Period[(C + 200 - n) % 200];
        }
        Speed = (n != 0) ? 1 / (Ps / 1000000.0 / n) : -1;
    }
}