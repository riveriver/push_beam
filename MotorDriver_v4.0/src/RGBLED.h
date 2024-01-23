#ifndef RGBLED_H
#define RGBLED_H
#include <FastLED.h>
#define NUM_LEDS 3

class RGBLED
{
private:
    int GBR_Code[NUM_LEDS][4][2] = {0};
    int Count = 0;
    int Color_Priority[NUM_LEDS] = {0};

public:
    const byte BLINK30 = 1;
    const byte LIGHT30 = 2;
    const byte LIGHT50 = 4;
    void SetUp();
    void Off();
    void Write(int n, int R, int G, int B, byte State, int Priority);
    void Write(int n, int R, int G, int B, byte State);
    void Update();
};

extern RGBLED LED;

#endif


