#ifndef Net_h
#define Net_h

#include <Arduino.h>
#include <NimBLEDevice.h>

/* Note
  NimBLEDevice.h save around 600 kB memory and 100kB heap (1/3 heap of esp32) from Arduino built in BLEDevice.h
  ****************** Important***********************
  Remember to change the BLE Num
  #define CONFIG_BTDM_CONTROLLER_BLE_MAX_CONN 9
  #define CONFIG_BTDM_CONTROLLER_BLE_MAX_CONN_EFF 9
  ***************************************************
  1.Main Sensor -> Client / Motor Driver -> Server
    Client send message to Server : use BLERemoteCharacteristic->writeValue(), can return bool(failed/success).
    Server sent message to Client : use BLECharacteristic->notify(true), no return(failed/success).
    Client read message from Server : use BLERemoteCharacteristic->readValue(). Reading can be detect by server, but hard to make client read deta simultaneously.
  2.Main Sensor -> Client / SubSensor -> Server
    Save Motor Driver and subsensor Service ID in same list will be much simple.
    ESP only allow 9 device connection.
  3.SubSensor -> Server / Motor Driver -> Client
    Subsensor always work as server.
*/

bool UserCommand = false;

BLEScan *pBLEScan;
String Characteristic[9] = {"Roll", "Pitch", "Yaw", "Serial", "Command", "TimeStep", "TimeSet", "HaveSub", "WallAng"};

BLEUUID ServersUUID("2222eeee-00f2-0123-4567-abcdef123456");
BLEUUID ServiceUUID("1111eeee-00f4-0123-4567-abcdef123456");
BLEUUID RollAngUUID("a001eeee-00f2-0123-4567-abcdef123456");
BLEUUID PitcAngUUID("a002eeee-00f2-0123-4567-abcdef123456");
BLEUUID YawwAngUUID("a003eeee-00f2-0123-4567-abcdef123456");
BLEUUID NodeNumUUID("0000eeee-00f2-0123-4567-abcdef123456");
BLEUUID CommandUUID("c000eeee-00f4-0123-4567-abcdef123456");
BLEUUID TimeStpUUID("c001eeee-00f4-0123-4567-abcdef123456");
BLEUUID TimeSetUUID("c002eeee-00f4-0123-4567-abcdef123456");
BLEUUID HaveSubUUID("d155eeee-00f4-0123-4567-abcdef123456");
BLEUUID WallAngUUID("a004eeee-00f4-0123-4567-abcdef123456");

char DeviceID;
const int MaxServiceNum = 10;
TaskHandle_t InitScan;
TaskHandle_t WaitForConnect[MaxServiceNum];
int BLEPointTo = 1;
int DroppedID = 0;
int NodeNumber[3] = {0, 0, 0};    // Connect - Wait For Connect - Scanned
int DeviceType[3][MaxServiceNum]; // -1 : No Device, 0 : MD only, 1 : MD + Sub, 2: Sub Only
bool isInPage = false;
bool isAutoAdd = true;
String DropAddress;
String Address[3][MaxServiceNum]; // Connect - Wait For Connect - Scanned
BLEUUID ThisUUID;
BLEClient *TempCliecnt;
BLEClient *ConnectClient[MaxServiceNum];
BLERemoteService *RequestService;
BLERemoteCharacteristic *RequestCharacteristic[9];
class MyClientCallback;
class BankClientCallback;
class pBLE;
double Euler_Children[MaxServiceNum][3];

static void BLEInitScanClock(void *pvParameter)
{
  for (;;)
  {
    vTaskDelay(3 * 60 * 1000);
    isAutoAdd = false;
    Debug.println("BLE Initialize Scan Done");
    // SendCommand = true;//!!! for Testing stability only,remeber to delete it in final version !!!
    vTaskDelete(InitScan);
  }
}

void Net_Initialize()
{
  BLEDevice::init("Client");
  char HexID = BLEDevice::getAddress().toString().back();
  int IntID = (HexID >= 'a') ? (HexID - 'a' + 10) : (HexID - '0');
  DeviceID = "ABCDEFHKLPRSTUXZ"[IntID]; // Can replace dirty looking char by Z
  pBLEScan = BLEDevice::getScan();
  // https://esp32.com/viewtopic.php?t=2291
  // deal with the scan result after the scanning procedure, thus use interval = window
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(100);
  xTaskCreatePinnedToCore(BLEInitScanClock, "BLE Init Scan Clock", 2048, NULL, 1, &InitScan, 0);
  memset(DeviceType, -1, sizeof(DeviceType));
}

