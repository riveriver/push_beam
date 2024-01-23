/*
 * Not Complete : All
 */
#ifndef RealTimeClock_h
#define RealTimeClock_h
#include <Arduino.h>
#include <RTClib.h>

class RealTimeClock
{
public:
  void Initialize();
  String TimeStamp(String NowSet, String str);
  String DateStamp(String NowSet, String YMD, String str, int YearDigit);
  String TimeStamp();
  String DateStamp(String str, int YearDigit);
  String DateStamp(String NowSet, String Format);
  String DateTimeStamp();
  char RTC_State(); // * : RTC Failed, used built in. ! : RTC Lost Power
  void FatDateTime(uint16_t* date, uint16_t* time);
  //void SetTime(int Time[6]);
  void ResetUserSetTimeBuffer();
  void UserSetTime(int Do);
  void update();
  int Cursor = -1;
  int NowSec();
  DateTime now;
  bool RtcBegin = true;
  bool RtcLostPower = false;

private:
  DateTime CompileTime = DateTime(F(__DATE__), F(__TIME__));
  DateTime UserSetTimeBuffer = DateTime(F(__DATE__), F(__TIME__));
  DateTime TimeSpanYearMonth(DateTime T, int addYear, int addMonth);
  int RTCFalidCount = 0;
};
#endif
