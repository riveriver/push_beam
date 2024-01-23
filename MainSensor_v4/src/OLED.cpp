
#include "OLED.h"

#include "IMU.h"
#include <U8g2lib.h>
#include "SerialDebug.h"

/*
Setting U8G2 Here (Using SSD1309_128X64, I2C, No reset)
1. Name : NONAME0 / NONAME1 (when sometime require offset)
2. Wire : HW (Arduino Define SDA/SCL) / SW (User Define SDA/SCL)
3. Rotation : U8G2_R0 (0) / U8G2_R1 (90) / U8G2_R2 (180) / U8G2_R3 (-90) / U8G2_Mirror (Left Right)
*/
U8G2_SSD1309_128X64_NONAME0_F_HW_I2C u8g2(U8G2_R0);
// U8G2_SSD1309_128X64_NONAME0_F_SW_I2C u8g2(U8G2_R0,/*SCL*=/ 22, /*SDA=*/ 21);

void OLED::Begin()
{
  if (!isU8G2Begin)
  {
    while (millis() < 2000)
      ;
    Wire.begin();
    Wire.beginTransmission(60);
    byte error = Wire.endTransmission();
    if (error == 0)
      I2C_Add = 0x3C;
    else
    {
      Wire.beginTransmission(61);
      error = Wire.endTransmission();
      if (error == 0)
        I2C_Add = 0x3D;
    }
    
    if (I2C_Add == 0x00)
    {
      Debug.println("[OLED] OLED Address Not Found. Try to begin u8g2 anyway.");
    }
    else
    {
      u8g2.setI2CAddress(I2C_Add * 2);
      Debug.println("[OLED] OLED begin, Address = 0x" + String(I2C_Add, HEX));
    }
    u8g2.begin();
    u8g2.setFlipMode(0);
    isU8G2Begin = true;
    u8g2.clear();
    u8g2.sendBuffer();
  }
}

void OLED::PowerSave(int mode)
{
  Begin();
  if (mode == 1)
  {
    u8g2.clear();
  }
  u8g2.setPowerSave(mode);
}

void OLED::Initialize()
{
  Begin();
  u8g2.enableUTF8Print();
  u8g2.setFont(ManuFont);
  u8g2.setFontDirection(0);
  u8g2.clear();
  int w = u8g2.getStrWidth("Wonder Construct");
  u8g2.drawStr(64-w/2, 30, "Wonder Construct");
  w = u8g2.getStrWidth("Controller V4");
  u8g2.drawStr(64-w/2, 50, "Controller V4");
  u8g2.sendBuffer();
}