void BLE_Send_Wall_Angle(float Angle) // Send the command to Servr
{
  for (int i = 0; i < NodeNumber[0]; i++)
  {
    if (DeviceType[0][i] != 2)
    {
      bool SendSuccess = ConnectClient[i]->getService(ServiceUUID)->getCharacteristic(WallAngUUID)->writeValue(Angle);
      // Serial.print(millis());Serial.print(" Send ");Serial.println(command_dtheta,6);
      if (!SendSuccess)
      {
        ConnectClient[i]->disconnect();
        return;
      }
    }
  }
  // Serial.print(millis());Serial.print("-->");Serial.println(String(command_dtheta,6));
}

void BLE_Send(float command_dtheta) // Send the command to Servr
{
  for (int i = 0; i < NodeNumber[0]; i++)
  {
    if (DeviceType[0][i] == 0)
    {
      command_dtheta = 0;
      control.Stop();
      break;
    }
  }

  for (int i = 0; i < NodeNumber[0]; i++)
  {
    if (DeviceType[0][i] != 2)
    {
      bool SendSuccess = ConnectClient[i]->getService(ServiceUUID)->getCharacteristic(CommandUUID)->writeValue(command_dtheta);
      // Serial.print(millis());Serial.print(" Send ");Serial.println(command_dtheta,6);
      if (!SendSuccess)
      {
        ConnectClient[i]->disconnect();
        command_dtheta = 0;
        for (int j = 0; j < NodeNumber[0]; j++)
        {
          if (DeviceType[0][j] != 2)
          {
            ConnectClient[i]->getService(ServiceUUID)->getCharacteristic(CommandUUID)->writeValue(0);
          }
        }
        return;
      }
    }
  }
  // Serial.print(millis());Serial.print("-->");Serial.println(String(command_dtheta,6));
}

void ShiftNodeList(String Mode, int i)
{
  if (Mode == "Connect")
  {
    // Shifting the connect node list.
    memmove(&Euler_Children[i][0], &Euler_Children[i + 1][0], sizeof(Euler_Children[i][0]) * (NodeNumber[0] - i + 1) * 3);
    memmove(&imu.Count[i + 1], &imu.Count[i + 2], sizeof(imu.Count[i + 1]) * (NodeNumber[0] - i + 1));
    memmove(&ConnectClient[i], &ConnectClient[i + 1], sizeof(ConnectClient[i]) * (NodeNumber[0] - i + 1));
    memmove(&DeviceType[0][i], &DeviceType[0][i + 1], sizeof(DeviceType[0][i]) * (NodeNumber[0] - i + 1));
    memmove(&Address[0][i], &Address[0][i + 1], sizeof(Address[0][i]) * (NodeNumber[0] - i + 1));
    NodeNumber[0]--;
    for (int j = i; j < NodeNumber[0]; j++)
    {
      if (DeviceType[0][j] == 2)
      {
        ConnectClient[j]->getService(ServersUUID)->getCharacteristic(NodeNumUUID)->writeValue(DeviceID + String(j + 1));
      }
      else
      {
        ConnectClient[j]->getService(ServiceUUID)->getCharacteristic(NodeNumUUID)->writeValue(DeviceID + String(j + 1));
      }
    }
    DeviceType[0][NodeNumber[0]] = -1;
  }
  else if (Mode.startsWith("S"))
  {
    // Shifting the Scanned node list
    memmove(&DeviceType[2][i], &DeviceType[2][i + 1], sizeof(DeviceType[2][i]) * (NodeNumber[2] - i + 1));
    memmove(&Address[2][i], &Address[2][i + 1], sizeof(Address[2][i]) * (NodeNumber[2] - i + 1));
    NodeNumber[2]--;
  }
  else if (Mode.startsWith("W") && Address[1][i] != "")
  {
    Address[1][i] = "";
    NodeNumber[1]--;
    DeviceType[1][i] = -1;
    vTaskDelete(WaitForConnect[i]);
  }
}

static void BLEReConScanClock(void *pvParameter)
{ // Trigger When disconnect, start counting for specific time.
  // If can't find the device in time, stop scanning.
  int ThisID = (int)pvParameter;
  for (;;)
  {
    vTaskDelay(30000);
    if (Address[1][ThisID] != "")
    {
      if(SendCommand)
      {
        SendCommand = false;
        control.Stop();
        Debug.print("BLE Send Command Stop : ");
        digitalWrite(42,HIGH);
      }
      oled.State[5] = control.Cursor;
      Debug.print(Address[1][ThisID]);
      Debug.println(" connect time out.");
      ShiftNodeList("Wait",ThisID);
    }
    vTaskDelete(NULL);
  }
}

