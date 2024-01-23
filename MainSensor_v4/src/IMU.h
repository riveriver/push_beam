/*
 * Reading roll angle only
 */
#ifndef IMU_h
#define IMU_h
#include <Arduino.h>

#ifndef IMU_C
#define IMU_C 3
#endif

struct Euler{double roll, pitch, yaw, Ang[3];};
struct Rotation{double R00,R01,R02,R10,R11,R12,R20,R21,R22;};

class IMU
{
  public:
  void Initialize(double* pReceiveAngle, int * pNodeNumber);
  bool Read();
  float EulerLocal[3] = { 0.0, 0.0, 0.0};
  double EulerAvg[3]={ 0.0, 0.0, 0.0};
  void PointTo(int Action, int MaxNum);
  int IMUPointTo = 0;
  int Count[11];
  double LocalAngleShift();
  bool Average();
  int CopeFailed = false;
  double* pChildrenEuler;
  int * pNodeNum;

  private:
  float AngleMeasure[IMU_C][3];
  bool isUpdate = false;
  bool isWarmUp = false;
  int LocalEulerOutsideStDev = 0; // Time Stemp
  double AngleDifference = 0;
  Rotation Rbase;
  Rotation Multipli(Rotation R1, Rotation R2);
  Rotation ToR(double Degree[3]);
  Euler RToE(Rotation R);
  Euler ChangeBase;
};

#endif
