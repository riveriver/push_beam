/*
 *Not Complete : ButtonPress Callback
 */
#ifndef Button_h
#define Button_h
#include <Arduino.h>

//Button Press Detect ----------------------------------
const byte ButtonPin[] = {6, 7, 0};
// !! When esp32 built in wifi power turn off, GIPO 36 somehow can't detect long press. 
bool ShortPress[3] = {false, false, false};
bool LongPress[3] = {false, false, false};
bool ButtonState[3] = {false, false, false};
long unsigned int PressReleaseTimeStemp[] = {millis(),millis(),millis()};
int Mode = 0; 
int ModePage[7] = {7,1,3,4,6,7,1};
int *ModeChoose = &ModePage[5];
int ModePageSelect = 4;

void Press(int i);
void Release(int i);
void WaitForRelease(int i);
void Press0(){Press(0);}
void Press1(){Press(1);}
void Press2(){Press(2);}
void Release0(){Release(0);}
void Release1(){Release(1);}
void Release2(){Release(2);} 
void WaitForRelease0(){WaitForRelease(0);}
void WaitForRelease1(){WaitForRelease(1);}
void WaitForRelease2(){WaitForRelease(2);} 
void DoNothing(){};
TaskHandle_t ButtonTask0;
TaskHandle_t ButtonTask1;
TaskHandle_t ButtonTask2;



static void PressDueCount(void * pvParameters)
{
  for(;;){
    int i = (int)pvParameters;
    vTaskDelay(100);
    ButtonState[i]= true;
    vTaskDelay(1500);
    switch (i){
      case 0 : attachInterrupt(digitalPinToInterrupt(ButtonPin[0]), WaitForRelease0, RISING);break;
      case 1 : attachInterrupt(digitalPinToInterrupt(ButtonPin[1]), WaitForRelease1, RISING);break;
      case 2 : attachInterrupt(digitalPinToInterrupt(ButtonPin[2]), WaitForRelease2, RISING);break;
    }
    LongPress[i] = true;
    ButtonState[i]= false;
    //Serial.print("Button ");Serial.print(i);Serial.print(" Long Press");
    vTaskDelete(NULL);
  }
}

void WaitForRelease(int i)
{
  PressReleaseTimeStemp[i]= millis();
  //Serial.println(" --> Release");
  switch (i){
    case 0 : attachInterrupt(digitalPinToInterrupt(ButtonPin[0]), Press0, FALLING);break;
    case 1 : attachInterrupt(digitalPinToInterrupt(ButtonPin[1]), Press1, FALLING);break;
    case 2 : attachInterrupt(digitalPinToInterrupt(ButtonPin[2]), Press2, FALLING);break;
  }
}

void Button_Initialize()
{
  for (int i = 0; i < sizeof(ButtonPin); i++){pinMode(ButtonPin[i],INPUT);}
  while(digitalRead(ButtonPin[1]) == 0){delay(10);};
  attachInterrupt(digitalPinToInterrupt(ButtonPin[0]), Press0, FALLING);
  attachInterrupt(digitalPinToInterrupt(ButtonPin[1]), Press1, FALLING);
  attachInterrupt(digitalPinToInterrupt(ButtonPin[2]), Press2, FALLING);
}

void Press(int i)
{
  if(millis()-PressReleaseTimeStemp[i] > 400){
    switch (i){
      case 0 : 
        xTaskCreatePinnedToCore(PressDueCount,"PressDueCounter",1000 , (void*) 0, 5 , &ButtonTask0,1);
        attachInterrupt(digitalPinToInterrupt(ButtonPin[0]), Release0, RISING);
        break;
      case 1 : 
        xTaskCreatePinnedToCore(PressDueCount,"PressDueCounter",1000 , (void*) 1, 5 , &ButtonTask1,1);
        attachInterrupt(digitalPinToInterrupt(ButtonPin[1]), Release1, RISING);
        break;
      case 2 : 
        xTaskCreatePinnedToCore(PressDueCount,"PressDueCounter",1000 , (void*) 2, 5 , &ButtonTask2,1);
        attachInterrupt(digitalPinToInterrupt(ButtonPin[2]), Release2, RISING);
        break;
    }
  }
}

void Release(int i)
{
  attachInterrupt(digitalPinToInterrupt(ButtonPin[i]), DoNothing, CHANGE);
  //Serial.print("Button ");Serial.print(i);Serial.println(" Short Press");
  ShortPress[i] = true;
  PressReleaseTimeStemp[i]= millis();
  switch (i){
    case 0 : 
      vTaskDelete(ButtonTask0);
      attachInterrupt(digitalPinToInterrupt(ButtonPin[0]), Press0, FALLING);
      break;
    case 1 : 
      vTaskDelete(ButtonTask1);
      attachInterrupt(digitalPinToInterrupt(ButtonPin[1]), Press1, FALLING);
      break;
    case 2 : 
      vTaskDelete(ButtonTask2);
      attachInterrupt(digitalPinToInterrupt(ButtonPin[2]), Press2, FALLING);
      break;
  }
  ButtonState[i]= false;
}

//Button Press Result-----------------------------------------------------------------------------------