void WaitForConnectTo(String WaitForAddress, int ThisDeviceType) // add new address to waiting list
{
  while (NodeNumber[1] < MaxServiceNum && Address[1][DroppedID] != "")
  {
    DroppedID = (DroppedID + 1) % MaxServiceNum;
  }
  Address[1][DroppedID] = WaitForAddress;
  DeviceType[1][DroppedID] = ThisDeviceType;
  xTaskCreatePinnedToCore(BLEReConScanClock, "BLE Wait For Connect Clock", 2048, (void *)DroppedID, 1, &WaitForConnect[DroppedID], 0);
  NodeNumber[1]++;
  DroppedID = (DroppedID + 1) % MaxServiceNum;
}

void SendTime()
{
  for (int i = 0; i < NodeNumber[0]; i++)
  {
    if (DeviceType[0][i] != 2)
    {
      ConnectClient[i]->getService(ServiceUUID)->getCharacteristic(TimeSetUUID)->writeValue(Clock.TimeStamp());
    }
  }
}

class MyClientCallback : public BLEClientCallbacks // Call back when dropped connection
{
  void onConnect(BLEClient *ThisClient) {}
  void onDisconnect(BLEClient *ThisClient)
  {
    DropAddress = ThisClient->getPeerAddress().toString().c_str();
    int i = 0;
    while (i < NodeNumber[0] && Address[0][i] != DropAddress)
    {
      i++;
    }

    if (i == NodeNumber[0])
    {
      Debug.print("Unknown dropped address ");
      Debug.println(DropAddress);
    }
    else
    {
      WaitForConnectTo(DropAddress, DeviceType[0][i]);
      ShiftNodeList("Connect", i);
      BLE_Send(0);
      Debug.print("Dropped Connection : ");
      Debug.println(DropAddress);
    }
  }
};

class BlankClientCallback : public BLEClientCallbacks // Used when User Command Disconnect
{
  void onConnect(BLEClient *ThisClient) { ; }
  void onDisconnect(BLEClient *ThisClient)
  {
    Debug.print("Disconnect to Device : ");
    Debug.println(ThisClient->getPeerAddress().toString().c_str());
  }
};

void AngleNotifyCB(BLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
  // Call when function Characteristic.notify(true) is call by the server.
  // Read and update the value.
  // can't use pRemoteCharacteristic->readValue() in Notify CallBack, used pData instead.
  String NotifyAddress = pRemoteCharacteristic->getRemoteService()->getClient()->getPeerAddress().toString().c_str();
  int i = 0;
  while (i < NodeNumber[0] && Address[0][i] != NotifyAddress)
  {
    i++;
  }

  if (i == NodeNumber[0])
  {
    Debug.print("Unknown angle notify from address ");
    Debug.println(NotifyAddress);
  }
  else if (pRemoteCharacteristic->getUUID() == RollAngUUID)
  {
    Euler_Children[i][0] = *(float *)pData;
  }
  else if (pRemoteCharacteristic->getUUID() == PitcAngUUID)
  {
    Euler_Children[i][1] = *(float *)pData;
  }
  else if (pRemoteCharacteristic->getUUID() == YawwAngUUID)
  {
    Euler_Children[i][2] = *(float *)pData;
  }
}

void DisconnectNotifyCB(BLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
  if (SendCommand)
  {
    SendCommand = false;
    BLE_Send(0);
    control.Stop();
    Debug.print("BLE Send Command Stop : ");
    digitalWrite(42,HIGH);
  }
  String NotifyAddress = pRemoteCharacteristic->getRemoteService()->getClient()->getPeerAddress().toString().c_str();

  int i = 0;
  while (i < NodeNumber[0] && Address[0][i] != NotifyAddress)
  {
    i++;
  }

  if (i == NodeNumber[0])
  {
    Debug.print("Unknown disconnect notify from address ");
    Debug.println(NotifyAddress);
  }
  else
  {
    pRemoteCharacteristic->getRemoteService()->getClient()->setClientCallbacks(new BlankClientCallback());
    pRemoteCharacteristic->getRemoteService()->getClient()->disconnect();
    ShiftNodeList("Connect", i);
    Debug.print("Disconnect request from address ");
    Debug.println(NotifyAddress);
  }
}

