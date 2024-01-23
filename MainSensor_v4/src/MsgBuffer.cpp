/*
 * Not Complete : Change sprintf into strcat (for speed up)
 */

#include "MsgBuffer.h"
#include <Arduino.h>
#include "SerialDebug.h"

void MsgBuffer::Add(String& TimeStamp, double (&Ang)[3],float (&LocalAng)[3],String (&Address)[3][10],double (&Euler_Children)[10][3], int (&NodeNum)[3])
{
  if(Msg != ""){
    Msg = Msg + "\n";
  }
  Msg = Msg + TimeStamp + ", " + String(Ang[0],6) + ", " + String(Ang[1],6) + ", " + String(Ang[2],6) + ", " + String(LocalAng[0],6) + ", " + String(LocalAng[1],6) + ", " + String(LocalAng[2],6);
  for(int i = 0; i < NodeNum[0] ; i++){
    AddrEdit.setCharAt(0,Address[0][i].charAt(12));
    AddrEdit.setCharAt(1,Address[0][i].charAt(13));
    AddrEdit.setCharAt(2,Address[0][i].charAt(15));
    AddrEdit.setCharAt(3,Address[0][i].charAt(16));
    AddrEdit.toUpperCase();
    Msg = Msg + ", " + AddrEdit + ", " + String(Euler_Children[i][0],6) + ", " + String(Euler_Children[i][1],6) + ", " + String(Euler_Children[i][2],6);
  }
}

void MsgBuffer::BufferCheck()
{
  if(Msg.length()>16384){
    Debug.print("Message length : from ");Debug.print(String(Msg.length()));
    MsgNew = "";
    LineEnd = Msg.indexOf("\n");
    while(LineEnd != -1){
      if(KeepLine){
        MsgNew += Msg.substring(0, LineEnd+1);
      }
      Msg = Msg.substring(LineEnd+1);
      LineEnd = Msg.indexOf("\n");
      KeepLine = !KeepLine;
    }
    Msg = MsgNew+ Msg;
    WriteWhen *= 2;
    Debug.print(" to ");
    Debug.println(String(Msg.length()));
  }
}

void MsgBuffer::Clear()
{
  Msg = "";
  WriteWhen = 1;
}
