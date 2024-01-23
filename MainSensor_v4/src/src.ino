// IO-4  (Top LED G) : Blink on when Saving
// IO-16 (Top LED R) : Light on when SendCommand=true
// IO-40 (Mid LED B) : Light on when in scanning window (1s)
// IO-41 (Mid LED G) : Light on when IO-0 press for ext0 wakeup

TaskHandle_t Read;
TaskHandle_t Show;
TaskHandle_t Save;
TaskHandle_t Load;
TaskHandle_t Send;
TaskHandle_t Scan;
TaskHandle_t SEND;
TaskHandle_t Buff;
TaskHandle_t SleepClock;

#include "SerialDebug.h"
SerialDebug D;
extern SerialDebug Debug = D;

bool SendCommand = false;

#include "RealTimeClock.h"
#include "IMU.h"
#include "MsgBuffer.h"
#include "Control.h"

#include "SDCard.h"

#include "Battery.h"
#include "OLED.h"

Control control;
SDCard sdCard;
MsgBuffer msg;
IMU imu;
RealTimeClock Clock;
Battery battery;
OLED oled;

bool led = true;

#include "Net.h"
#include "OnOffControl.h"
#include "Button.h"

void setup()
{
  WakeUpDetect();

  imu.Initialize(&Euler_Children[0][0], &NodeNumber[0]);
  battery.SetPin(17);
  oled.Initialize();
  sdCard.Swich();
  oled.bat = &battery;

  /*  Note :
  *. All task are suggest to be as short/fast as possible. If can't, be aware of the priority and core choose

  1. Attachinturrupt (Button)    : setup in core 1, thus keep staying in core 1 for convenience. Highest priority -> Core 1, Priority 5
  2. Check Button Press result   : Don't want the button press bool change when running. Run before OLED update <-----------------------\
  3. OLED update                 : Need fast update rate (100/200/250/500) ----------------------------------------> Core 1, Priority 3--|
  4. RTC update                  : Required most often by OLED. Update before OLED update. <--------------------------------------------/
  5. LoRa/SD/BLE Scan and Connect: Don't want to be inturrupted or making OLED lagged -----------------------------> Core 0
  6. BLE Scan and Connect        : Library include delay(), used highest priority to avoid been blocked too long.--> Core 0, Priority 5
  7. BLE Send                    : Don't want to be lagged by LoRa/SD (BLE Send will block BLE Scan) --------------> Core 1, Priority 4
  8. HardwareSerial Read(IMU/GPS): Don't want to be lagged by LoRa/SD ---------------------------------------------> Core 1, Priority 2
  9. TingGPSPlus.encode          : Suggest not using delay --------------------------------------------------------> Core 1, Priority 1

  *. To avoided blocking BLE Scan for too long, LoRa and SD are suggest to be in two different task

  +  If SD and LoRa used same SPI, then they need to be put in the same task.
  +  When using painlneemesh, core 0 won't responce in core 1 (guess it is already used by library)
  +  When using painlessmesh library, the mesh.update only work in core 1.
  */
  xTaskCreatePinnedToCore(ScanBLEDCallback, "Scan", 20000, NULL, 4, &Scan, 0);
  xTaskCreatePinnedToCore(SaveDataCallback, "Save", 10000, NULL, 1, &Save, 0);
  xTaskCreatePinnedToCore(SendDataCallback, "Send", 8192, NULL, 5, &Send, 0);
  xTaskCreatePinnedToCore(ShowDataCallback, "Show", 4096, NULL, 3, &Show, 1);
  xTaskCreatePinnedToCore(ReadDataCallback, "Read", 4096, NULL, 2, &Read, 1);
  xTaskCreatePinnedToCore(SaveBuffCallback, "Buff", 4096, NULL, 2, &Buff, 1);

  Button_Initialize();
  // Start recording after everything is well set.
  msg.Clear();
  digitalWrite(40, HIGH);
  digitalWrite(41, HIGH);
  digitalWrite(42, HIGH);
}

void ReadDataCallback(void *pvParameters)
{
  for (;;)
  {
    // Euler_Childern_Update();
    imu.Read();
    imu.LocalAngleShift();
    vTaskDelay(25);
  }
}

void SaveBuffCallback(void *pvParameters)
{
  vTaskDelay(30 * 1000);
  for (;;)
  {
    msg.Add(Clock.DateTimeStamp() + "," + String((battery.Percent), 2), imu.EulerAvg, imu.EulerLocal, Address, Euler_Children, NodeNumber);
    vTaskDelay(msg.WriteWhen * 1000);
  }
}

void ShowDataCallback(void *pvParameters)
{
  vTaskDelay(3000); // For initial figure show
  for (;;)
  {
    Clock.update();
    ButtonPress();
    oled.Display(Mode, DeviceID, *ModeChoose, ButtonState,
                 NodeNumber, DeviceType,
                 control.UserSetting, control.CheckSetting(),
                 msg.Msg, msg.WriteWhen, Clock, Address, imu, sdCard);
    vTaskDelay(100); // suggest to be factor of 1000 for smooth clock display.
  }
}

void SaveDataCallback(void *pvParameters)
{
  vTaskDelay(5000);
  sdCard.SetPin();
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);
  pinMode(16, OUTPUT);
  digitalWrite(16, HIGH);
  for (;;)
  {
    // Save Data to SD card
    if (!sdCard.SwichMode)
    {
      msg.BufferCheck();
      digitalWrite(16, LOW);
      delay(500);
    }
    else
    {
      digitalWrite(4, LOW);
      if (sdCard.CheckCount != -1)
        digitalWrite(16, LOW);
      if (!sdCard.CheckState())
      {
        msg.BufferCheck();
      }
      else
      {
        digitalWrite(16, HIGH);
        if (!sdCard.Initialize)
        {
          sdCard.CreateFile(Clock.DateStamp("", 2));
        }
        if (sdCard.Initialize)
        {
          String MsgTest = msg.Msg;
          msg.Clear();
          sdCard.WriteFile(MsgTest);
        }
        else
        {
          msg.BufferCheck();
        }
      }
    }
    oled.State[3] = sdCard.CheckCount;
    digitalWrite(4, HIGH);
    digitalWrite(16, HIGH);
    // delay for 10000 Tick
    vTaskDelay(max({10000, msg.WriteWhen * 1000}));
  }
}

void ScanBLEDCallback(void *pvParameters)
{
  // BLE class item consume large amount of heap, thus create "!!!! ONE TASK ONLY !!!!"
  // and used other variable to control the scan function.
  Net_Initialize();
  for (;;)
  {
    vTaskDelay(5000);
    BLE_Scan();
    battery.Update();
  }
}

void SendDataCallback(void *pvParameters)
{
  for (;;)
  {
    if (NodeNumber[0] != 0)
    {
      float WallAngle = imu.LocalAngleShift();
      BLE_Send_Wall_Angle(WallAngle);
      if (SendCommand && NodeNumber[1] == 0)
      {
        control.Estimate(imu.LocalAngleShift());
        BLE_Send(control.V);
      }
    }
    vTaskDelay(control.dt_ms);
  }
}

void loop() {}
