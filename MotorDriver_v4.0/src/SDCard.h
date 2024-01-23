/*
 *CreateFile : Create .csv with NewFileName (or NewFileName(n), auto detect) if SD card insert
 *             New file contain headling
 *             Serial print "SD Card Fail" if SD card initialize failed
 *WriteFile  : Write String into New File
 */
#ifndef SDCard_h
#define SDCard_h

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

class SDCard
{
public:
  void SetPin(byte sck, byte miso, byte mosi, byte ss);
  byte Save(String FolderName, String &SaveString);
  byte SDOK = 0;
  byte Err_SD_Off = 1;
  byte Err_SD_begin_Failed = 2;
  byte Err_SD_Open_Failed = 3;
  byte Err_File_Open_Failed = 4;
  byte Err_File_Not_Create = 5;
  byte Err_File_Write_Failed = 6;
  String path;
  String DebugString = "";
  int LastCheck = 0;

private:
  SPIClass *spi = NULL;
  byte SSPin;
  File root;
  File rootForCreate;
  File FileName;
  int temp;
  int AvoidWDTimeOut;
  int UsedSpace = 0;
  int CheckCount = -1;
  int AvoidWDTimeOutCount = 0;
  bool SwichMode = true;
  bool Initialize = false;
  void Swich();
  void CheckFault(bool add, String DebugPrint);
  void CheckSaveString(String &SaveString);
  void CreateFile(String FolderName);
  bool WriteFile(String SaveString);
  byte CheckState();
  String DebugPath = "/Debug.txt";
};

#endif
