/*
 *CreateFile : Create .csv with NewFileName (or NewFileName(n), auto detect) if SD card insert
 *             New file contain headling
 *             Serial print "SD Card Fail" if SD card initialize failed
 *WriteFile  : Write String into New File
 */
#ifndef SDCard_h
#define SDCard_h

#include <Arduino.h>
#include <SD.h>

class SDCard
{
public:
  void CreateFile(String NewFileName);
  void WriteFile(String SaveString);
  void Swich();
  void Swich(bool State);
  void SetPin();
  String path;
  String Show[3] = {": NULL", ": NULL", ": NULL"};
  bool SwichMode = false;
  bool Initialize = false;
  bool CheckState();
  int CheckCount = -1;
  int AvoidWDTimeOutCount = 0;
  String DebugString = "";
  String DebugPath = "/Debug.txt";

private:
  SPIClass *spi = NULL;
  const byte SSPin = 15;
  File root;
  File rootForCreate;
  int temp;
  int AvoidWDTimeOut;
  int UsedSpace = 0;
  File FileName;
  //void dateTime(uint16_t* date, uint16_t* time);
};



#endif
