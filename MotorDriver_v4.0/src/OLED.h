#ifndef OLED_H
#define OLED_H
#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "MotorDriver.h"

#ifndef OLED_RST
#define OLED_RST 13
#endif

class OLED
{
public:
    void Initialize();
    void PowerSave(bool isPowerSave);
    void Update();
    byte *Page;
    byte *Cursor;
    void Block(String Str);
    MotorDriver *pMD;
    int *Encoder_Temp;
    int *Battery;
    bool *isAdvertising;
    bool *isScanning;
    bool (*isConnect)[3];
    float (*Angle)[3];
    String (*Address)[3];
    String *pMD_C_show;

private:
    const uint8_t *DefaultFont = u8g2_font_8x13B_tf;
    uint8_t OLED_Addr = 0x3C;
    char esp_mac[5];
    int BlockTimer = 0;
    void DrawBar();
    void Page_MD();
    void Page_BLE_S();
    void Page_BLE_M();
    void Page_Setting();
    void Page_Home();
    bool Flash(int C);
    void DrawArrorFrame(int x, int y, int L, bool Point);
    int DrawCenter(int x, int y, int L, const char * S);
    byte fPage = 1;
    int Count = 0;
};

#endif