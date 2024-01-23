// Z-Y'-X'' Euler (i.e. X-Y-Z Euler)
// So far we don't have exact world Z direction rotation information.
// Try another method : (for wall roll only)
// When attach on wall, we wish that the axis z of the IMU have no displacement on world Z direction
// i.e. cos(pitch)* cos(roll) ~ 0
// sin(pitch) represent the world Z displacement of the IMU axis x
// That is, we need roll ~ 90
// Average the children roll directly !!

#include "IMU.h"
#include <Wire.h>
#include <JY901.h>
#include <HardwareSerial.h>
#include <Preferences.h>
#include "SerialDebug.h"
#define PI 3.1415926535897932384626433832795

extern HardwareSerial Serial1;

void IMU::Initialize(double *pReceiveAngle, int * pNodeNumber)
{
  Serial1.setRxBufferSize(256);
  Serial1.begin(9600, SERIAL_8N1, 38, 39); // RX: 26 TX: 27,
  memset(&Count, -1, sizeof(Count));
  memset(&AngleMeasure, 0, sizeof(AngleMeasure));
  Rbase.R00 = -1;
  Rbase.R01 = 0;
  Rbase.R02 = 0;
  Rbase.R10 = 0;
  Rbase.R11 = -1;
  Rbase.R12 = 0;
  Rbase.R20 = 0;
  Rbase.R21 = 0;
  Rbase.R22 = 1;
  pChildrenEuler = pReceiveAngle;
  pNodeNum = pNodeNumber;
}

bool IMU::Read()
{
  isUpdate = false;
  for (int j = 0; j < IMU_C * 2; j++)
  {
    // read multiple time in case of wrong coping
    CopeFailed++;
    while (Serial1.available())
    {
      JY901.CopeSerialData(Serial1.read());
      CopeFailed = 0;
    }

    if (!isWarmUp)
    {
      if (millis() > 25 * 1000)
      {
        isWarmUp = true;
        LocalEulerOutsideStDev = millis();
      }
      else
      {
        return false;
      }
    }

    if (CopeFailed == 0)
    {
      isUpdate = true;
      float NewAngleMeeasure[3] = {0, 0, 0};
      for (size_t i = 0; i < 3; i++)
      {
        NewAngleMeeasure[i] = JY901.stcAngle.Angle[i] / 32768.0 * 180.0;
        isUpdate = !(NewAngleMeeasure[i] == 0 || abs(NewAngleMeeasure[i]) > 180);
      }

      if (isUpdate)
      {
        memmove(&AngleMeasure[1][0], &AngleMeasure[0][0], sizeof(AngleMeasure[0][0]) * 3 * (IMU_C - 1));
        memcpy(&AngleMeasure[0][0], &NewAngleMeeasure, sizeof(NewAngleMeeasure));
        float Avg[3] = {0, 0, 0};

        for (int i = 0; i < IMU_C; i++)
        {
          Avg[0] += AngleMeasure[i][0] / IMU_C;
          Avg[1] += AngleMeasure[i][1] / IMU_C;
          Avg[2] += AngleMeasure[i][2] / IMU_C;
        }


        for (int i = 0; i < IMU_C * 3; i++)
        {
          if (abs(AngleMeasure[i / 3][i % 3] - Avg[i % 3]) > 50)
          {
            isUpdate = false;
            break;
          }
        }
      }
      if (isUpdate)
      {
        EulerLocal[0] = -AngleMeasure[0][0];
        EulerLocal[1] = -AngleMeasure[0][1];
        EulerLocal[2] = (AngleMeasure[0][2] > 0) ? (AngleMeasure[0][2] - 180) : (AngleMeasure[0][2] + 180);
        break;
      }
    }
    else
    {
      delay(10);
    }
  }
  if (CopeFailed > 5)
  {
    Count[0] = -1;
    memset(&EulerLocal, 0.0, sizeof(EulerLocal));
  }

  if (EulerLocal[0] == 0 && EulerLocal[1] == 0 && EulerLocal[2] == 0)
  {
    Count[0] = -1;
    Average();
  }
  else if (Count[0] == -1)
  {
    Count[0] = (millis() - LocalEulerOutsideStDev < 2000);
    if(!Average())
      Count[0] = 1;
    Average();
  }
  return isUpdate;
}