void OLED::Display(int &Mode, char &ID, int &ModeChoose, bool (&ButtonState)[3],
                   int (&NodeNum)[3], int (&DeviceType)[3][10],
                   int UserSetting[8], bool ControlSetValue,
                   String MsgBuffer, int Period, RealTimeClock &rClock, String (&Address)[3][10], IMU &rIMU, SDCard &rSDCard)
{
  u8g2.clearBuffer();
  u8g2.setCursor(0, 20);
  switch (Mode)
  {
  case 0:
    ManualDisplay(ModeChoose);
    break;
  case 1: // Angle
    EulerDisplay(NodeNum[0], ID, rIMU);
    break;
  case 2: // GPS
    break;
  case 3: // RTC
    TimeDisplay(rClock);
    break;
  case 4: // SD
    SaveDisplay(MsgBuffer, Period, rSDCard);
    break;
  case 5: // LoRa (Close)
    break;
  case 6: // Control
    ControlDisplay(UserSetting, ControlSetValue, rIMU);
    break;
  case 7: // BLE
    MeshDisplay(ID, NodeNum, DeviceType, Address);
    break;
  }

  if (Mode > 0)
  {
    switch (Mode)
    {
    case 1:
      DrawIcon(107, 32, 12, 2 + ButtonState[1], ButtonState);
      break;
    case 4:
      if (State[3] == 0)
      {
        DrawIcon(107, 32, 41, 2 + ButtonState[1], ButtonState);
      }
      else
      {
        DrawIcon(107, 32, 42, 2 + ButtonState[1], ButtonState);
      }
      break;
    case 6:
      DrawIcon(107, 32, 61, 2 + (!ControlSetValue), ButtonState);
      break;
    case 7:
      DrawIcon(107, 32, 71, 2 + (State[6] == -1 && State[5] != -1), ButtonState);
      break;
    default:
      DrawIcon(107, 32, 11, 2, ButtonState);
      break;
    }
    DrawIcon(107, 53, 10, 2 + ButtonState[2], ButtonState); // Exit
    BatteryDisplay(102, 0, 9, 5, 5);
  }
  else
  {
    BatteryDisplay(128, 0, 9, 5, 5);
  }

  if (Mode == 1)
  {
    DrawIcon(107, 11, Mode, 2 + (rIMU.CopeFailed == 0), ButtonState);
  }
  else if (Mode == 3)
  {
    DrawIcon(107, 11, Mode, 2 + (rClock.RtcBegin), ButtonState);
  }
  else if (Mode == 2 || Mode == 4 || Mode == 5)
  {
    DrawIcon(107, 11, Mode, 2 + (State[Mode - 1] != -1), ButtonState);
  }
  else if (Mode == 7)
  {
    DrawIcon(107, 11, Mode, 2 + (State[5] != -1), ButtonState);
  }
  else if (Mode == 6)
  {
    DrawIcon(107, 11, Mode, 2 + (State[Mode - 1] == -1), ButtonState);
  }

  u8g2.setFont(u8g2_font_tinyface_tr);
  u8g2.setCursor(40, 5);
  if (NodeNum[0] != 0 || Mode == 6 || NodeNum[1] != 0)
  {
    if (State[5] != -1 || Flash(1000, 1000))
    {
      u8g2.print(String(ID) + ":" + String(NodeNum[0]));
    }
  }
  else
  {
    u8g2.print(ID);
  }

  if (State[3] == -1 || (State[3] != 0 && Flash(800, 800)))
  {
    u8g2.setCursor(60, 5);
    u8g2.print("SD 0FF");
  }

  ClockDisplay(rClock);
  u8g2.setFont(DefaultFont);
  u8g2.sendBuffer();
}

