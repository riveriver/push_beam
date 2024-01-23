// Add I2C Current Sensor
// Add I2C Voltage Sensor

const gpio_num_t IO_Extern_Wakeup = GPIO_NUM_0;
const byte IO_Button_SW = GPIO_NUM_0;
const byte IO_I2C_SCL = 9;
const byte IO_I2C_SDA = 8;
const byte IO_MD_V_FB = 7; //
const byte IO_MD_Dir = 6;  // OutPut, PH
const byte IO_MD_Brak = 5; // Output, LOW to wake up, HIGH to sleep.
const byte IO_MD_V = 16;   // Output, EN
const byte IO_MD_Swich = 18; // 
const byte IO_MD_EN_Dir = -1;// 
const byte IO_Battery_OCF = 17; // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
const byte IO_IMU_ISR = 20;
const byte IO_IMU_SCK = 21;
const byte IO_IMU_MISO = 10;
const byte IO_IMU_MOSI = 47;
const byte IO_IMU_CS = 48;
const byte IO_IMU_RX = 38;
const byte IO_IMU_TX = 39;
const byte IO_Button_CL = 40;
const byte IO_Button_DT = 41;
#define LED_PIN 42
#define OLED_RST 13
#define IMU_CS 48

// IMU with Serial0
#include "MotorDriver.h"
#include "SDCard.h"
#include "OLED.h"
#include "Clock.h"
#include "OnOff.h"
#include "IMU42688.h"

#include "SerialDebug.h"
SerialDebug Extern_Serial_Debug;
extern SerialDebug Debug = Extern_Serial_Debug;
RGBLED Extern_RGB_LED;
extern RGBLED LED = Extern_RGB_LED;

byte Page = 2;
byte Cursor = 0;

IMU42688 imu;
MotorDriver MD;
OLED oled;
Clock L_clock;
OnOff Swich;
SDCard sdCard;

#include "Net.h"
#include "Button.h"

TaskHandle_t T_LOOP;
TaskHandle_t T_FAST;
TaskHandle_t T_SLOW;
TaskHandle_t T_CONN;
TaskHandle_t T_SEND;
TaskHandle_t T_MDCON;
TaskHandle_t T_CHECK;
TaskHandle_t T_SAVE;
TaskHandle_t T_BACK;

String MD_Last_Recieve;

static void Conn(void *pvParameter)
{
  // Core 0
  Server_Initialize();
  Client_Initialize();
  for (;;)
  {
    Client_Connect();
    vTaskDelay(3000);
  }
}

static void Send(void *pvParameter)
{
  // Core 0
  vTaskDelay(10000);
  for (;;)
  {
    BLE_Manual_Command_Check();
    BLE_Send_Update_Angle();
    vTaskDelay(50);
  }
}

static void Check(void *pvParameter)
{
  // Core 0
  for (;;)
  {
    vTaskDelay(30 * 60 * 1000);
    Check_Server_Characteristic();
  }
}

static void MDCON(void *pvParameter)
{
  // Core 1
  TickType_t xLastWakeTime;
  BaseType_t xWasDelayed;
  xLastWakeTime = xTaskGetTickCount();
  for (;;)
  {
    if(MD.Check)
      xWasDelayed = xTaskDelayUntil(&xLastWakeTime, StepTime/2);
    else
      xWasDelayed = xTaskDelayUntil(&xLastWakeTime, 1000);
    if (!xWasDelayed)
      Debug.println("[Warning] Task MDCON Time Out.");
    imu.Update();
    MD.Check_Connect();
    MD.AccControl();
  }
}

static void Slow(void *pvParameter)
{
  // Core 1
  for (;;)
  {
    Swich.OffCheck();
    LED.Update();
    MD.ReadBattery();
    vTaskDelay(500);
  }
}

static void Loop(void *pvParameter)
{
  // Core 1
  delay(1000);
  for (;;)
  {
    Button_Update();
    MD_Last_Recieve = L_clock.toString(LastUpdate);
    oled.Update();
    vTaskDelay(250);
  }
}

static void Fast(void *pvParameter)
{
  // Core 1
  delay(1000);
  TickType_t xLastWakeTime;
  BaseType_t xWasDelayed;
  xLastWakeTime = xTaskGetTickCount();
  for (;;)
  {
    xWasDelayed = xTaskDelayUntil(&xLastWakeTime, 50);
    if (!xWasDelayed)
      Debug.println("[Warning] Task FAST Time Out.");
    MD.CurrentFB();
  }
}

static void Back(void *pvParameter)
{
  delay(1000);
  TickType_t xLastWakeTime;
  BaseType_t xWasDelayed;
  xLastWakeTime = xTaskGetTickCount();
  for (;;)
  {
    xWasDelayed = xTaskDelayUntil(&xLastWakeTime, 2);
    if (!xWasDelayed)
      Debug.println("[Warning] Task BACK Time Out.");
    imu.Update42688();
  }
}

void setup()
{
  Swich.On(GPIO_NUM_0);
  //Swich.pSD = &sdCard;
  Swich.pMD = &MD;
  //Debug.Setup(sdCard);
  Debug.printOnTop("-------------------------ESP_Start-------------------------");
  MD.Initialize(IO_MD_V, IO_MD_Dir, IO_MD_Brak, IO_MD_V_FB, IO_MD_Swich, IO_MD_EN_Dir);
  MD.MountedAngle = &imu.Angle[0];
  imu.Initialize(IO_IMU_SCK,IO_IMU_MISO,IO_IMU_MOSI,IO_IMU_CS);
  oled.pMD = &MD;
  oled.Battery = &MD.BatteryPercent;
  oled.isConnect = &isConnect;
  oled.isAdvertising = &keepAdvertising;
  oled.isScanning = &isScanning;
  oled.Address = &Address;
  oled.pMD_C_show = &MD_Last_Recieve;
  oled.Angle = &Angle;
  xTaskCreatePinnedToCore(MDCON, "MDCont", 10000, NULL, 4, &T_MDCON, 1);
  xTaskCreatePinnedToCore(Fast, "Fast", 10000, NULL, 3, &T_FAST, 1);
  xTaskCreatePinnedToCore(Loop, "Main", 10000, NULL, 2, &T_LOOP, 1);
  xTaskCreatePinnedToCore(Slow, "Slow", 10000, NULL, 1, &T_SLOW, 1);
  xTaskCreatePinnedToCore(Back, "BackGround", 10000, NULL, 5, &T_BACK, 1);
  xTaskCreatePinnedToCore(Conn, "Conn", 20000, NULL, 3, &T_CONN, 0);
  xTaskCreatePinnedToCore(Send, "Send", 10000, NULL, 2, &T_SEND, 0);
  xTaskCreatePinnedToCore(Check, "Check Char", 10000, NULL, 1, &T_CHECK, 0);

  Button_Iitialize();
}

void loop()
{
}