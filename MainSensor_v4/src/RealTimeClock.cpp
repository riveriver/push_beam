#include "RealTimeClock.h"
#include <Arduino.h>
#include <ESP32Time.h>
#include "SerialDebug.h"

RTC_DS3231 rtc;
ESP32Time Inrtc;

void RealTimeClock::Initialize()
{
  if (!rtc.begin())
  {
    Debug.print("[Clock] Couldn't find RTC! Used System Upload Time : ");
    Debug.print(__DATE__);
    Debug.print(" ");
    Debug.println(__TIME__);
    RtcBegin = false;
    now = DateTime(CompileTime.unixtime() + millis() / 1000);
  }
  else
  {
    if (rtc.lostPower())
    {
      RtcLostPower = true;
      Debug.print("[Clock] RTC lost power, reset the time : ");
      Debug.print(__DATE__);
      Debug.print(" ");
      Debug.println(__TIME__);
      rtc.adjust(CompileTime);
    }

    // Load the time to give an initial time.
    for (int i = 0; i < 10; i++)
    {
      if (rtc.now().isValid())
      {
        now = rtc.now();
        CompileTime = DateTime(now.unixtime() - millis() / 1000);
        Debug.println("[Clock] RTC Start, now = " + DateTimeStamp());
        Inrtc.setTime(now.unixtime());// Set esp internal rtc time (For SDCard modified time)
        return;
      }
    }
    RtcBegin = false;
    now = DateTime(CompileTime.unixtime() + millis() / 1000);
    Inrtc.setTime(now.unixtime());
    Debug.println("[Clock] Can't Load RTC Time, try to reset the RTC (turn off main power and install the RTC battary again)");
  };
}

void RealTimeClock::update()
{
  // If using now = rtc.now() directly in date-time string generating function directly,
  // the library may consume large amount of ROM and have high possibility to crash.
  // Update the date-time information in individual loop cycle will be more stable.
  if (!RtcBegin)
  {
    now = DateTime(CompileTime.unixtime() + millis() / 1000);
  }
  else if (rtc.begin() && rtc.now().isValid())
  {
    now = rtc.now();
    RTCFalidCount = 0;
  }
  else
  {
    RTCFalidCount++;
    Debug.println("[Clock] RTC Read Error ( " + String(RTCFalidCount) + " )");
    if (RTCFalidCount > 3)
    {
      now = DateTime(CompileTime.unixtime() + millis() / 1000);
    }
  }
}

int RealTimeClock::NowSec()
{
  return now.unixtime();
}

String RealTimeClock::TimeStamp(String NowSet, String str)
{
  NowSet.toUpperCase();
  DateTime T;
  if (NowSet == "NOW")
  {
    T = now;
  }
  else if (NowSet == "SET")
  {
    T = UserSetTimeBuffer;
  }
  else
  {
    Debug.println("[Clock] TimeStamp print Error. T = now");
  }

  String Now = "";

  if (T.hour() < 10)
  {
    Now += "0";
  }
  Now += String(T.hour()) + str;
  if (T.minute() < 10)
  {
    Now += "0";
  }
  Now += String(T.minute()) + str;
  if (T.second() < 10)
  {
    Now += "0";
  }
  Now += String(T.second());

  return Now;
}

String RealTimeClock::DateStamp(String NowSet, String YMD, String str, int YearDigit)
{
  NowSet.toUpperCase();
  DateTime T;
  if (NowSet == "NOW")
  {
    T = now;
  }
  else if (NowSet == "SET")
  {
    T = UserSetTimeBuffer;
  }
  else
  {
    Debug.println("[Clock] TimeStamp print Error. T = now");
  }

  String Today = "";
  if (YMD == "DMY")
  {
    if (T.day() < 10)
    {
      Today += "0";
    }
    Today += String(T.day()) + str;
    if (T.month() < 10)
    {
      Today += "0";
    }
    Today += String(T.month()) + str + String(T.year()).substring(min(4 - YearDigit, 4));
  }
  else
  {
    Today = String(T.year()).substring(min(4 - YearDigit, 4)) + str;
    if (T.month() < 10)
    {
      Today += "0";
    }
    Today += String(T.month()) + str;
    if (T.day() < 10)
    {
      Today += "0";
    }
    Today += String(T.day());
  }
  return Today;
}

