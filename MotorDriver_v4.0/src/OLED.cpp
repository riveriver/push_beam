#include "OLED.h"

#include <Arduino.h>
#include <U8g2lib.h>
#include "OLED_XBM.h"

#if OLED_RST == -1
U8G2_SSD1309_128X64_NONAME0_F_HW_I2C u8g2(U8G2_R0);
#else
U8G2_SSD1309_128X64_NONAME0_F_HW_I2C u8g2(U8G2_R0, OLED_RST);
#endif

void OLED::Initialize()
{
    delay(500); // Wait for pullup
    PowerSave(0);
    u8g2.enableUTF8Print();
    u8g2.setFont(DefaultFont);
    u8g2.setFontDirection(0);
    u8g2.clearBuffer();
    int StrNum = 2;
    String S[StrNum] = {"Motor Driver", "V 4.0"};
    int a = u8g2.getAscent();
    int l = (u8g2.getAscent() - u8g2.getDescent()) * 1.5;
    int h = (64 - l * (StrNum - 1) - a) / 2;
    for (int i = 0; i < StrNum; i++)
    {
        DrawCenter(0, h + l * i + a, 128, S[i].c_str());
    }
    u8g2.sendBuffer();
    uint8_t mac_id[6];
    esp_efuse_mac_get_default(mac_id);
    sprintf(esp_mac, "%02x%02x", mac_id[4], mac_id[5] + 2);
}

void OLED::PowerSave(bool isPowerSave)
{
    Wire.begin();
    Wire.beginTransmission(60);
    byte error = Wire.endTransmission();
    if (error == 0)
        OLED_Addr = 0x3C;
    else
        OLED_Addr = 0x3D;
    u8g2.setI2CAddress(OLED_Addr * 2);
    u8g2.begin();
    u8g2.clearBuffer();
    u8g2.sendBuffer();
    u8g2.setPowerSave(isPowerSave);
}

void OLED::Block(String Str)
{
    BlockTimer = millis();
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_helvR08_tr);
    int StrNum = 0;
    int IndexNum = Str.indexOf(",");
    String str[5];
    while (IndexNum != -1 && StrNum < 4)
    {
        str[StrNum] = Str.substring(0, IndexNum);
        Str = Str.substring(IndexNum + 1);
        IndexNum = Str.indexOf(",");
        StrNum++;
    }
    str[StrNum] = Str;
    StrNum++;
    int a = u8g2.getAscent();
    int l = (u8g2.getAscent() - u8g2.getDescent()) * 1.3;
    int h = (64 - l * (StrNum - 1) - a) / 2;
    for (int i = 0; i < StrNum; i++)
    {
        int w = u8g2.getStrWidth(str[i].c_str());
        u8g2.drawStr((128 - w) / 2, h + l * i + a, str[i].c_str());
    }
    u8g2.sendBuffer();
}

void OLED::Update()
{
    if (millis() - BlockTimer < 2000)
    {
        return;
    }
    u8g2.clearBuffer();
    DrawBar();
    u8g2.setFont(u8g2_font_helvR08_tr);
    switch (*Page)
    {
    case 0:
        Page_BLE_S();
        break;
    case 1:
        Page_BLE_M();
        break;
    case 2:
        Page_Home();
        break;
    case 3:
        Page_MD();
        break;
    case 4:
        Page_Setting();
        break;
    default:
        break;
    }
    u8g2.sendBuffer();
}