void StopCommandNotifyCB(BLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
  if (SendCommand)
  {
    SendCommand = false;
    BLE_Send(0);
    control.Stop();
    Debug.print("BLE Send Command Stop : ");
    digitalWrite(42,HIGH);
  }
  String NotifyAddress = pRemoteCharacteristic->getRemoteService()->getClient()->getPeerAddress().toString().c_str();
  Debug.print("Stop command notify from address ");
  Debug.println(NotifyAddress);
}

void MDrWithSubNotifyCB(BLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
  String NotifyAddress = pRemoteCharacteristic->getRemoteService()->getClient()->getPeerAddress().toString().c_str();
  int i = 0;
  while (i < NodeNumber[0] && Address[0][i] != NotifyAddress)
  {
    i++;
  }

  if (i == NodeNumber[0])
  {
    Debug.print("Unknown sensor connect change notify from address ");
    Debug.println(NotifyAddress);
  }
  else
  {
    DeviceType[0][i] = *(int *)pData;
    Debug.print(NotifyAddress);
    Debug.print(" Subsensor connect : ");
    Debug.println(String(DeviceType[0][i]));
  }
}

bool ConnectToDevice(BLEAddress RequestAddress, bool isSubSensor) // Check UUID and connect
{
  Debug.print((isSubSensor) ? "Find Free Subsensor : " : "Address Found : ");
  Debug.print(RequestAddress.toString().c_str());
  // Serial.println("");
  // The procedure include some delay in the library and may be inturrupted by other task in core 0.
  // The task in core 0 need to be short enough to avoided watchdog starve.
  Debug.print(" - Start Connection ");

  // Serial.println("");Serial.print("  - Creating Client ...");
  TempCliecnt = BLEDevice::createClient();

  // Serial.println("");Serial.print("  - Connecting to Address ...");
  if (!TempCliecnt->connect(RequestAddress))
  {
    Debug.print("-> Failed to Connnect to the Address ");
    Debug.print(RequestAddress.toString().c_str());
    Debug.println("");
    return false;
  }

  // Serial.println("");Serial.print("  - Finding Service ...");
  RequestService = (isSubSensor) ? (TempCliecnt->getService(ServersUUID)) : (TempCliecnt->getService(ServiceUUID));

  if (RequestService == nullptr)
  {
    Debug.println("-> Can't find service UUID");
    TempCliecnt->disconnect();
    return false;
  }

  // Serial.println("");Serial.print("  - Finding Request Characterisic ...");
  RequestCharacteristic[0] = RequestService->getCharacteristic(RollAngUUID); // Notify
  RequestCharacteristic[1] = RequestService->getCharacteristic(PitcAngUUID); // Notify
  RequestCharacteristic[2] = RequestService->getCharacteristic(YawwAngUUID); // Notify
  RequestCharacteristic[3] = RequestService->getCharacteristic(NodeNumUUID); // Notify + Write
  if (!isSubSensor)
  {
    RequestCharacteristic[4] = RequestService->getCharacteristic(CommandUUID); // Notify + Write
    RequestCharacteristic[5] = RequestService->getCharacteristic(TimeStpUUID); // Write
    RequestCharacteristic[6] = RequestService->getCharacteristic(TimeSetUUID); // Write
    RequestCharacteristic[7] = RequestService->getCharacteristic(HaveSubUUID); // Notify
    RequestCharacteristic[8] = RequestService->getCharacteristic(WallAngUUID); // Write
  }
  int CharacteristicNum = (isSubSensor) ? 4 : 9;
  bool isRequestNotify[9] = {true, true, true, true, true, false, false, true, false};
  bool isRequestWrites[9] = {false, false, false, true, true, true, true, false, true};

  for (int i = 0; i < CharacteristicNum; i++)
  {
    if (RequestCharacteristic[i] == nullptr)
    {
      Debug.println("-> Can't find characteristic " + Characteristic[i]);
      TempCliecnt->disconnect();
      return false;
    }
    if (isRequestNotify[i] && !RequestCharacteristic[i]->canNotify())
    {
      Debug.println("-> Notify " + Characteristic[i] + " Failed");
      TempCliecnt->disconnect();
      return false;
    }
    if (isRequestWrites[i] && !RequestCharacteristic[i]->canWrite())
    {

      Debug.println("-> Write " + Characteristic[i] + " Failed");
      TempCliecnt->disconnect();
      return false;
    }
  }

  int i = 0;
  while (i < NodeNumber[2] && Address[2][i] != RequestAddress.toString().c_str())
  {
    i++;
  }
  if (i != NodeNumber[2])
  {
    ShiftNodeList("Scanned", i);
  }

  int j = 0;
  while (j < MaxServiceNum && Address[1][j] != RequestAddress.toString().c_str())
  {
    j++;
  }
  if (j != MaxServiceNum)
  {
    ShiftNodeList("Wait", j);
  }

  TempCliecnt->setClientCallbacks(new MyClientCallback());
  ConnectClient[NodeNumber[0]] = TempCliecnt;
  Address[0][NodeNumber[0]] = RequestAddress.toString().c_str();
  DeviceType[0][NodeNumber[0]] = (isSubSensor) ? (2) : (RequestCharacteristic[7]->readValue<int>());
  RequestCharacteristic[3]->writeValue(DeviceID + String(NodeNumber[0] + 1));
  String DeviceTypeShow[3] = {" (Motor Driver without Subsensor)", " (Motor Driver)", " (Subsensor)"};
  Debug.print("--> Connect Success. ");
  Debug.println(DeviceTypeShow[DeviceType[0][NodeNumber[0]]]);
  NodeNumber[0]++;

  RequestCharacteristic[0]->subscribe(true, AngleNotifyCB);
  RequestCharacteristic[1]->subscribe(true, AngleNotifyCB);
  RequestCharacteristic[2]->subscribe(true, AngleNotifyCB);
  RequestCharacteristic[3]->subscribe(true, DisconnectNotifyCB);
  if (!isSubSensor)
  {
    RequestCharacteristic[4]->subscribe(true, StopCommandNotifyCB);
    RequestCharacteristic[5]->writeValue(control.dt_ms);
    RequestCharacteristic[6]->writeValue(Clock.TimeStamp());
    RequestCharacteristic[7]->subscribe(true, MDrWithSubNotifyCB);
    RequestCharacteristic[8]->writeValue(imu.LocalAngleShift());
  }
  return true;
}

