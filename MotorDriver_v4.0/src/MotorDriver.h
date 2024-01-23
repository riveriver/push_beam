#ifndef MotorDriver_H
#define MotorDriver_H
#include <Arduino.h>

class MotorDriverSpeed
{
public:
    unsigned int Period[200] = {0};
    unsigned int last_Rising = 0;
    unsigned short C = 0;
    int F_Hz()
    {
        if (last_Rising == 0)
        {
            return -1;
        }
        else if (micros() - last_Rising > 500000)
        {
            last_Rising = 0;
            return -1;
        }
        else
        {
            return 1 / (Period[(C + 199) % 200] / 1000000.0);
        }
    }
};

void IRAM_ATTR MD_Speed_ISR();

// ISR Callback pointer can't used "this", thus need to be outside the class.
// Must used IRAM_ATTR to speed up.
// Used as least calculation as possible in the ISR.

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MotorDriver
{
private:
    const int PWM_FREQUENCY = 1000;
    const byte PWM_RESOUTION = 8;
    const byte PWM_CHANNEL = 0;
    byte MD_Brak;
    byte MD_Dir;
    byte MD_V;

    float MaximumVelocity = 5; // mm/s
    float MinimumVelocity = 0.7;
    int Max_Acc = 255;
    // u_out = AngularVelocity - 10
    int u_t0 = 0;
    int16_t adc1, adc2, adc3;
    float CFB_Read[10] = {0};
    float VFB_Read[5] = {0};
    float VFB_BW[5] = {0};

    bool isAS5600Begin = false;
    int Vc = 0;

    int VrtoVl = 95; // Speed Feed Back Value to Linear Velocity

public:
    bool Swich = true;
    bool Check = false;
    int u_out = 0;
    int Speed = -1;
    float SpeedR = -1;
    float Current = -1;
    float Battery = -1;
    int BatteryPercent = -1;
    float MDCurrent = -1;
    int H = 1500; // mm
    // int D = 860;  // mm
    float *MountedAngle;
    float WallAngle = 90;
    MotorDriverSpeed *SpeedFB;

    void Initialize(byte IO_V, byte IO_Dir, byte IO_Brak, byte IO_V_FB, byte IO_SW, byte IO_EN_Dir);
    void AccControl();
    bool Output(float AngularVelocity);
    bool Manual(double Speed);
    void Emergency_Stop(bool isStop);
    void Update_Feedback();
    void CurrentFB();
    void ReadBattery();
    void Check_Connect();
};

#endif