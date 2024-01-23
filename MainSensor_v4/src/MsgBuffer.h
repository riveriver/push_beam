#ifndef MsgBuffer_h
#define MsgBuffer_h

#include <Arduino.h>

class MsgBuffer
{
  public:
  void Add(String& TimeStamp, double (&Ang)[3],float (&LocalAng)[3],String (&Address)[3][10],double (&Euler_Children)[10][3], int (&NodeNum)[3]);
  void Clear();
  void BufferCheck();
  String Msg;
  int WriteWhen = 1;

  private:
  String MsgNew;
  String AddrEdit = "NULL";
  int LineEnd;
  bool KeepLine = true;
  
};

#endif