void OLED::DrawIcon(int x, int y, int mode, int IconMode, bool ButtonPress[3])
{
  // IconMode : 0(Icon only), 1(Icon with Title), 2(Icon with frame),
  //            3(Icon with frame black),
  u8g2.setFontPosCenter();
  if (mode == 8)
  {
    mode = 1;
  }
  if (mode == 0)
  {
    mode = 7;
  }
  if (mode < 10 && (IconMode == 2 || IconMode == 3) && ButtonPress[0])
  {
    if (Flash(250, 250))
    {
      IconMode = (IconMode - 1) % 2 + 2;
    }
  }
  if ((mode == 71) && ButtonPress[1])
  {
    if (Flash(250, 250))
    {
      IconMode = (IconMode - 1) % 2 + 2;
    }
  }
  if ((mode == 61) && (IconMode == 3) && ButtonPress[1])
  {
    if (Flash(250, 250))
    {
      IconMode = IconMode - 1;
    }
  }

  if (IconMode == 1)
  {
    u8g2.setFont(ManuFont);
    u8g2.setCursor(x + 23, y + 1);
    switch (mode)
    {
    case 1:
      u8g2.print("Angle");
      break;
    case 2:
      u8g2.print("Position");
      break;
    case 3:
      u8g2.print("Time");
      break;
    case 4:
      u8g2.print("Auto Save");
      break;
    case 5:
      u8g2.print("LoRa");
      break;
    case 6:
      u8g2.print("Control");
      break;
    case 7:
      u8g2.print("Connection");
      break;
    }
  }

  if (IconMode == 2)
  {
    u8g2.drawRFrame(x - 1, y - 11, 22, 22, 5);
  }
  else if (IconMode == 3)
  {
    u8g2.drawRFrame(x - 1, y - 11, 22, 22, 5);
    u8g2.drawRBox(x + 1, y - 9, 18, 18, 3);
    u8g2.setDrawColor(0);
  }

  switch (mode)
  {
  case 1: // Angle
    u8g2.setFont(u8g2_font_unifont_t_77);
    u8g2.drawGlyph(x + 2, y, 9883);
    break;
  case 2: // Position
    u8g2.setFont(u8g2_font_open_iconic_all_2x_t);
    u8g2.drawGlyph(x + 2, y, 201);
    // u8g2.drawGlyph(x+2,y,209);
    break;
  case 3: // Time
    u8g2.setFont(u8g2_font_open_iconic_all_2x_t);
    u8g2.drawGlyph(x + 2, y, 123);
    break;
  case 4: // Auto Save
    u8g2.setFont(u8g2_font_iconquadpix_m_all);
    u8g2.drawGlyph(x + 4, y - 1, 66);
    if (IconMode == 3)
    {
      u8g2.drawBox(x + 18, y - 7, 2, 14);
      u8g2.setDrawColor(1);
      u8g2.drawRFrame(x + 1, y - 9, 18, 18, 3);
      u8g2.drawBox(x + 8, y + 1, 3, 3);
      u8g2.setDrawColor(0);
    }
    else
    {
      u8g2.setDrawColor(0);
      u8g2.drawBox(x + 8, y + 1, 3, 3);
      u8g2.setDrawColor(1);
    }
    u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
    u8g2.setFontDirection(1);
    u8g2.drawGlyph(x + 13, y, 205);
    u8g2.setFontDirection(0);
    break;
  case 5: // LoRa
    u8g2.setFont(u8g2_font_open_iconic_all_2x_t);
    u8g2.drawGlyph(x + 2, y, 247);
    // u8g2.drawGlyph(x+2,y,84);
    break;
  case 6:
    u8g2.setFont(u8g2_font_unifont_t_77);
    u8g2.drawGlyph(x + 2, y, 9935);
    break;
  case 7: // BLE
    u8g2.setFont(u8g2_font_open_iconic_all_2x_t);
    u8g2.drawGlyph(x + 3, y, 94);
    if (IconMode == 3)
    {
      u8g2.drawVLine(x + 2, y - 3, 6);
      u8g2.drawVLine(x + 4, y - 1, 2);
      u8g2.drawVLine(x + 15, y - 1, 2);
      u8g2.drawVLine(x + 17, y - 3, 6);
    }
    break;
  case 10: // Exit
    u8g2.setFont(u8g2_font_open_iconic_all_2x_t);
    u8g2.drawGlyph(x + 1, y + 1, 64);
    u8g2.setDrawColor(2);
    u8g2.drawBox(x + 1, y - 1, 2, 2);
    break;
  case 11: // X
    u8g2.setFont(u8g2_font_open_iconic_all_2x_t);
    u8g2.drawGlyph(x + 2, y, 283);
    break;
  case 12: // =
    u8g2.drawBox(x + 4, y + 2, 12, 3);
    u8g2.drawBox(x + 4, y - 5, 12, 3);
    break;
  case 41:
    u8g2.setFont(u8g2_font_open_iconic_all_2x_t);
    u8g2.drawGlyph(x + 3, y + 1, 167);
    u8g2.setDrawColor((u8g2.getDrawColor() + 1) % 2);
    u8g2.drawFrame(x + 3, y - 8, 14, 16);
    u8g2.drawBox(x + 3, y + 7, 14, 2);
    u8g2.setFont(DefaultFont);
    u8g2.drawGlyph(x + 5, y + 4, 43);
    u8g2.setDrawColor((u8g2.getDrawColor() + 1) % 2);
    break;
  case 42:
    u8g2.setFont(u8g2_font_open_iconic_all_2x_t);
    u8g2.drawGlyph(x + 3, y, 271);
    break;
  case 61:
    u8g2.setFont(u8g2_font_open_iconic_all_2x_t);
    u8g2.drawGlyph(x + 2, y, 282);
    break;
  case 71:
    u8g2.setFont(u8g2_font_open_iconic_all_2x_t);
    u8g2.drawGlyph(x + 2, y, 198);
    u8g2.setFont(u8g2_font_u8glib_4_tr);
    u8g2.drawStr(x + 13, y + 7, "A");
    break;
  }
  if (IconMode == 1)
  {
    u8g2.setFont(u8g2_font_tinyface_tr);
    switch (mode)
    {
    case 4:
      if (State[mode - 1] > 0)
      {
        u8g2.drawGlyph(x + 18, y - 4, 33);
      }
    }
  }
  u8g2.setDrawColor(1);
  u8g2.setFont(DefaultFont);
  u8g2.setFontPosBaseline();
}