void OLED::DrawBar()
{
    u8g2.setFont(u8g2_font_6x12_tr);
    fPage = (*Page < fPage) ? *Page : (*Page > fPage + 2) ? *Page - 2
                                                          : fPage;
    for (int i = 0; i < 3; i++)
    {
        u8g2.setDrawColor(1);
        if (i == *Page - fPage)
        {
            u8g2.drawRFrame(0, 6 + 17 * i, 18, 18, 3);
            if (*Cursor == 0)
            {
                u8g2.drawRBox(2, 8 + i * 17, 14, 14, 1);
                u8g2.setDrawColor(0);
            }
            else
            {
                u8g2.drawRFrame(2, 8 + i * 17, 14, 14, 1);
            }
        }
        else
        {
            u8g2.drawFrame(2, 8 + i * 17, 14, 14);
        }

        switch (i + fPage)
        {
        case 0:
            u8g2.drawXBM(4, 10 + i * 17, 10, 10, BLES_10x10_XBM);
            break;
        case 1:
            u8g2.drawXBM(4, 10 + i * 17, 10, 10, BLEC_10x10_XBM);
            break;
        case 2:
            u8g2.drawXBM(4, 10 + i * 17, 10, 10, Home_10x10_XBM);
            break;
        case 3:
            u8g2.drawXBM(4, 10 + i * 17, 10, 10, BLDC_10x10_XBM);
            break;
        case 4:
            u8g2.drawXBM(4, 10 + i * 17, 10, 10, Ruler_10x10_XBM);
            break;
        }
    }
    u8g2.setDrawColor(1);

    switch (fPage)
    {
    case 0:
        u8g2.drawBox(2, 3, 14, 2);
        for (int i = 0; i < 7; i++)
            u8g2.drawBox(2 + i * 2, 59, 2, 5 - abs(3 - i));
        break;
    case 2:
        for (int i = 0; i < 7; i++)
            u8g2.drawBox(2 + i * 2, abs(3 - i), 2, 5 - abs(3 - i));
        u8g2.drawBox(2, 59, 14, 2);
        break;
    default:
        for (int i = 0; i < 7; i++)
        {
            u8g2.drawBox(2 + i * 2, abs(3 - i), 2, 5 - abs(3 - i));
            u8g2.drawBox(2 + i * 2, 59, 2, 5 - abs(3 - i));
        }
        break;
    }
}

void OLED::Page_Home()
{
    u8g2.setFont(u8g2_font_5x8_tr);
    DrawCenter(20, 6, 58, "CONTROLLER");
    DrawCenter(80, 6, 48, "SENSOR");
    u8g2.drawHLine(24, 7, 49);
    u8g2.drawHLine(85, 7, 38);
    u8g2.setFont(u8g2_font_6x12_tr);
    if (*(*isConnect + 1))
    {
        DrawCenter(20, 47, 58, ("LLA:" + *(*Address + 2)).c_str());
        u8g2.setFont(u8g2_font_profont29_tr);
        DrawCenter(20, 34, 58, (*Address + 1)->c_str());
    }
    else
    {
        if (*isAdvertising)
            DrawCenter(20, 47, 58, "Waiting");
        else
            DrawCenter(20, 47, 58, "Disable");
        u8g2.setFont(u8g2_font_profont22_tr);
        DrawCenter(20, 32, 58, (*Address + 2)->c_str());
    }
    u8g2.drawBox(78, 0, 2, 50);
    if (*isConnect[0] && *isScanning)
    {
        if (*Angle[0] == 0)
        {
            int x = 104;
            int t = Count % 20;
            uint16_t DotSize[4] = {1, 2, 2, 3};
            for (int i = 0; i < 4; i++)
            {
                u8g2.drawDisc(x + 8 * sin(t / 20.0 * 2.0 * PI), 26 - 8 * cos(t / 20.0 * 2.0 * PI), DotSize[i]);
                t = (t + 3) % 20;
            }
            Count++;
        }
        else
        {
            u8g2.setFont(u8g2_font_5x7_tr);
            char A[8];
            for (int i = 0; i < 3; i++)
            {
                u8g2.drawGlyph(83, 22 + i * 10, 88 + i);
                u8g2.drawGlyph(88, 22 + i * 10, 0x003a);
                dtostrf(*(*Angle + i), 7, 2, A);
                u8g2.drawStr(93, 22 + i * 10, A);
            }
        }
    }
    else if (*isScanning)
    {
        u8g2.setFont(u8g2_font_open_iconic_all_2x_t);
        u8g2.drawGlyph(97, 35, 0x005e);
        Count++;
        Count %= 20;
        u8g2.drawBox(96 + 15 * (Count < 10), 26, 2, 2);
    }
    else
    {
        u8g2.setFont(u8g2_font_open_iconic_all_2x_t);
        u8g2.drawGlyph(97, 35, 0x00eb);
    }

    u8g2.drawBox(20, 50, 108, 2);
    u8g2.setFont(u8g2_font_8x13B_tr);
    if (!pMD->Swich)
    {
        if (Flash(1000))
            DrawCenter(20, 64, 108, "LOCK");
    }
    else if (!pMD->Check)
    {
        if (Flash(500))
            DrawCenter(20, 64, 108, "ERROR");
    }
    else if (*pMD_C_show == "00:00:00")
    {
        DrawCenter(20, 64, 108, "READY");
    }
    else
    {
        u8g2.setFont(u8g2_font_7x14B_tr);
        u8g2.drawStr(20, 64, pMD_C_show->c_str());
        u8g2.setFont(u8g2_font_unifont_t_86);
        u8g2.drawGlyph(77, 65, 0x2b62);
        u8g2.setFont(u8g2_font_8x13B_tr);
        if (*(*isConnect + 2))
            u8g2.drawStr(128 - 8 * String(pMD->u_out).length(), 64, String(pMD->u_out).c_str());
        else
            u8g2.drawStr(94, 64, "STOP");
    }
}

