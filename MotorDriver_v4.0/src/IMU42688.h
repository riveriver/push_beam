#ifndef IMU42688_h
#define IMU42688_h
#include <Arduino.h>
#include "WC_IMU.h"
#include "WC_ICM42688.h"

#ifndef IMU_C
#define IMU_C 3
#endif

class IMU42688
{
private:
    float AngleMeasure[IMU_C][3];
    int IMUStart = 0;
    int CopeFailed = 0;
    double acc[3] = {0};
    double gyro[3] = {0};
    int16_t tempRaw;

public:
    float Angle[3] = {0, 0, 0};
    float AngleShow[3] = {0, 0, 0};
    float SensorTemperature;
    byte Gravity = 2;
    byte GravityPrevious = 1;
    const byte IMU_Update_Success = 0;
    const byte Err_IMU_Not_Warm_Up = 1;
    const byte Err_IMU_Receive_Data_Error = 2;
    const byte Err_IMU_Data_StDev_Outside_TrustHold = 3;
    const byte Err_IMU_Cope_Failed = 4;
    byte ErrorCode = Err_IMU_Not_Warm_Up;
    byte fWarmUp;
    void Initialize(byte SCK, byte MISO, byte MOSI, byte CS);
    byte Update();
    void Update42688();
};

#endif