void OLED::ManualDisplay(int &ModeChoose)
{
  u8g2.drawRFrame(22, 25, 106, 20, 3);
  if (State[*(&ModeChoose - 1)] == -1)
  {
    u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
    if (ModeChoose == 6) // if Start Command
    {
      if (Flash(1000, 1000))
      {
        u8g2.drawGlyph(115, 39, 174);
      }
      else
      {
        u8g2.drawGlyph(115, 39, 173);
      }
    }
    else if (*(&ModeChoose - 1) != 7)
    {
      u8g2.drawGlyph(115, 39, 235);
    }

    u8g2.setFont(DefaultFont);
  }
  bool FALSE[3] = {false, false, false};
  DrawIcon(24, 35, ModeChoose, 1, FALSE);
  DrawIcon(0, 17, *(&ModeChoose - 1), 1, FALSE);
  DrawIcon(0, 53, *(&ModeChoose + 1), 1, FALSE);
}

void OLED::EulerDisplay(int NodeNum, char ID, IMU &rIMU)
{
  u8g2.setFont(u8g2_font_tinyface_tr);
  u8g2.setCursor(0, 14);
  u8g2.print("EULER Z - Y - X");
  u8g2.setCursor(0, 30);
  if (NodeNum > 0)
  {
    u8g2.print("AVG ");
    for (int i = 0; i < 3; i++)
    {
      u8g2.drawGlyph(36 + 29 * i, 23, 88 + i); // Prit X Y Z
      RightPrintDouble(rIMU.EulerAvg[i], 2, 46 + 29 * i, 30, 4);
    }
    u8g2.drawHLine(0, 31, 105);
    FitBar(State[0], 4, NodeNum + 1);
    for (int j = 0; j < 4; j++)
    {
      if (j + DisplayStartFrom < NodeNum + 1)
      {
        if (j == State[0] - DisplayStartFrom)
        {
          u8g2.drawBox(0, 34 + j * 8, 105, 7);
          u8g2.setDrawColor(0);
        }

        switch (rIMU.Count[j + DisplayStartFrom])
        {
        case 0:
          u8g2.drawFrame(2, 35 + j * 8, 5, 5);
          break;
        case 1:
          u8g2.drawBox(2, 35 + j * 8, 5, 5);
          break;
        case -1:
          u8g2.drawFrame(2, 35 + j * 8, 5, 5);
          u8g2.drawHLine(1, 37 + j * 8, 7);
          break;
        }
        u8g2.setCursor(10, 40 + j * 8);
        if (j + DisplayStartFrom == 0)
        {
          u8g2.print("L");
          for (int i = 0; i < 3; i++)
          {
            RightPrintDouble(rIMU.EulerLocal[i], 2, 46 + 29 * i, 40, 4);
          }
        }
        else
        {
          u8g2.print(ID + String(j + DisplayStartFrom));
          for (int i = 0; i < 3; i++)
          {
            RightPrintDouble(*(rIMU.pChildrenEuler + (j + DisplayStartFrom - 1) * 3 + i), 2, 46 + 29 * i, 40 + j * 8, 4);
          }
        }
        u8g2.setDrawColor(1);
      }
    }
  }
  else
  {
    u8g2.print("ANG :");
    for (int i = 0; i < 3; i++)
    {
      u8g2.drawGlyph(36 + 29 * i, 23, 88 + i); // Prit X Y Z
      RightPrintDouble(rIMU.EulerLocal[i], 2, 46 + 29 * i, 30, 4);
    }
  }
  u8g2.setFont(DefaultFont);
}