String RealTimeClock::TimeStamp()
{
  return TimeStamp("Now", ":");
}

String RealTimeClock::DateStamp(String str, int YearDigit)
{
  return DateStamp("now", "YMD", str, YearDigit);
}


String RealTimeClock::DateTimeStamp()
{
  return (DateStamp("now", "YMD", "/", 4) + " " + TimeStamp());
}

char RealTimeClock::RTC_State()
{
  if (!RtcBegin || RTCFalidCount > 100)
  {
    return '*';
  }
  else if (RtcLostPower || RTCFalidCount > 3)
  {
    return '!';
  }
  else
  {
    return ' ';
  }
}

void RealTimeClock::ResetUserSetTimeBuffer()
{
  UserSetTimeBuffer = now;
  Cursor = 0;
}

void RealTimeClock::UserSetTime(int Do)
{
  if (Do == -1)
  {
    return;
  }
  switch (Cursor)
  {
  case 0:
    switch (Do)
    {
    case 0:
      UserSetTimeBuffer = UserSetTimeBuffer + TimeSpan(1, 0, 0, 0);
      break;
    case 1:
      Cursor++;
      break;
    case 2:
      UserSetTimeBuffer = UserSetTimeBuffer - TimeSpan(1, 0, 0, 0);
      break;
    }
    break;
  case 1:
    switch (Do)
    {
    case 0:
      UserSetTimeBuffer = TimeSpanYearMonth(UserSetTimeBuffer, 0, 1);
      break;
    case 1:
      Cursor++;
      break;
    case 2:
      UserSetTimeBuffer = TimeSpanYearMonth(UserSetTimeBuffer, 0, -1);
      break;
    }
    break;
  case 2:
    switch (Do)
    {
    case 0:
      UserSetTimeBuffer = TimeSpanYearMonth(UserSetTimeBuffer, 1, 0);
      break;
    case 1:
      Cursor++;
      break;
    case 2:
      UserSetTimeBuffer = TimeSpanYearMonth(UserSetTimeBuffer, -1, 0);
      break;
    }
    break;
  case 3:
    switch (Do)
    {
    case 0:
      UserSetTimeBuffer = UserSetTimeBuffer + TimeSpan(0, 1, 0, 0);
      break;
    case 1:
      Cursor++;
      break;
    case 2:
      UserSetTimeBuffer = UserSetTimeBuffer - TimeSpan(0, 1, 0, 0);
      break;
    }
    break;
  case 4:
    switch (Do)
    {
    case 0:
      UserSetTimeBuffer = UserSetTimeBuffer + TimeSpan(0, 0, 1, 0);
      break;
    case 1:
      Cursor++;
      break;
    case 2:
      UserSetTimeBuffer = UserSetTimeBuffer - TimeSpan(0, 0, 1, 0);
      break;
    }
    break;
  case 5:
    switch (Do)
    {
    case 0:
      UserSetTimeBuffer = UserSetTimeBuffer + TimeSpan(0, 0, 0, 1);
      break;
    case 1:
      Cursor++;
      break;
    case 2:
      UserSetTimeBuffer = UserSetTimeBuffer - TimeSpan(0, 0, 0, 1);
      break;
    }
    break;
  case 6:
    switch (Do)
    {
    case 0:
      Cursor = 0;
      break;
    case 2:
      Cursor = 3;
      break;
    case 1:
      if (RtcBegin)
      {
        rtc.adjust(UserSetTimeBuffer);
      }
      else
      {
        CompileTime = DateTime(UserSetTimeBuffer.unixtime() - millis() / 1000);
      }
      Cursor = 0;
      update();
      Inrtc.setTime(now.unixtime());
      Debug.println("[Clock] Adjust Time. now = "+ DateTimeStamp());
      break;
    }
    break;
  }
}

DateTime RealTimeClock::TimeSpanYearMonth(DateTime T, int addYear, int addMonth)
{
  int MonthOperator = (T.month() + addMonth) % 12;
  int YearOperator = T.year() + addYear + (T.month() + addMonth - MonthOperator) / 12;
  if (MonthOperator == 0)
  {
    YearOperator--;
    MonthOperator = 12;
  }
  return DateTime(YearOperator, MonthOperator, T.day(), T.hour(), T.minute(), T.second());
}
