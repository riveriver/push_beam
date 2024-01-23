#include "RGBLED.h"
CRGB leds[NUM_LEDS];

void RGBLED::SetUp()
{
#ifndef LED_PIN
#define LED_PIN 42
#endif
#ifndef LEDSetUp
#define LEDSetUp
    FastLED.addLeds<WS2812B, LED_PIN, RGB>(leds, NUM_LEDS);
#endif
    Off();
}

void RGBLED::Off()
{
    for (int i = 0; i < NUM_LEDS; i++)
    {
        leds[i] = CRGB::Black;
    }
    memset(GBR_Code, 0, sizeof(GBR_Code));
    memset(Color_Priority, 0, sizeof(Color_Priority));
    FastLED.show();
}

void RGBLED::Write(int n, int R, int G, int B, byte State, int Priority)
{
    if (n < NUM_LEDS)
    {
        if (R + G + B != 0)
        {
            Color_Priority[n] = max(Priority, Color_Priority[n]);
            int Bright = (State == LIGHT50) ? 50 : 30;
            GBR_Code[n][0][Priority] = G * Bright / (R + G + B);
            GBR_Code[n][1][Priority] = R * Bright / (R + G + B);
            GBR_Code[n][2][Priority] = B * Bright / (R + G + B);
            GBR_Code[n][3][Priority] = State;
        }
        else
        {
            GBR_Code[n][0][Priority] = 0;
            GBR_Code[n][1][Priority] = 0;
            GBR_Code[n][2][Priority] = 0;
            GBR_Code[n][3][Priority] = 0;
            int p = 1;
            while(p > 0 && GBR_Code[n][3][p] == 0)
                p--;
            Color_Priority[n] = p;
        }
    }
    else
    {
        Serial.println("Led num error");
    }
}

void RGBLED::Write(int n, int R, int G, int B, byte State)
{
    Write(n, R, G, B, State, 0);
}

void RGBLED::Update()
{
    Count++;
    for (int i = 0; i < NUM_LEDS; i++)
    {
        if (GBR_Code[i][3][Color_Priority[i]] == BLINK30)
        {
            if (Count % 2 == i%2)
            {
                leds[i] = CRGB(GBR_Code[i][0][Color_Priority[i]], GBR_Code[i][1][Color_Priority[i]], GBR_Code[i][2][Color_Priority[i]]);
            }
            else
            {
                leds[i] = CRGB::Black;
            }
        }
        else if (GBR_Code[i][3][Color_Priority[i]] == 0)
        {
            leds[i] = CRGB::Black;
        }
        else
        {
            leds[i] = CRGB(GBR_Code[i][0][Color_Priority[i]], GBR_Code[i][1][Color_Priority[i]], GBR_Code[i][2][Color_Priority[i]]);
        }
    }
    FastLED.show();
}