void OLED::MeshDisplay(char ID, int NodeNum[3], int DeviceType[3][10], String (&Address)[3][10])
{
  u8g2.setFont(ManuFont);
  if (NodeNum[0] + NodeNum[1] + NodeNum[2] == 0)
  {
    u8g2.drawStr(18, 40, "No  Connection");
  }
  else
  {
    u8g2.drawStr(0, 16, "Station");
    u8g2.drawGlyph(35, 16, ID);
    u8g2.drawStr(50, 16, "D   / S");
    int SensorCount = 0;
    for (int i = 0; i < NodeNum[0]; i++)
    {
      if (DeviceType[0][i] == 2)
      {
        SensorCount++;
      }
    }
    u8g2.drawGlyph(58, 16, NodeNum[0] - SensorCount + 48);
    u8g2.drawGlyph(80, 16, SensorCount + 48);
    u8g2.drawHLine(0, 17, 103);
    FitBar(State[6] - 1, 4, NodeNum[0] + NodeNum[1] + NodeNum[2]);
    if (State[5] != -1 && State[6] != -1 && NodeNum[0] + NodeNum[1] + NodeNum[2] > 0)
    {
      u8g2.drawStr(0, 27 + 12 * (State[6] - 1 - DisplayStartFrom), ">");
    }
    String Show;
    int Type;
    for (int i = 0; i < 4; i++)
    {
      Type = -1;
      Show = "";
      if (NodeNum[0] > DisplayStartFrom + i)
      {
        u8g2.drawGlyph(7, 28 + 12 * i, ID);
        u8g2.drawGlyph(15, 28 + 12 * i, i + 49 + DisplayStartFrom);
        u8g2.drawGlyph(20, 28 + 12 * i, '.');
        Show = Address[0][i + DisplayStartFrom];
        Type = DeviceType[0][i + DisplayStartFrom];
      }
      else if (NodeNum[1] > i + DisplayStartFrom - NodeNum[0])
      {
        u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
        u8g2.drawGlyph(10, 28 + 12 * i, 198);
        u8g2.setFont(ManuFont);
        int ConnectCount = 0;
        int j = 0;
        while (ConnectCount < i + DisplayStartFrom - NodeNum[0] + 1)
        {
          if (Address[1][j] != "")
          {
            ConnectCount++;
          }
          j++;
        }
        Show = Address[1][j - 1];
        Type = DeviceType[1][j - 1];
      }
      else if (NodeNum[2] > i + DisplayStartFrom - NodeNum[0] - NodeNum[1])
      {
        Show = Address[2][i + DisplayStartFrom - NodeNum[0] - NodeNum[1]];
        Type = DeviceType[2][i + DisplayStartFrom - NodeNum[0] - NodeNum[1]];
      }
      else
      {
        break;
      }
      switch (Type)
      {
      case 0:
        u8g2.drawStr(25, 28 + 12 * i, "Driver*");
        break;
      case 1:
        u8g2.drawStr(25, 28 + 12 * i, "Driver");
        break;
      case 2:
        u8g2.drawStr(25, 28 + 12 * i, "Sensor");
        break;
      }
      u8g2.drawHLine(58, 24 + 12 * i, 4);
      Show.toUpperCase();
      int At[4] = {12, 13, 15, 16};
      for (int j = 0; j < 4; j++)
      {
        char A = Show.charAt(At[j]);
        int w = u8g2.getStrWidth(&A);
        u8g2.drawGlyph(71 - (w + 1) / 2 + 7 * j, 28 + 12 * i, A); // 83
      }
      u8g2.setDrawColor(1);
    }
  }
  u8g2.setFont(DefaultFont);
}