void OLED::Page_MD()
{
    u8g2.setFont(u8g2_font_9x15_tr);
    u8g2.drawStr(25, 15, "Vc");
    u8g2.drawStr(40, 14, ":");
    DrawArrorFrame(49, 4, 57, (*Cursor == 1));
    if (*Cursor == 1)
    {
        u8g2.drawBox(51, 6, 53, 9);
        u8g2.setDrawColor(0);
    }

    if (!pMD->Swich)
    {
        u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
        u8g2.drawGlyph(73, 15, 0x00ca);
    }
    else if (!pMD->Check)
    {
        u8g2.setFont(u8g2_font_siji_t_6x10);
        u8g2.drawGlyph(73, 15, 0xe0b3);
    }
    else
    {
        u8g2.setFont(u8g2_font_6x12_tr);
        u8g2.drawStr(85 - String(pMD->u_out).length() * 6, 14, String(pMD->u_out).c_str());
    }

    u8g2.setDrawColor(2);
    u8g2.drawHLine(20, 22, 108);
    char Title[3][4] = {"Ve:", "Ie:", "Ee:"};
    double V[3] = {pMD->SpeedR, (double)pMD->Current * 7.6 - 0.22, *Battery * 4.5 / 100.0 + 16.5};
    int Vp[3] = {(int)(max(pMD->SpeedR, 0.0F) * 86 / 120), (int)(V[1] * 86 / 10), *Battery * 86 / 100};
    for (int i = 0; i < 3; i++)
    {
        u8g2.drawFrame(37, 27 + 13 * i, 90, 11);
        u8g2.setFont(u8g2_font_6x12_tr);
        u8g2.drawStr(20, 36 + 13 * i, Title[i]);
        u8g2.setFont(u8g2_font_tinyface_tr);
        String S = String(V[i], 1);
        u8g2.drawStr(82 - 4 * S.length(), 35 + 13 * i, S.c_str());
        u8g2.drawXBM(83, 30 + 13 * i, MD_unit_w[i], 5, MD_unit_XBM[i]);
        u8g2.drawBox(39, 29 + 13 * i, Vp[i], 7);
    }
}

void OLED::Page_BLE_S()
{
    u8g2.setFont(u8g2_font_8x13B_tr);
    u8g2.drawStr(20, 10, "Sensor");
    u8g2.drawHLine(20, 12, 108);

    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.drawStr(20, 24, "-LLA..........");
    u8g2.drawStr(104, 24, (*Address + 2)->c_str());

    char able[4] = "OFF";

    if (*isConnect[0] && *isScanning)
    {
        u8g2.drawStr(20, 35, "-State.....Connect");
        u8g2.drawStr(20, 46, "--Address.......");
        u8g2.drawStr(104, 46, (*Address)->c_str());
        ;
    }
    else if (*isScanning)
    {
        u8g2.drawStr(20, 35, "-State....Scanning");
    }
    else
    {
        u8g2.drawStr(20, 35, "-State.....Disable");
        memcpy(able, "ON", sizeof(able));
    }
    u8g2.setFont(u8g2_font_5x8_tr);
    u8g2.drawFrame(54, 52, 40, 12);
    DrawCenter(54, 61, 40, able);
    if (*Cursor == 1)
    {
        u8g2.setDrawColor(2);
        u8g2.drawBox(56, 54, 36, 8);
        u8g2.setDrawColor(1);
    }
}

