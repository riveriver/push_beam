#include "IMU42688.h"

WC_IMU kalman;
#ifdef IMU_CS
WC_ICM42688_SPI ICM42688(IMU_CS);
#else
WC_ICM42688_SPI ICM42688(48);
#endif

void IMU42688::Initialize(byte SCK, byte MISO, byte MOSI, byte CS)
{
    SPI.begin(SCK, MISO, MOSI, CS);
    IMUStart = millis();
    memset(AngleMeasure, 0, sizeof(AngleMeasure));
    int status;
    while ((status = ICM42688.begin()) != 0)
    {
        if (status == -1)
            Serial.println("bus data access error");
        else
            Serial.println("Chip versions do not match");
        delay(1000);
    }
    Serial.println("ICM42688 begin success!!!");
    ICM42688.setODRAndFSR(ALL, ODR_1KHZ, FSR_3);
    ICM42688.startTempMeasure();
    ICM42688.startGyroMeasure(LN_MODE);
    ICM42688.startAccelMeasure(LN_MODE);
    Update42688();
    kalman.init();
}

void IMU42688::Update42688()
{
    double ICM42688Temp;
    if ((ICM42688Temp = ICM42688.getTemperature()) != 0)
    {
        tempRaw = ICM42688Temp;
    }
    if ((ICM42688Temp = ICM42688.getAccelDataX()) != 0)
    {
        acc[0] = ICM42688Temp;
    }
    if ((ICM42688Temp = ICM42688.getAccelDataY()) != 0)
    {
        acc[1] = ICM42688Temp;
    }
    if ((ICM42688Temp = ICM42688.getAccelDataZ()) != 0)
    {
        acc[2] = ICM42688Temp;
    }
    if ((ICM42688Temp = ICM42688.getGyroDataX()) != 0)
    {
        gyro[0] = ICM42688Temp;
    }
    if ((ICM42688Temp = ICM42688.getGyroDataY()) != 0)
    {
        gyro[1] = ICM42688Temp;
    }
    if ((ICM42688Temp = ICM42688.getGyroDataZ()) != 0)
    {
        gyro[2] = ICM42688Temp;
    }
    // Serial.println(acc[0]);
    kalman.updateICM42688(acc, gyro);
    kalman.doKalman();
}

byte IMU42688::Update()
{
    float AngleCope[6] = {0};
    byte Gravity_cope = 0;

    // Read Angle and do basic check -----------------------------

    Gravity_cope = kalman.Gdir;
    if (Gravity_cope < 0 || Gravity_cope > 6)
    {
        ErrorCode = Err_IMU_Receive_Data_Error;
    }

    Angle[0] = kalman.getXAvg();
    Angle[1] = kalman.getYAvg();
    Angle[2] = kalman.getZAvg();
    AngleShow[0] = kalman.getUIX();
    AngleShow[1] = kalman.getUIY();
    AngleShow[2] = kalman.getUIZ();

    Gravity = Gravity_cope;
    SensorTemperature = tempRaw;
    ErrorCode = IMU_Update_Success;
    
    // Check Warm Up Time ------------------------------
    if (fWarmUp != 100 && millis() - IMUStart < 60 * 1000)
    {
        ErrorCode = Err_IMU_Not_Warm_Up;
        fWarmUp = (millis() - IMUStart) / 600;
    }
    else if (fWarmUp != 100)
    {
        fWarmUp = 100;
    }
    return ErrorCode;
} // end void Update()