bool isNewAddress(String &ThisAddress)
{
  for (int j = 0; j < NodeNumber[0]; j++)
  {
    if (ThisAddress == Address[0][j])
    {
      return false;
    }
  }
  return true;
}

void ScanModeCheck(BLEAdvertisedDevice &ThisDevice, bool isSubSensor, String (&NewAddress)[10], int (&NewAddressType)[10], int &NewAddressNum)
{
  String ThisAddress = ThisDevice.getAddress().toString().c_str();

  // Check if Already Connect
  if (!isNewAddress(ThisAddress))
  {
    return;
  }

  // if the Address is in the wait to connect list.
  int j = 0;
  while (j < MaxServiceNum && Address[1][j] != ThisAddress)
  {
    j++;
  }
  if (j != MaxServiceNum)
  {
    ConnectToDevice(ThisDevice.getAddress(), isSubSensor);
    return;
  }

  // if auto add on
  if (isAutoAdd)
  {
    ConnectToDevice(ThisDevice.getAddress(), isSubSensor);
  }
  else if (isInPage && NewAddressNum < MaxServiceNum)
  {
    NewAddress[NewAddressNum] = ThisAddress;
    NewAddressType[NewAddressNum] = isSubSensor + 1;
    NewAddressNum++;
  }
  Debug.print((isSubSensor) ? "Find Free Subsensor : " : "Address Found : ");
  Debug.println(ThisAddress);
}

