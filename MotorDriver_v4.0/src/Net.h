// Server : Save Data, Send Data to MainSensor(Client)
// Client : Scan and Access Data from subsensor(Server)
#ifndef Net_H
#define Net_H

#include <NimBLEDevice.h>
#include <Arduino.h>
#include "RGBLED.h"
#include "SerialDebug.h"

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

BLECharacteristic *RollAngChar;
BLECharacteristic *PitcAngChar;
BLECharacteristic *YawwAngChar;
BLECharacteristic *NodeNumChar;
BLECharacteristic *CommandChar;
BLECharacteristic *RemTimeChar;
BLECharacteristic *SetTimeChar;
BLECharacteristic *HaveSubChar;
BLECharacteristic *WallAngChar;

BLERemoteCharacteristic *SubRollAng;
BLERemoteCharacteristic *SubPitcAng;
BLERemoteCharacteristic *SubYawwAng;
BLERemoteCharacteristic *SubNodeNum;

BLE2904 *p2904CommOmega = new BLE2904();
BLE2904 *p2904AngleRoll = new BLE2904();
BLE2904 *p2904AnglePitc = new BLE2904();
BLE2904 *p2904AngleYaww = new BLE2904();
BLE2904 *p2904IntTimRem = new BLE2904();
BLE2904 *p2904IntHavSub = new BLE2904();
BLE2904 *p2904StrNodeID = new BLE2904();
BLE2904 *p2904StrSetTim = new BLE2904();
BLE2904 *p2904WallAngle = new BLE2904();

TaskHandle_t COMMAND_TIMER;
int StepTime = 200;
int StepTimeTH = 100;

String Address[3] = {"....", "...", "...."}; // { 0: Subsensor / 1 : MainSensor / 2 : Local}
bool isConnect[3] = {false, false, false};   // { 0:sub sensor connsct / 1:main sensor connect / 2:Recieving Command }
bool isAdvertising = false;
bool keepAdvertising = true;
bool isScanning = true;
int LastUpdate = -1;
float Angle[3] = {0.0, 0.0, 0.0};
BLEScan *pBLEScan;
BLEClient *ThisClient;

class MyServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer)
    {
        if (keepAdvertising)
        {
            isAdvertising = false;
            isConnect[1] = true;
            Debug.println("Server (Main Sensor) Connect");
            LED.Write(1, 0, 255, 0, LED.BLINK30);
        }
    };
    void onDisconnect(BLEServer *pServer)
    {
        isConnect[1] = false;
        // TODO Address[1] NodeNumChar LastUpdate
        Address[1] = "...";
        NodeNumChar->setValue(NULL);
        LastUpdate = -1;
        if (isConnect[0])
        {
            SubNodeNum->writeValue(Address[2]);
        }

        // Start Advertising
        if (keepAdvertising)
        {
            isAdvertising = true;
            LED.Write(1, 128, 128, 0, LED.BLINK30);
            BLEDevice::startAdvertising();
            oled.Block("Main Sensor,Disconnect");
        }
        else
        {
            LED.Write(1, 255, 0, 0, LED.LIGHT30);
        }
        Debug.println("Server (Main Sensor) Disconnect");
    }
};

class MyClientCallback : public BLEClientCallbacks // Call back when dropped connection
{
    void onConnect(BLEClient *ThisClient) {}
    void onDisconnect(BLEClient *ThisClient)
    {
        isConnect[0] = false;
        Address[0] = "....";
        LED.Write(0, 1, isScanning, 0, LED.LIGHT30);
        Debug.println("Client (Sub Sensor) Disconnect");
        Angle[0] = 0;
        Angle[1] = 0;
        Angle[2] = 0;
        RollAngChar->setValue(0);
        RollAngChar->notify(true);
        PitcAngChar->setValue(0);
        PitcAngChar->notify(true);
        YawwAngChar->setValue(0);
        YawwAngChar->notify(true);
        HaveSubChar->setValue(0);
        HaveSubChar->notify(true);
    }
};

class BlankClientCallback : public BLEClientCallbacks // Call back when dropped connection
{
    void onConnect(BLEClient *ThisClient) {}
    void onDisconnect(BLEClient *ThisClient)
    {
        Debug.print("Disconnect to Device : ");
        Debug.println(ThisClient->getPeerAddress().toString().c_str());
    }
};

class NodeNumCallBacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        String tNodeNum = NodeNumChar->getValue().c_str();
        if (tNodeNum == "0")
        {
            Address[1] = "...";
            return;
        }
        Address[1] = tNodeNum;
        LastUpdate = -1;
        if (isConnect[0])
        {
            SubNodeNum->writeValue(Address[1]);
        }
        LED.Write(1, 0, 0, 255, LED.LIGHT30);
    }
};

class WallAngCallBacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        MD.WallAngle = pCharacteristic->getValue<float>();
    }
};

class CommandCallBacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        float AngularVelocity = pCharacteristic->getValue<float>();
        if (!MD.Swich || !MD.Check)
        {
            CommandChar->notify();
            MD.Output(0);
        }
        else if (!MD.Output(AngularVelocity))
        {
            // If Motor return fault, tell main sensor to stop other motor driver and stop sending new command.
            CommandChar->notify();
            MD.Output(0);
        }
        LastUpdate = millis();
        LED.Write(1, 0, 0, 255, LED.BLINK30);
        isConnect[2] = true;
        // Serial.print(millis());Serial.print(" Start : ");Serial.println(AngularVelocity,6); // For Debug
    }
};

class RemTimeCallBacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        StepTime = pCharacteristic->getValue<int>();
        StepTimeTH = (StepTime < 500) ? StepTime + 50 : (StepTime > 1000) ? StepTime + 100
                                                                          : StepTime * 1.1;
        // Serial.print("Step Time Set : "); Serial.println(StepTime);
    }
};

class SetTimeCallBecks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        String SetTime = SetTimeChar->getValue().c_str();
        L_clock.SetTime(SetTime);
    }
};

static void CommandTimer(void *pvParameter)
{
    // Use short & fast program with high priority high frequency to check the time
    // ONLY FAST & SIMPLE CODE can be put in this task,
    // Using vTaskCreate and vTaskDelete to create a timer look pretty, BUT WILL FAILED EASILY !!!
    for (;;)
    {
        if (isConnect[2] && millis() - LastUpdate > StepTimeTH)
        {
            MD.Output(0);
            if (LastUpdate != -1)
                LastUpdate = millis(); // record the command stop time for oled show.
            isConnect[2] = false;
            if (isConnect[1])
                LED.Write(1, 0, 0, 255, LED.LIGHT30);
            else
                LED.Write(1, 128, 128, 0, LED.BLINK30);
            // Serial.print(millis());Serial.println(" Stop."); // For Debug
        }
        vTaskDelay(10);
    }
}

void BLE_Send_Update_Angle()
{
    if (isConnect[0])
    {
        Angle[0] = SubRollAng->readValue<float>();
        Angle[1] = SubPitcAng->readValue<float>();
        Angle[2] = SubYawwAng->readValue<float>();
        if (isConnect[1])
        {
            RollAngChar->setValue(Angle[0]);
            RollAngChar->notify(true);
            PitcAngChar->setValue(Angle[1]);
            PitcAngChar->notify(true);
            YawwAngChar->setValue(Angle[2]);
            YawwAngChar->notify(true);
        }
    }
}