void OLED::Page_BLE_M()
{
    u8g2.setFont(u8g2_font_8x13B_tr);
    u8g2.drawStr(20, 10, "Controller");
    u8g2.drawHLine(20, 12, 108);

    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.drawStr(20, 24, "-LLA..........");
    u8g2.drawStr(104, 24, (*Address + 2)->c_str());

    char able[4] = "OFF";
    if (*(*isConnect + 2))
    {
        u8g2.drawStr(20, 35, "-State......Moving");
        u8g2.drawStr(20, 46, "--Address.......");
        u8g2.drawStr(116, 46, (*Address + 1)->c_str());
    }
    if (*(*isConnect + 1))
    {
        u8g2.drawStr(20, 35, "-State.....Connect");
        u8g2.drawStr(20, 46, "--Address.......");
        u8g2.drawStr(116, 46, (*Address + 1)->c_str());
    }
    else if (*isAdvertising)
    {
        u8g2.drawStr(20, 35, "-State.....Waiting");
    }
    else
    {
        u8g2.drawStr(20, 35, "-State.....Disable");
        memcpy(able, "ON", sizeof(able));
    }
    u8g2.setFont(u8g2_font_5x8_tr);
    u8g2.drawFrame(54, 52, 40, 12);
    DrawCenter(54, 61, 40, able);
    if (*Cursor == 1)
    {
        u8g2.setDrawColor(2);
        u8g2.drawBox(56, 54, 36, 8);
        u8g2.setDrawColor(1);
    }
}

void OLED::Page_Setting()
{
    DrawArrorFrame(32, 8, 50, (*Cursor == 1));
    u8g2.setFont(u8g2_font_6x12_t_cyrillic);
    u8g2.drawStr(22, 18, "H:");
    u8g2.drawGlyph(22, 38, 0x0472);
    u8g2.drawStr(28, 38, "l:");
    if (*(*isConnect + 1))
    {
        u8g2.drawGlyph(22, 50, 0x0472);
        u8g2.drawStr(28, 50, "w:");
    }

    u8g2.setFont(u8g2_font_6x12_tf);
    u8g2.drawStr(44 - (pMD->H > 999) * 6, 18, String(pMD->H).c_str());
    u8g2.drawStr(65, 18, "mm");

    char MonAng[6];
    dtostrf(*pMD->MountedAngle, 5, 1, MonAng);
    u8g2.drawStr(45, 38, MonAng);
    u8g2.drawGlyph(75, 38, 0x00b0);
    if (*(*isConnect + 1))
    {
        dtostrf(pMD->WallAngle, 5, 1, MonAng);
        u8g2.drawStr(45, 50, MonAng);
        u8g2.drawGlyph(75, 50, 0x00b0);
    }

    if (*Cursor != 0)
    {
        u8g2.setDrawColor(2);
        u8g2.drawBox(34, 16 * (*Cursor) - 6, 46, 9);
        u8g2.setDrawColor(1);
    }

    u8g2.drawTriangle(122, 14, 122, 56, 80, 56);
    u8g2.setDrawColor(0);
    u8g2.drawTriangle(120, 19, 120, 54, 85, 54);
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_5x8_tr);
    u8g2.drawStr(124, 40, "H");
}

void OLED::DrawArrorFrame(int x, int y, int L, bool Point)
{
    u8g2.drawFrame(x, y, L + 9, 13);
    u8g2.drawVLine(x + L - 1, y + 1, 11);
    u8g2.drawHLine(x + L, y + 6, 8);
    u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
    u8g2.drawGlyph(x + L, y + 7, 0x0070);
    u8g2.drawGlyph(x + L, y + 14, 0x006d);
    if (Point)
    {
        u8g2.setDrawColor(2);
        if (*Encoder_Temp > 0)
            u8g2.drawBox(x + L, y + 1, 8, 5);
        else if (*Encoder_Temp < 0)
            u8g2.drawBox(x + L, y + 7, 8, 5);
        u8g2.setDrawColor(1);
    }
    u8g2.setFont(u8g2_font_6x12_tr);
}

int OLED::DrawCenter(int x, int y, int L, const char *S)
{
    int Sl = u8g2.getStrWidth(S);
    u8g2.drawStr(x + (L - Sl) / 2, y, S);
    return (L - Sl) / 2;
}

bool OLED::Flash(int C)
{
    return (millis() / C % 2 == 1);
}