void BLE_Scan()
{
  // Check if need to scan or not
  if ((SendCommand || (!isAutoAdd && !isInPage)) && (NodeNumber[1] == 0))
  {
    return;
  }
  digitalWrite(40, LOW);

  // Scan -Since we are using task, the Scan duration need to be short to avioded watchdog trigger.
  pBLEScan->start(1, false);

  // Renew the Scan Result when in Page

  String NewAddress[10];
  int NewAddressType[10];
  int NewAddressNum = 0;
  int NewFreeSub[10];
  int NewFreeSubNum = 0;
  int i = 0;
  while (i < pBLEScan->getResults().getCount() && (MaxServiceNum - 1 - NodeNumber[0]) > 0)
  {
    BLEAdvertisedDevice ThisDevice = pBLEScan->getResults().getDevice(i);
    // Check Service UUID
    if (ThisDevice.haveServiceUUID() && ThisDevice.isAdvertisingService(ServiceUUID))
    {
      ScanModeCheck(ThisDevice, false, NewAddress, NewAddressType, NewAddressNum);
    }
    else if (ThisDevice.haveServiceUUID() && ThisDevice.isAdvertisingService(ServersUUID))
    {
      String ThisAddress = ThisDevice.getAddress().toString().c_str();
      // Check if Already Connect
      if (isNewAddress(ThisAddress))
      {
        NewFreeSub[NewFreeSubNum] = i;
        NewFreeSubNum++;
      }
    }
    i++;
  } // End Device Check

  int MDNoSub = 0;
  if (isAutoAdd)
  {
    // Auto Disconnect to Subsensor if any Motor Driver didn't have Subsensor.
    for (int i = 0; i < NodeNumber[0]; i++)
    {
      MDNoSub += (DeviceType[0][i] == 0);
    }
    int i = NodeNumber[0] - 1;
    while (i >= 0 && MDNoSub > NewFreeSubNum)
    {
      if (DeviceType[0][i] == 2)
      {
        ConnectClient[i]->setClientCallbacks(new BlankClientCallback());
        ConnectClient[i]->disconnect();
        ShiftNodeList("Connect", i);
        NewFreeSubNum++;
      }
      i--;
    }
  }
  i = 0;
  while (i < NewFreeSubNum - MDNoSub && (MaxServiceNum - 1 - NodeNumber[0]) > 0)
  {
    BLEAdvertisedDevice ThisDevice = pBLEScan->getResults().getDevice(NewFreeSub[i]);
    ScanModeCheck(ThisDevice, true, NewAddress, NewAddressType, NewAddressNum);
    i++;
  }
  pBLEScan->clearResults();

  NodeNumber[2] = 0;
  if (isInPage)
  {
    i = 0;
    while (NodeNumber[2] < MaxServiceNum - 1 && i < NewAddressNum)
    {
      // Check if Address is already connect or waiting to be connect
      int j = 0;
      while (j < MaxServiceNum * 2 && NewAddress[i] != *(&Address[0][0] + j))
      {
        j++;
      }
      if (j == MaxServiceNum * 2)
      {
        Address[2][NodeNumber[2]] = NewAddress[i];
        DeviceType[2][NodeNumber[2]] = NewAddressType[i];
        NodeNumber[2]++;
      }
      i++;
    }
    for (int i = NodeNumber[2]; i < MaxServiceNum; i++)
    {
      Address[2][i] = "";
      DeviceType[2][i] = -1;
    }
  }
  digitalWrite(40, HIGH);
  // Serial.printf("Free heap (Net.h 213) : %d \n", ESP.getFreeHeap());
}

void SelectNode(int Do)
{
  switch (Do)
  {
  case -1:
    BLEPointTo = (BLEPointTo <= 0) ? 1 : min(BLEPointTo, NodeNumber[0] + NodeNumber[1] + NodeNumber[2]);
    break;
  case 0:
    BLEPointTo--;
    break;
  case 1:
    // Make sure that no other function is editing the list.
    if (BLEPointTo <= NodeNumber[0])
    {
      ConnectClient[BLEPointTo - 1]->setClientCallbacks(new BlankClientCallback());
      ConnectClient[BLEPointTo - 1]->disconnect();
      if (NodeNumber[2] < MaxServiceNum - 1)
      {
        Address[2][NodeNumber[2]] = Address[0][BLEPointTo - 1];
        DeviceType[2][NodeNumber[2]] = DeviceType[0][BLEPointTo - 1];
        NodeNumber[2]++;
      }
      ShiftNodeList("Connect", BLEPointTo - 1);
    }
    else if (BLEPointTo > NodeNumber[0] + NodeNumber[1])
    {
      if ((NodeNumber[1] < MaxServiceNum))
      {
        WaitForConnectTo(Address[2][BLEPointTo - NodeNumber[0] - NodeNumber[1] - 1], DeviceType[2][BLEPointTo - NodeNumber[0] - NodeNumber[1] - 1]);
        ShiftNodeList("Scanned", BLEPointTo - NodeNumber[0] - NodeNumber[1]);
      }
    }
    else
    {
      int Count = 0;
      int i = -1;
      while (Count < BLEPointTo - NodeNumber[0])
      {
        i++;
        Count += (Address[1][i] != "");
      }
      if (NodeNumber[2] < MaxServiceNum - 1)
      {
        Address[2][NodeNumber[2]] = Address[1][i];
        DeviceType[2][NodeNumber[2]] = DeviceType[1][i];
        NodeNumber[2]++;
      }
      ShiftNodeList("Wait",i);
    }
    break;
  case 2:
    BLEPointTo++;
    break;
  }
}

#endif