void OLED::ControlDisplay(int UserSetting[8], bool SettingMode, IMU &rIMU)
{
  u8g2.drawHLine(0, 20, 105);
  u8g2.setFont(u8g2_font_tinyface_tr);
  RightPrintDouble(rIMU.EulerAvg[0], 2, 31, 18, 4);
  RightPrintDouble(rIMU.EulerAvg[1], 2, 62, 18, 4);
  RightPrintDouble(rIMU.EulerAvg[2], 2, 93, 18, 4);

  u8g2.setFont(ManuFont);
  if (State[5] > 0 && State[5] < 10)
  {
    u8g2.drawStr(0, 20 + 14 * State[5], ">");
  }
  else if (State[5] > 30 && UserSetting[6] > 9)
  {
    u8g2.drawHLine((State[5] == 31) ? 40 : 57, 63, (State[5] == 31) ? 14 : 7);
  }
  else if (State[5] > 10)
  {
    int LineX[3] = {40, 50, 57};
    int LineY[3] = {35, 49, 63};
    u8g2.drawHLine(LineX[State[5] % 10 - 1], LineY[State[5] / 10 - 1], 7);
  }

  u8g2.drawStr(7, 34, "a");
  u8g2.drawStr(13, 34, "ngle");
  u8g2.drawStr(7, 48, "hight");
  u8g2.drawStr(6, 62, "speed");

  for (int i = 0; i < 3; i++)
  {
    u8g2.drawStr(33, 34 + 14 * i, ":");
  }

  u8g2.drawStr(41, 34, (UserSetting[0] > 0) ? "+" : "-");
  u8g2.drawGlyph(51, 34, UserSetting[1] + 48);
  u8g2.drawGlyph(58, 34, UserSetting[2] + 48);
  u8g2.drawGlyph(41, 48, UserSetting[3] + 48);
  u8g2.drawGlyph(47, 48, '.');
  u8g2.drawGlyph(51, 48, UserSetting[4] + 48);
  u8g2.drawGlyph(58, 48, UserSetting[5] + 48);

  if (UserSetting[6] > 9)
  {
    u8g2.drawGlyph(41, 62, UserSetting[6] / 10 + 48);
    u8g2.drawGlyph(48, 62, UserSetting[6] % 10 + 48);
    u8g2.drawGlyph(54, 62, '.');
    u8g2.drawGlyph(58, 62, UserSetting[7] + 48);
  }
  else
  {
    u8g2.drawGlyph(41, 62, UserSetting[6] + 48);
    u8g2.drawGlyph(47, 62, '.');
    u8g2.drawGlyph(51, 62, UserSetting[7] + 48);
  }

  u8g2.drawStr(73, 34, "degree");
  u8g2.drawStr(73, 48, "m");
  u8g2.drawStr(73, 62, "mm/s");
}

void OLED::SaveDisplay(String MsgBuffer, int Period, SDCard &rSDCard)
{
  u8g2.setFont(ManuFont);
  u8g2.drawHLine(0, 20, 101);
  if (State[3] == 0)
  {
    if (rSDCard.path == "")
    {
      u8g2.setCursor(0, 17);
      u8g2.print("Creating");
      if (Flash(1000, 1000))
      {
        u8g2.setCursor(40, 17);
        u8g2.print("......");
      }
      else
      {
        u8g2.setCursor(40, 17);
        u8g2.print("............");
      }
    }
    else
    {
      u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
      u8g2.drawGlyph(0, 17, 146);
      // u8g2.setFont(DefaultFont);
      u8g2.setFont(ManuFont);
      u8g2.setCursor(10, 17);
      u8g2.print(": " + rSDCard.path);
    }
    u8g2.setCursor(0, 32);
    u8g2.print("Card");
    u8g2.setCursor(28, 32);
    u8g2.print(rSDCard.Show[0]);
    u8g2.setCursor(0, 48);
    u8g2.print("Space");
    u8g2.setCursor(28, 48);
    u8g2.print(rSDCard.Show[1]);
    u8g2.setCursor(0, 64);
    u8g2.print("# File");
    u8g2.setCursor(28, 64);
    u8g2.print(rSDCard.Show[2]);
  }
  else
  {
    if (State[3] == -1)
    {
      u8g2.setCursor(0, 17);
      u8g2.print("Function  Off");
    }
    else if (Flash(1200, 800))
    {
      u8g2.setCursor(0, 17);
      u8g2.print("!  No  Card  Insert");
    }
    u8g2.setCursor(0, 32);
    u8g2.print("Buffer");
    u8g2.setCursor(31, 32);
    u8g2.print(":");
    u8g2.drawBox(36, 24, (MsgBuffer.length() * 65) / 18000, 8);
    u8g2.setCursor(0, 48);
    u8g2.print("Begine");
    u8g2.setCursor(31, 48);
    u8g2.print(":");
    u8g2.setCursor(36, 48);
    u8g2.print(MsgBuffer.substring(11, 19));
    u8g2.setCursor(0, 64);
    u8g2.print("Period");
    u8g2.setCursor(31, 64);
    u8g2.print(":");
    u8g2.setCursor(36, 64);
    u8g2.print(String(Period) + " s");
  }
  u8g2.setFont(DefaultFont);
}