void ButtonPress()
{
  // Don't put any time consuming function here !!!
  switch (Mode){
    case 0: 
      if (ShortPress[0]){ModePageSelect = (ModePageSelect + 4) % 5; ModeChoose = &ModePage[ModePageSelect+1];}
      if (ShortPress[2]){ModePageSelect = (ModePageSelect + 1) % 5; ModeChoose = &ModePage[ModePageSelect+1];}
      if (ShortPress[1]){
        if (true/*ModeChoose != 5*/){Mode = *ModeChoose;}
        if (*ModeChoose == 3){Clock.ResetUserSetTimeBuffer();}
        if (*ModeChoose == 7){isInPage = true;isAutoAdd = false;}
        if (*ModeChoose == 6){control.ClearSet();}
      }
      if (LongPress[2])
      {
        cli();
        oled.PowerSave(1);
        for(int i=0; i < 3; i++)
        {
          detachInterrupt(digitalPinToInterrupt(ButtonPin[i]));
        }
        while(digitalRead(0)==0)
        digitalWrite(SWPin,LOW);
        esp_deep_sleep_start();
      }
      if(ButtonState[2])
      {
        digitalWrite(40,LOW);
        digitalWrite(41,LOW);
        digitalWrite(42,LOW);
      }
      if(ShortPress[2])
      {
        digitalWrite(40,HIGH);
        digitalWrite(41,HIGH);
        digitalWrite(42,HIGH);
      }
      break;
    case 1: //IMU
      if (ShortPress[0]){imu.PointTo(0,NodeNumber[0]);}
      if (ShortPress[1]){imu.PointTo(1,NodeNumber[0]);}
      if (ShortPress[2]){imu.PointTo(2,NodeNumber[0]);}
      if  (LongPress[0]){Mode = 0;imu.IMUPointTo =0;}
      if  (LongPress[1]){imu.Average();}
      if  (LongPress[2]){Mode = 0;imu.IMUPointTo =0;}
      imu.PointTo(-1,NodeNumber[0]);
      oled.State[0] = imu.IMUPointTo;
      break;
    case 3: // RTC
      if (ShortPress[0]){Clock.UserSetTime(0);}
      if (ShortPress[1]){
        Clock.UserSetTime(1);
        if(Clock.Cursor == 0){SendTime();}
      }
      if (ShortPress[2]){Clock.UserSetTime(2);}
      oled.State[2] = Clock.Cursor;
      if (LongPress[1]){Mode = 0; oled.State[2] = 0;}
      if (LongPress[2]){Mode = 0; oled.State[2] = 0;}
      if (LongPress[0]){Mode = 0; oled.State[2] = 0;}
      break;
    case 4: // SD
      if (ShortPress[0]){}
      if (ShortPress[1]){}
      if (ShortPress[2]){Mode = 0;}
      if (LongPress[0]){
        if(!SendCommand)
        {
          sdCard.Swich();
        }
        oled.State[3] = sdCard.CheckCount;
      }
      if (LongPress[1]){
        if(sdCard.CheckCount ==0){
          sdCard.Initialize = false;
          sdCard.AvoidWDTimeOutCount = 0;
          sdCard.Show[2] = ": ......";
          sdCard.path = "";
        }else{
          msg.Clear();
        }
      }
      if (LongPress[2]){Mode = 0;}
      break;
    case 6: // Command
    {
      bool SendCommand_ori = SendCommand;
      if (SendCommand){
        if (ShortPress[0]){SendCommand = false;}
        if (ShortPress[1]){SendCommand = false;}
        if (ShortPress[2]){Mode = 0;}
        if (LongPress[1]){SendCommand = false;}
        if (LongPress[2]){Mode = 0;}
        if(!SendCommand){control.Stop();BLE_Send(0);}
      }else{
        if (ShortPress[0]){control.UserSet(0);}
        if (ShortPress[1]){control.UserSet(1);}
        if (ShortPress[2]){control.UserSet(2);}
        if (LongPress[0]){
          if(NodeNumber[0] != 0){
            control.Start(SendCommand,imu.LocalAngleShift(),&sdCard);
          }
        }
        if (LongPress[1]){control.Set();}
        if (LongPress[2]){Mode = 0;control.ClearSet();}
      }
      if(SendCommand){
        oled.State[5] = -1;
      }else{
        oled.State[5] = control.Cursor;
      }
      if(SendCommand_ori!=SendCommand)
      {
        Debug.println((SendCommand)?"BLE Send Command start : Manual.":"BLE Send Command stop : Manual.");
        digitalWrite(42,(SendCommand)?LOW:HIGH);
      }
      break;
    }
    case 7: //BLE
      if (!SendCommand && !isAutoAdd && NodeNumber[0] + NodeNumber[1] + NodeNumber[2] > 0){
        if (ShortPress[0]){SelectNode(0);}
        if (ShortPress[1]){SelectNode(1);}
        if (ShortPress[2]){SelectNode(2);}
        SelectNode(-1);// Adjust cursor
        oled.State[6] = BLEPointTo;
      }else if(isAutoAdd || SendCommand){
        oled.State[6] = -1;
        if (ShortPress[0]){oled.DisplayStartFrom --;}
        if (ShortPress[1]){isAutoAdd = false;}
        if (ShortPress[2]){oled.DisplayStartFrom ++;}
      }
      if (LongPress[1] && !SendCommand){isAutoAdd = !isAutoAdd;}
      if (LongPress[2]){Mode = 0;isInPage = false;isAutoAdd = false;}
      
      break;
  }
  ShortPress[0] = false;
  ShortPress[1] = false;
  ShortPress[2] = false;
  LongPress[0] = false;
  LongPress[1] = false;
  LongPress[2] = false;
}

#endif
