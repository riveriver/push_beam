#ifndef Control_H
#define Control_H

#include "SDCard.h"

class Control
{
    public:
    
    int Cursor = 1;
    int dt_ms = 250;
    double V;
    int UserSetting[8];
    void Estimate(double AngleEstimate);
    void Stop();
    void Start(bool &SendCommand,double AngleEstimate,SDCard* SD);
    void Set();
    void ClearSet();
    void UserSet(int i);
    bool CheckSetting();
    bool SettingMode = false; // true then can set Kp Ki Kd dt
    

    private:
    //double a = 0.35; //m
    //double b = 0.45; //m
    
    //double CommandDeltaL;
    int AngleCommand = 90;
    int t_command = 0;
    double Xt[5000];
    double Vt[5000];
    // Set maximum anguler velocity (Easier to control)
    // Omg_max ~(c * Vc_Max)/ (a x b x sin(AngleEstimate)) < Vc_Max / H @ AngleEsitmate ~ 90 
    // can change into setting Vc_Max control
    double V_Max = 0.005; // m/s
    double V_Max_define = 0.010;
    double V_min = 0.0005;
    double A_Max = 0.0025;
    double dt_s = dt_ms/1000.0 ; //s
    double Kp = 0.13;
    double Ki = 0.0001;
    double Kd = 0.1;
    double E_t0 = 0;
    double E_integral = 0;
    double E_differential = 0;
    double E_t1;
    double H = 1.5; //m
    double UserSettingShow[7] = {(double)AngleCommand,H,V_Max*10000,Kp,Ki,Kd,dt_s};
    bool SDMode = true;
    bool isStable;
    bool isStop = true;
    SDCard* p_SD;

};

#endif