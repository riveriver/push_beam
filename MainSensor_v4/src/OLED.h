/*
 * Not Complete : All
 * Show Battery
 * Show Sensor Connection
 * Show SD Card situation
 * Show Send Message
 * Show Save File Name
 * Show which reviever to send
 * Create new file
 */
#ifndef OLED_h
#define OLED_h
#include <Arduino.h>
#include <U8g2lib.h>
#include "RealTimeClock.h"
#include "MsgBuffer.h"
#include "SDCard.h"
#include "IMU.h"
#include "MsgBuffer.h"
#include "Battery.h"


class OLED
{
  public:
  void PowerSave(int mode);
  void Initialize();
  void Display(int &Mode,char &ID, int &ModeChoose, bool (&ButtonState)[3],
  int (&NodeNumber)[3], int (&DeviceType)[3][10],
  int UserSetting[8], bool ControlSetValue,
  String MsgBuffer, int Period, RealTimeClock &rClock,String (&Address)[3][10],IMU &rIMU, SDCard &rSDCard);
  int State[7] = {0,0,0,0,0,1,1};
  int DisplayStartFrom = 0;
  Battery *bat;
  
  private:
  void Begin();
  bool isU8G2Begin = false;
  void BatteryDisplay(int R, int T, int w, int h, int d);
  void ClockDisplay(RealTimeClock &rClock);
  void TimeDisplay(RealTimeClock &rClock);
  void MeshDisplay(char ID, int NodeNum[3],int DeviceType[3][10],String (&Address)[3][10]);
  void EulerDisplay(int NodeNum,char ID,IMU &rIMU);
  void SaveDisplay(String MsgBuffer,int Period,SDCard &rSDCard);
  
  void RightPrintDouble(double PrintDouble, int decimal, int x, int y, int unitlength);
  void ManualDisplay(int &ModeChoose);
  void DrawIcon(int x, int y, int mode, int IconMode, bool ButtonPress[3]);
  void FitBar(int Select,int DisplayLine, int TotalLine);
  void ControlDisplay(int UserSetting[8],bool ControlSetValue, IMU &rIMU);
  bool Flash(int TrueDue,int FalseDue);
  double FlashStart = millis();
  int BatteryShow1;
  int BatteryShow2;
  uint8_t I2C_Add = 0x00;



  const uint8_t *DefaultFont = u8g2_font_timR08_tr;

  //const uint8_t *ManuFont = u8g2_font_heisans_tr;
  const uint8_t *ManuFont = u8g2_font_helvR08_tr;

};


#endif