void OLED::TimeDisplay(RealTimeClock &rClock)
{
  u8g2.setFont(ManuFont);
  u8g2.drawStr(0, 18, "Setting");
  u8g2.drawHLine(0, 20, 101);
  u8g2.setFont(DefaultFont);
  u8g2.drawStr(0, 33, "Date");
  u8g2.drawStr(23, 33, ":");
  u8g2.drawStr(30, 33, rClock.DateStamp("Set", "DMY", " / ", 4).c_str());
  u8g2.drawStr(0, 47, "Time");
  u8g2.drawStr(23, 47, ":");
  u8g2.drawStr(30, 47, rClock.TimeStamp("Set", " : ").c_str());
  u8g2.drawRFrame(35, 52, 33, 12, 3);
  switch (State[2])
  {
  case 0:
    u8g2.drawHLine(29, 34, 11);
    break;
  case 1:
    u8g2.drawHLine(46, 34, 11);
    break;
  case 2:
    u8g2.drawHLine(63, 34, 21);
    break;
  case 3:
    u8g2.drawHLine(29, 48, 11);
    break;
  case 4:
    u8g2.drawHLine(46, 48, 11);
    break;
  case 5:
    u8g2.drawHLine(63, 48, 11);
    break;
  case 6:
    u8g2.drawRBox(35, 52, 33, 12, 3);
    u8g2.setDrawColor(0);
  }
  u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
  u8g2.drawGlyph(38, 62, 282);
  u8g2.setFont(DefaultFont);
  u8g2.drawStr(48, 62, "Save");
  u8g2.setDrawColor(1);
}

void OLED::RightPrintDouble(double PrintDouble, int decimal, int x, int y, int unitlength)
{
  String PrintString = String(PrintDouble, decimal);
  int PrintStringLength;
  if (PrintDouble >= 0)
  {
    PrintStringLength = PrintString.length() * unitlength - (unitlength - 1);
  }
  else
  {
    PrintStringLength = PrintString.length() * unitlength - (unitlength * 2 - 4);
  }
  u8g2.setCursor(x - PrintStringLength, y);
  u8g2.print(PrintString);
}

void OLED::BatteryDisplay(int R, int T, int w, int h, int d)
{
  // Put at Top Right Corner of 128X64 Screen
  u8g2.drawFrame((R - w), T, w - 1, h);
  u8g2.drawBox((R - 1), T + 1, 1, h - 2);
  if (bat->Percent > 100)
  {
    u8g2.drawBox((R + 1 - 2 * w - d) + (w + d), T + 1, (w - 3), h - 2);
    u8g2.setFont(u8g2_font_tinyface_tr);
    u8g2.setDrawColor(0);
    u8g2.drawStr(R - w / 2 - 2, T + h + 1, "*");
    u8g2.setDrawColor(1);
  }
  else if (bat->Percent > 0)
  {
    u8g2.drawBox((R + 1 - 2 * w - d) + (w + d), T + 1, int(bat->Percent / 100.0 * (w - 3)), h - 2);
  }
}

void OLED::ClockDisplay(RealTimeClock &rClock)
{
  u8g2.setFont(u8g2_font_tinyface_tr);
  u8g2.drawStr(0, 5, rClock.TimeStamp().c_str());
  u8g2.drawGlyph(29, 5, rClock.RTC_State());
  u8g2.setFont(DefaultFont);
}

bool OLED::Flash(int TrueDue, int FalseDue)
{
  if (millis() - FlashStart < TrueDue)
  {
    return true;
  }
  else if (millis() - FlashStart < TrueDue + FalseDue)
  {
    return false;
  }
  else
  {
    FlashStart = millis();
    return true;
  }
}

void OLED::FitBar(int Select, int DisplayLine, int TotalLine)
{
  // Line 1 -> Select 0, DisplayStartFrom 0
  if (Select == -2)
  {
    if (TotalLine < DisplayStartFrom + DisplayLine)
    {
      DisplayStartFrom = TotalLine - DisplayLine;
    }
    if (DisplayStartFrom < 0)
    {
      DisplayStartFrom = 0;
    }
  }
  else if (Select <= 0 || TotalLine <= DisplayLine)
  {
    DisplayStartFrom = 0;
  }
  else
  {
    if (Select < DisplayStartFrom)
    {
      DisplayStartFrom = Select;
    }
    else if (Select > DisplayStartFrom + DisplayLine - 1)
    {
      DisplayStartFrom = Select - DisplayLine + 1;
    }
    if (TotalLine < DisplayStartFrom + DisplayLine)
    {
      DisplayStartFrom = TotalLine - DisplayLine;
    }
  }
}