bool IMU::Average()
{
  for (int i = 0; i < *pNodeNum; i++)
  {
    if (*(pChildrenEuler + 3 * i) == 0 && *(pChildrenEuler + 3 * i + 1) == 0 && *(pChildrenEuler + 3 * i + 2) == 0)
    {
      Count[i + 1] = -1;
    }
    else if (Count[i + 1] == -1)
    {
      Count[i + 1] = 1; // default as include
    }
  }
  double Roll_avg;
  bool isDelete;
  bool is_All_Angle_in_StDev = true;
  do
  {
    isDelete = false;
    double roll_sum = 0;
    int CountNum = 0;
    if (Count[0] == 1)
    {
      roll_sum = EulerLocal[0];
      CountNum = 1;
    }
    for (int i = 0; i < *pNodeNum; i++)
    {
      if (Count[i + 1] == 1)
      {
        roll_sum += *(pChildrenEuler + 3 * i);
        CountNum++;
      }
    }

    if (CountNum == 0)
    {
      EulerAvg[0] = 0;
      AngleDifference = 0;
      return false;
    }
    else if (CountNum < 3)
    {
      EulerAvg[0] = roll_sum / CountNum;
      AngleDifference = (Count[0] == 1) ? (EulerAvg[0] - EulerLocal[0]) : (EulerAvg[0] - *pChildrenEuler);
      return true;
    }

    Roll_avg = roll_sum / CountNum;

    double Std_sum = 0;
    if (Count[0] == 1)
    {
      Std_sum = pow(EulerLocal[0] - Roll_avg, 2);
    }
    for (int i = 0; i < *pNodeNum; i++)
    {
      if (Count[i + 1] == 1)
      {
        Std_sum += pow(*(pChildrenEuler + 3 * i) - Roll_avg, 2);
      }
    }

    double Std = max(pow(Std_sum / CountNum, 0.5), 2.0);
    if (Count[0] == 1 && abs(EulerLocal[0] - Roll_avg) > Std)
    {
      Count[0] = -1;
      is_All_Angle_in_StDev = false;
      LocalEulerOutsideStDev = (LocalEulerOutsideStDev == 0) ? millis() : LocalEulerOutsideStDev;
      isDelete = true;
      Debug.println("Local Angle Value outside StDev.");
    }
    else
    {
      LocalEulerOutsideStDev = 0;
    }
    for (int i = 0; i < *pNodeNum; i++)
    {
      if (Count[i + 1] == 1 && abs(*(pChildrenEuler + 3 * i) - Roll_avg) > Std)
      {
        Count[i + 1] = -1;
        is_All_Angle_in_StDev = false;
        isDelete = true;
        Debug.println("No." + String(i + 1) + " Angle Value outside StDev");
      }
    }
  } while (isDelete);

  EulerAvg[0] = Roll_avg;
  AngleDifference = (Count[0] == 1) ? (Roll_avg - EulerLocal[0]) : (Roll_avg - *pChildrenEuler);
  return is_All_Angle_in_StDev;
}

double IMU::LocalAngleShift()
{
  EulerAvg[0] = (Count[0] == 1) ? (EulerLocal[0] + AngleDifference) : (*pChildrenEuler + AngleDifference);
  return EulerAvg[0];
}

void IMU::PointTo(int Action, int MaxNum)
{
  if (Action == 0 && IMUPointTo != 0)
  {
    IMUPointTo--;
  }
  else if (Action == 2 && IMUPointTo != MaxNum)
  {
    IMUPointTo++;
  }
  else if (Action == 1 && Count[IMUPointTo] != -1)
  {
    Count[IMUPointTo] = !Count[IMUPointTo];
    Average();
  }
  if (IMUPointTo > MaxNum)
  {
    IMUPointTo = MaxNum;
  }
}

Rotation IMU::Multipli(Rotation R1, Rotation R2)
{
  Rotation R;
  R.R00 = R1.R00 * R2.R00 + R1.R01 * R2.R10 + R1.R02 * R2.R20;
  R.R01 = R1.R00 * R2.R01 + R1.R01 * R2.R11 + R1.R02 * R2.R21;
  R.R02 = R1.R00 * R2.R02 + R1.R01 * R2.R12 + R1.R02 * R2.R22;
  R.R10 = R1.R10 * R2.R00 + R1.R11 * R2.R10 + R1.R12 * R2.R20;
  R.R11 = R1.R10 * R2.R01 + R1.R11 * R2.R11 + R1.R12 * R2.R21;
  R.R12 = R1.R10 * R2.R02 + R1.R11 * R2.R12 + R1.R12 * R2.R22;
  R.R20 = R1.R20 * R2.R00 + R1.R21 * R2.R10 + R1.R22 * R2.R20;
  R.R21 = R1.R20 * R2.R01 + R1.R21 * R2.R11 + R1.R22 * R2.R21;
  R.R22 = R1.R20 * R2.R02 + R1.R21 * R2.R12 + R1.R22 * R2.R22;
  return R;
}

Rotation IMU::ToR(double Degree[3])
{
  Rotation R;
  double c1, c2, c3, s1, s2, s3;
  s1 = sin(Degree[0] * PI / 180);
  s2 = sin(Degree[1] * PI / 180);
  s3 = sin(Degree[2] * PI / 180);
  c1 = cos(Degree[0] * PI / 180);
  c2 = cos(Degree[1] * PI / 180);
  c3 = cos(Degree[2] * PI / 180);
  // Z-Y'-X'' Euler (= X-Y-Z)
  R.R00 = c2 * c3;
  R.R01 = s1 * s2 * c3 - c1 * s3;
  R.R02 = c1 * s2 * c3 + s1 * s3;
  R.R10 = c2 * s3;
  R.R11 = s1 * s2 * s3 + c1 * c3;
  R.R12 = c1 * s2 * s3 - s1 * c3;
  R.R20 = -s2;
  R.R21 = s1 * c2;
  R.R22 = c1 * c2;
  return R;
}

Euler IMU::RToE(Rotation R)
{
  Euler E;
  // Z-Y'-X'' Euler
  if (1 - abs(R.R20) > 1e-6)
  {
    E.pitch = asin(-R.R20);
    E.roll = atan2(R.R21 / cos(E.pitch), R.R22 / cos(E.pitch)) * 180 / PI;
    E.yaw = atan2(R.R10 / cos(E.pitch), R.R00 / cos(E.pitch)) * 180 / PI;
    E.pitch *= 180 / PI;
  }
  else
  {
    if (R.R20 > 0)
    {
      E.pitch = 90;
    }
    else
    {
      E.pitch = -90;
    }
    E.yaw = 0;
    E.roll = atan2(-R.R12, R.R11) / PI * 180;
  }
  E.Ang[0] = E.roll;
  E.Ang[1] = E.pitch;
  E.Ang[2] = E.yaw;
  return E;
}
