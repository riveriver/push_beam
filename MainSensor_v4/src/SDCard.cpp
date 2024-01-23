#include "SDCard.h"

#include <Arduino.h>
#include <FS.h>
#include <SPI.h>
#include <SD.h>
#include "SerialDebug.h"

void SDCard::SetPin()
{
  spi =  new SPIClass();
  spi->begin(14,13,12,SSPin);
  pinMode(SSPin, INPUT_PULLUP);
}

void SDCard::WriteFile(String SaveString)
{
  if (SaveString.length() != 0)
  {
    File DataFile = SD.open(path, FILE_APPEND);
    DataFile.println(SaveString);
    DataFile.close();
  }
  if (DebugString.length() != 0)
  {
    File DebugFile = SD.open(DebugPath, FILE_APPEND);
    DebugFile.print(DebugString);
    DebugFile.close();
    DebugString = "";
  }
}

void SDCard::CreateFile(String NewFilename)
{
  AvoidWDTimeOut = millis();
  String Headler = "RTC_Time, Battery, Roll, Pitch, Yaw, M Roll , M Pitch, M Yaw";
  for (int i = 1; i <= 9; i++)
  {
    Headler += ", S" + String(i) + " Address, S" + String(i) + " Roll, S" + String(i) + " Pitch, S" + String(i) + " Yaw";
  }

  if (AvoidWDTimeOutCount == 0)
  {
    rootForCreate = SD.open("/");
    if (!rootForCreate)
    {
      Debug.println("[SD Failed] open SD Card Faild");
      Initialize = false;
      return;
    }
    String pathtemp = "/" + NewFilename + ".csv";
    if (!SD.exists(pathtemp))
    {
      path = pathtemp;
      File NewFile = SD.open(path, FILE_APPEND);
      Debug.println("[SD] SD Start. Create New File : " + path);
      NewFile.println(Headler); // file headling
      NewFile.close();
      rootForCreate.close();
      Initialize = true;
      Show[2] = ": NULL";
      return;
    }
  }
  temp = 0;
  String StartWithPathName = NewFilename + "(";
  int StartWithPathNameLong = NewFilename.length() + 1;
  File file, NewFile;
  while (millis() - AvoidWDTimeOut < 4000)
  {
    file = rootForCreate.openNextFile(FILE_READ);
    if (!file)
    {
      path = "/" + NewFilename + "(" + String(temp + 1) + ").csv";
      NewFile = SD.open(path, FILE_APPEND);
      Debug.println("[SD] SD Start. Create New File : " + path);
      NewFile.println(Headler); // file headling
      NewFile.close();
      rootForCreate.close();
      Initialize = true;
      AvoidWDTimeOutCount++;
      Show[2] = ": " + String(AvoidWDTimeOutCount);
      return;
    }
    else
    {
      String ThisFileName = String(file.name());
      if (ThisFileName.endsWith(").csv") && ThisFileName.startsWith(StartWithPathName))
      {
        int ThisFileNum = ThisFileName.substring(StartWithPathNameLong).toInt();
        temp = max(temp, ThisFileNum);
      }
      file.close();
      AvoidWDTimeOutCount++;
      Show[2] = ": " + String(AvoidWDTimeOutCount);
    }
  }
  Debug.println("[SD] "+String(AvoidWDTimeOutCount)+" file search.");
  //Serial.printf("Free heap (SDCard.h 89) : %d \n", ESP.getFreeHeap());
}

void SDCard::Swich()
{
  SwichMode = !SwichMode;
  CheckCount = SwichMode - 1;
  Debug.print("[SD] Save Function Swich ");
  Debug.println(String(SwichMode));
}

void SDCard::Swich(bool State)
{
  SwichMode = State;
  CheckCount = SwichMode - 1;
  Debug.print("[SD] Save Function Swich ");
  Debug.println(String(SwichMode));
}

bool SDCard::CheckState()
{ // Close SD function if the Card Attach after 5 check
  if (!Initialize && !SD.begin(SSPin, *spi, 4000000))
  {
    Debug.println("[SD Failed] SD Begin Failed");
    CheckCount++;
    if (CheckCount > 5)
    {
      Swich();
    }
    return false;
  }

  if (SD.begin(SSPin, *spi, 4000000) == 0)
  {
    /* Check
    1.Is SD card insert
    2.Is SD card model well connect
    3.Is LoRa well connect (LoRa will return true even if LoRa VCC isn't well connect,
      but will cause the SD card begin failed)
    */
    Debug.println("[SD Failed] SD Begin Failed");
    CheckCount++;
    if (CheckCount > 5)
    {
      Swich();
    }
    return false;
  }
  root = SD.open("/");
  if (root == false)
  {
    Debug.println("[SD Failed] SD open Failed");
    CheckCount++;
    if (CheckCount > 5)
    {
      Swich();
    }
    root.close();
    return false;
  }
  root.close();

  if (!Initialize || CheckCount != 0 || UsedSpace == 0)
  {
    switch (SD.cardType())
    {
    case CARD_NONE:
      Show[0] = ": No Card";
      break;
    case CARD_MMC:
      Show[0] = ": MMC";
      break;
    case CARD_SD:
      Show[0] = ": SDSC";
      break;
    case CARD_SDHC:
      Show[0] = ": SDHC";
      break;
    default:
      Show[0] = ": Unknown";
      break;
    }
  }
  if (!Initialize || CheckCount != 0 || UsedSpace < 1024000)
  {
    UsedSpace = SD.usedBytes();
    if (UsedSpace < 1000)
    {
      Show[1] = ": " + String(UsedSpace) + " Byte/" + String(SD.cardSize() / (1024.0 * 1024.0 * 1024.0), 0) + " GB";
    }
    else if (UsedSpace / 1024.0 < 1000)
    {
      Show[1] = ": " + String(UsedSpace / 1024.0, 0) + " KB/" + String(SD.cardSize() / (1024.0 * 1024.0 * 1024.0), 0) + " GB";
    }
    else if (UsedSpace / (1024.0 * 1024.0) < 1000)
    {
      Show[1] = ": " + String(UsedSpace / (1024.0 * 1024.0), 0) + " MB/" + String(SD.cardSize() / (1024.0 * 1024.0 * 1024.0), 0) + " GB";
    }
    else
    {
      Show[1] = ": " + String(UsedSpace / (1024.0 * 1024.0 * 1024.0), 0) + " GB/" + String(SD.cardSize() / (1024.0 * 1024.0 * 1024.0), 0) + " GB";
    }
  }

  if (Initialize)
  {
    FileName = SD.open(path, FILE_READ);
    if (FileName == false)
    {
      FileName = SD.open(path, FILE_APPEND);
      if (FileName == false)
      {
        Debug.println("[SD Failed] File open Failed");
        CheckCount++;
        if (CheckCount > 5)
        {
          Swich();
        }
        FileName.close();
        return false;
      }
      else
      {
        CheckCount = 0;
        Debug.println("[SD] File " + path + " didn't exist. Create a new one.");
        FileName.close();
        return true;
      }
    }
    FileName.close();
  }
  if (CheckCount != 0)
  {
    Debug.println("[SD] Auto Save Start.");
  }
  CheckCount = 0;
  root.close();
  return true;
}