void Server_Initialize()
{
    // Start BLE Deviec
    BLEDevice::init("Motor Driver V3.1");

    // Create Server
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    pServer->advertiseOnDisconnect(false);

    // Create Service
    BLEService *pService = pServer->createService(ServiceUUID);

    // Create Characteristic
    RollAngChar = pService->createCharacteristic(RollAngUUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    PitcAngChar = pService->createCharacteristic(PitcAngUUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    YawwAngChar = pService->createCharacteristic(YawwAngUUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    NodeNumChar = pService->createCharacteristic(NodeNumUUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY);
    CommandChar = pService->createCharacteristic(CommandUUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY);
    RemTimeChar = pService->createCharacteristic(TimeStpUUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
    SetTimeChar = pService->createCharacteristic(TimeSetUUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
    HaveSubChar = pService->createCharacteristic(HaveSubUUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    WallAngChar = pService->createCharacteristic(WallAngUUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);

    // Set Characteristic Callback
    NodeNumChar->setCallbacks(new NodeNumCallBacks());
    CommandChar->setCallbacks(new CommandCallBacks());
    WallAngChar->setCallbacks(new WallAngCallBacks());
    RemTimeChar->setCallbacks(new RemTimeCallBacks());
    SetTimeChar->setCallbacks(new SetTimeCallBecks());

    // Add Descriptor (Avoid using UUID 2902 (Already used by NimBLE))
    // Add 2901 Descriptor
    RollAngChar->createDescriptor("2901", NIMBLE_PROPERTY::READ, 25)->setValue("Roll");
    PitcAngChar->createDescriptor("2901", NIMBLE_PROPERTY::READ, 25)->setValue("Pitch");
    YawwAngChar->createDescriptor("2901", NIMBLE_PROPERTY::READ, 25)->setValue("Yaw");
    NodeNumChar->createDescriptor("2901", NIMBLE_PROPERTY::READ, 30)->setValue("Local ID");
    CommandChar->createDescriptor("2901", NIMBLE_PROPERTY::READ, 30)->setValue("Command");
    RemTimeChar->createDescriptor("2901", NIMBLE_PROPERTY::READ, 30)->setValue("Command Period");
    SetTimeChar->createDescriptor("2901", NIMBLE_PROPERTY::READ, 30)->setValue("Set Time");
    HaveSubChar->createDescriptor("2901", NIMBLE_PROPERTY::READ, 30)->setValue("Have Sub Sensor Connected ?");
    WallAngChar->createDescriptor("2901", NIMBLE_PROPERTY::READ, 30)->setValue("Average Wall Angle");

    // Set input type and unit

    p2904AngleRoll->setFormat(p2904AngleRoll->FORMAT_FLOAT32);
    p2904AnglePitc->setFormat(p2904AnglePitc->FORMAT_FLOAT32);
    p2904AngleYaww->setFormat(p2904AngleYaww->FORMAT_FLOAT32);
    p2904StrNodeID->setFormat(p2904StrNodeID->FORMAT_UTF8);
    p2904CommOmega->setFormat(p2904CommOmega->FORMAT_FLOAT32);
    p2904IntTimRem->setFormat(p2904IntTimRem->FORMAT_UINT16);
    p2904StrSetTim->setFormat(p2904StrSetTim->FORMAT_UTF8);
    p2904IntHavSub->setFormat(p2904IntHavSub->FORMAT_UINT16);
    p2904WallAngle->setFormat(p2904CommOmega->FORMAT_FLOAT32);

    p2904AngleRoll->setUnit(0x2763);
    p2904AnglePitc->setUnit(0x2763);
    p2904AngleYaww->setUnit(0x2763);
    p2904CommOmega->setUnit(0x2743);
    p2904WallAngle->setUnit(0x2763);

    RollAngChar->addDescriptor(p2904AngleRoll);
    PitcAngChar->addDescriptor(p2904AnglePitc);
    YawwAngChar->addDescriptor(p2904AngleYaww);
    NodeNumChar->addDescriptor(p2904StrNodeID);
    CommandChar->addDescriptor(p2904CommOmega);
    RemTimeChar->addDescriptor(p2904IntTimRem);
    SetTimeChar->addDescriptor(p2904StrSetTim);
    HaveSubChar->addDescriptor(p2904IntHavSub);
    WallAngChar->addDescriptor(p2904WallAngle);

    HaveSubChar->setValue(0);

    // Start the Service
    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(ServiceUUID);
    pAdvertising->setScanResponse(true);
    isAdvertising = true;
    LED.Write(1, 128, 128, 0, LED.BLINK30);
    BLEDevice::startAdvertising();
    xTaskCreatePinnedToCore(CommandTimer, "Command Timer", 2048, NULL, 5, &COMMAND_TIMER, 0);
}

void Client_Initialize()
{
    String Addr = BLEDevice::getAddress().toString().c_str();
    Address[2].setCharAt(0, Addr.charAt(12));
    Address[2].setCharAt(1, Addr.charAt(13));
    Address[2].setCharAt(2, Addr.charAt(15));
    Address[2].setCharAt(3, Addr.charAt(16));
    Address[2].toUpperCase();
    pBLEScan = BLEDevice::getScan();
    // https://esp32.com/viewtopic.php?t=2291
    // deal with the scan result after the scanning procedure, thus use interval = window
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(100);
}

void Client_Connect()
{
    if (isConnect[0] || !isScanning)
    {
        return;
    }
    if (isAdvertising)
    {
        BLEDevice::stopAdvertising();
    }
    //Serial.println("Searching for subsensor ...");
    LED.Write(0, 255, 255, 0, LED.BLINK30);

    pBLEScan->start(1, false);
    for (int i = 0; i < pBLEScan->getResults().getCount(); i++)
    {
        BLEAdvertisedDevice ThisDevice = pBLEScan->getResults().getDevice(i);
        if (ThisDevice.haveServiceUUID() && ThisDevice.isAdvertisingService(ServersUUID))
        {
            // Start Connection
            LED.Write(0, 0, 255, 0, LED.BLINK30);
            String ThisAddress = ThisDevice.getAddress().toString().c_str();
            Debug.print("Address Found : ");
            Debug.print(ThisAddress);

            Debug.print(" - Start Connection ");
            ThisClient = BLEDevice::createClient();
            if (!ThisClient->connect(ThisDevice.getAddress()))
            {
                Debug.print("Failed to Connnect to the Address "); // Serial.print(RequestAddress.toString().c_str());
                Debug.println("");
                goto NextDevice;
            }
            BLERemoteService *RequestService = ThisClient->getService(ServersUUID);
            if (RequestService == nullptr)
            {
                Debug.print("Can't find service ");
                Debug.println(ServiceUUID.toString().c_str());
                goto NextDevice;
            }

            String Characteristic[4] = {"Roll", "Pitch", "Yaw", "NodeNum"};
            SubRollAng = RequestService->getCharacteristic(RollAngUUID);
            SubPitcAng = RequestService->getCharacteristic(PitcAngUUID);
            SubYawwAng = RequestService->getCharacteristic(YawwAngUUID);
            SubNodeNum = RequestService->getCharacteristic(NodeNumUUID);

            if (SubRollAng == nullptr || SubPitcAng == nullptr || SubYawwAng == nullptr || SubNodeNum == nullptr)
            {
                Debug.println("Can't find Characteristic.");
                goto NextDevice;
            }
            if (!SubRollAng->canRead() || !SubPitcAng->canRead() || !SubYawwAng->canRead() || !SubNodeNum->canWrite())
            {
                Debug.println("Characteristic property dismatch.");
                goto NextDevice;
            }

            if (Address[1] == "...")
            {
                SubNodeNum->writeValue(Address[2]);
            }
            else
            {
                SubNodeNum->writeValue(Address[1]);
            }

            ThisClient->setClientCallbacks(new MyClientCallback());

            Debug.println("--> Connect Success.");
            isConnect[0] = true;
            LED.Write(0, 0, 0, 255, LED.LIGHT30);
            HaveSubChar->setValue(1);
            HaveSubChar->notify(true);
            BLE_Send_Update_Angle();

            Address[0].setCharAt(0, ThisAddress.charAt(12));
            Address[0].setCharAt(1, ThisAddress.charAt(13));
            Address[0].setCharAt(2, ThisAddress.charAt(15));
            Address[0].setCharAt(3, ThisAddress.charAt(16));
            Address[0].toUpperCase();
            break;
        }
        if (false)
        {
        NextDevice:;
            ThisClient->setClientCallbacks(new BlankClientCallback());
            ThisClient->disconnect();
        }
    }
    if (!isScanning)
    {
        ThisClient->disconnect();
    }
    if (!isConnect[0])
    {
        LED.Write(0, 128, 128, 0, LED.LIGHT30);
    }
    pBLEScan->clearResults();
    if (isAdvertising && keepAdvertising)
    {
        BLEDevice::startAdvertising();
    }
}

void Check_Server_Characteristic()
{
    Serial.print("Roll : ");
    Serial.println(RollAngChar->getValue<float>());
    Serial.print("Pitch: ");
    Serial.println(PitcAngChar->getValue<float>());
    Serial.print("Yaw  : ");
    Serial.println(YawwAngChar->getValue<float>());
    Serial.print("Node : ");
    Serial.println(NodeNumChar->getValue().c_str());
    Serial.print("Comm : ");
    Serial.println(CommandChar->getValue<float>());
    Serial.print("Step : ");
    Serial.println(RemTimeChar->getValue<int>());
    Serial.print("Time : ");
    Serial.println(SetTimeChar->getValue().c_str());
    Serial.print("Sub  : ");
    Serial.println(HaveSubChar->getValue<int>());
    Serial.print("Comm : ");
    Serial.println(WallAngChar->getValue<float>());
}

bool act = false;

void BLE_Manual_Command_Check()
{
    if (!act)
    {
        return;
    }
    if (Page == 1)
    {
        if (isConnect[1])
        {
            NodeNumChar->notify();
        }
        if (!keepAdvertising && isAdvertising)
        {
            isAdvertising = false;
            LED.Write(1, 255, 0, 0, LED.LIGHT30);
            BLEDevice::stopAdvertising();
        }
        else if (keepAdvertising && !isAdvertising)
        {
            isAdvertising = true;
            LED.Write(1, 128, 128, 0, LED.BLINK30);
            BLEDevice::startAdvertising();
        }
    }
    else if (Page == 0)
    {
        if (isConnect[0])
        {
            ThisClient->disconnect();
        }
        LED.Write(0, 1, isScanning, 0, LED.LIGHT30);
    }
    act = false;
}

#endif