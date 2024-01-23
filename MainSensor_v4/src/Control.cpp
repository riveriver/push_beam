#include "Control.h"
#include <Arduino.h>
#include "SerialDebug.h"

#define PI 3.1415926535897932384626433832795

void Control::Start(bool &SendCommand, double AngleEstimate, SDCard *pSD)
{
    isStable = false;
    isStop = false;
    // 度转弧度
    double Xc = AngleCommand * PI / 180.0;
    double Xe = AngleEstimate * PI / 180.0;
    // 初始化位置和速度数组
    Xt[0] = Xe;
    Vt[0] = 0;
    // 获取运动方向
    int direction = (Xc > Xe) ? 1 : -1;
    // 重置命令时间
    t_command = 0;
    // TODO 70？计算控制参数
    double omega_M = V_Max / H / sin(PI / 180 * 70);
    double alpha_M = A_Max / H - omega_M * omega_M;//每秒增加的速度值
    // 初始化时间索引
    int t = 0;
    // 执行PID控制器的“精细调整”阶段。在这个阶段，系统会尝试逐渐接近目标状态
    // 执行PID控制器的“粗调”阶段。在这个阶段，系统会尽快达到最大速度，然后以最大速度向目标状态移动
    // TODO 看不懂
    if (abs(Xc - Xe) < omega_M * omega_M / alpha_M)
    {
        // TODO 反向运动 迭代位速 
        while (Vt[t] * direction < pow(abs(Xc - Xe) * alpha_M, 0.5) && t < 5000)
        {
            t++;
            Vt[t] = Vt[t - 1] + direction * alpha_M * dt_s;
            Xt[t] = Xt[t - 1] + Vt[t] * dt_s;
        }
        // TODO
        t--; // wishing never have overshoot.
        // TODO 正向运动 迭代位速
        while (Vt[t] * direction > 0 && (Xc - Xt[t]) * direction > 0 && t < 5000)
        {
            t++;
            Vt[t] = Vt[t - 1] - direction * alpha_M * dt_s;
            Xt[t] = Xt[t - 1] + Vt[t] * dt_s;
        }
         // TODO 值不传递就初始化么？ 初始化迭代值，以便下次计算
        for (int i = t; i < 5000; i++)
        {
            Vt[i] = 0;
            Xt[i] = Xc;
        }
    }
    else
    {
        while (Vt[t] * direction < omega_M && t < 5000)
        {
            t++;
            Vt[t] = Vt[t - 1] + direction * alpha_M * dt_s;
            Xt[t] = Xt[t - 1] + Vt[t] * dt_s;
        }
        Vt[t] = omega_M * direction;
        // 计算本次到达位置
        Xt[t] = Xt[t - 1] + Vt[t] * dt_s;
        double V_Max_due = 0;
        while (V_Max_due < (abs(Xc - Xe) - omega_M * omega_M / alpha_M) / omega_M && t < 5000)
        {
            t++;
            V_Max_due += dt_s;
            Vt[t] = omega_M * direction;
            Xt[t] = Xt[t - 1] + Vt[t] * dt_s;
        }
        t--;
        while (Vt[t] * direction > 0 && (Xc - Xt[t]) * direction > 0 && t < 5000)
        {
            t++;
            Vt[t] = Vt[t - 1] - direction * alpha_M * dt_s;
            Xt[t] = Xt[t - 1] + Vt[t] * dt_s;
        }
        for (int i = t; i < 5000; i++)
        {
            Vt[i] = 0;
            Xt[i] = Xc;
        }
    }

    // cali done,set send flag to 1
    SendCommand = true;
    ClearSet();
    SDMode = pSD->SwichMode;
    pSD->Swich(0);
    p_SD = pSD;
}

void Control::Estimate(double AngleEstimate)
{
    // Assume no time delay issue
    // calculate in radian
    if (isStop /* || isStable */ && abs(AngleCommand - AngleEstimate) > 0.1)
    {
        bool NoUsed;
        Start(NoUsed, AngleEstimate, p_SD);
    }

    if (!isStable)
    {
        t_command++;
        // Velocit Control
        E_t1 = (t_command < 5000) ? Xt[t_command] - AngleEstimate * PI / 180 : (AngleCommand - AngleEstimate) * PI / 180;
        // PID
        E_integral += E_t1 * dt_s;
        E_differential = (E_t1 - E_t0) / dt_s;
        V = (Kp * E_t1 + Ki * E_integral + Kd * E_differential) / dt_s;

        if (abs(V) > V_Max / H)
        {
            V = (V > 0) ? V_Max / H : -V_Max / H;
        }
        else if (abs(V) < V_min / H)
        {
            V = 0;
            if (t_command >= 5000 || Vt[t_command] == 0)
            {
                // isStable = true;
                // Stop();
                // delay(1500);
            }
        }
    }
        E_t0 = E_t1;
}

void Control::Stop()
{
    E_t0 = 0;
    E_t1 = 0;
    E_integral = 0;
    E_differential = 0;
    V = 0;
    isStop = true;
    p_SD->Swich(SDMode);
}

bool Control::CheckSetting()
{
    if (UserSettingShow[0] != AngleCommand)
    {
        return false;
    }
    if (UserSettingShow[1] != H)
    {
        return false;
    }
    if (UserSettingShow[2] != V_Max * 10000)
    {
        return false;
    }
    if (UserSettingShow[3] != Kp)
    {
        return false;
    }
    if (UserSettingShow[4] != Ki)
    {
        return false;
    }
    if (UserSettingShow[5] != Kd)
    {
        return false;
    }
    if (UserSettingShow[6] != dt_s)
    {
        return false;
    }
    return true;
}

void Control::Set()
{
    AngleCommand = UserSettingShow[0];
    H = UserSettingShow[1];
    V_Max = UserSettingShow[2] / 10000.0;
    Kp = UserSettingShow[3];
    Ki = UserSettingShow[4];
    Kd = UserSettingShow[5];
    dt_s = UserSettingShow[6];
}

void Control::ClearSet()
{
    double NowSetting[7] = {(double)AngleCommand, H, V_Max * 10000, Kp, Ki, Kd, dt_s};
    memcpy(UserSettingShow, &NowSetting, sizeof(NowSetting));
    if (AngleCommand < 0)
    {
        UserSetting[0] = -1;
    }
    else
    {
        UserSetting[0] = 1;
    }
    UserSetting[1] = abs(AngleCommand) / 10;
    UserSetting[2] = abs(AngleCommand) % 10;
    int Hx100 = H * 100;
    UserSetting[3] = Hx100 / 100;
    UserSetting[4] = (Hx100 / 10) % 10;
    UserSetting[5] = (Hx100 % 10);
    int dVx1000 = V_Max * 10000;
    UserSetting[6] = dVx1000 / 10;
    UserSetting[7] = dVx1000 % 10;
}

void Control::UserSet(int i)
{
    int MaxShow;
    if (SettingMode)
    {
        MaxShow = 7;
    }
    else
    {
        MaxShow = 3;
    }
    if (Cursor < 10 && Cursor > 0)
    {
        if (i == 1)
        {
            Cursor *= 10;
            Cursor++;
        }
        else if (i == 0 && Cursor != 1)
        {
            Cursor--;
        }
        else if (i == 2 && Cursor != MaxShow)
        {
            Cursor++;
        }
    }
    else
    {
        switch (Cursor)
        {
        case 11:
            if (i == 1)
            {
                Cursor++;
            }
            else if (i == 0)
            {
                UserSetting[0] *= -1;
            }
            else if (i == 2)
            {
                UserSetting[0] *= -1;
            }
            break;
        case 12:
            if (i == 1)
            {
                Cursor++;
            }
            else if (i == 0)
            {
                UserSetting[1]++;
                UserSetting[1] %= 10;
            }
            else if (i == 2)
            {
                UserSetting[1] += 9;
                UserSetting[1] %= 10;
            }
            break;
        case 13:
            if (i == 1)
            {
                Cursor = 1;
            }
            else if (i == 0)
            {
                UserSetting[2]++;
                UserSetting[2] %= 10;
            }
            else if (i == 2)
            {
                UserSetting[2] += 9;
                UserSetting[2] %= 10;
            }
            break;
        case 21:
            if (i == 1)
            {
                Cursor++;
            }
            else if (i == 0)
            {
                UserSetting[3]++;
                UserSetting[3] %= 10;
            }
            else if (i == 2)
            {
                UserSetting[3] += 9;
                UserSetting[3] %= 10;
            }
            break;
        case 22:
            if (i == 1)
            {
                Cursor++;
            }
            else if (i == 0)
            {
                UserSetting[4]++;
                UserSetting[4] %= 10;
            }
            else if (i == 2)
            {
                UserSetting[4] += 9;
                UserSetting[4] %= 10;
            }
            break;
        case 23:
            if (i == 1)
            {
                Cursor = 2;
            }
            else if (i == 0)
            {
                UserSetting[5]++;
                UserSetting[5] %= 10;
            }
            else if (i == 2)
            {
                UserSetting[5] += 9;
                UserSetting[5] %= 10;
            }
            break;
        case 31:
            if (i == 1)
            {
                Cursor++;
            }
            else if (i == 0 && UserSetting[6] < V_Max_define * 1000)
            {
                UserSetting[6]++;
            }
            else if (i == 2 && UserSetting[6] > 1)
            {
                UserSetting[6]--;
            }
            break;
        case 32:
            if (i == 1)
            {
                Cursor = 3;
            }
            else if (i == 0)
            {
                UserSetting[7]++;
                UserSetting[7] %= 10;
            }
            else if (i == 2)
            {
                UserSetting[7] += 9;
                UserSetting[7] %= 10;
            }
            break;
        }
    }
    UserSettingShow[0] = UserSetting[0] * (UserSetting[1] * 10 + UserSetting[2]);
    UserSettingShow[1] = UserSetting[3] + UserSetting[4] * 0.1 + UserSetting[5] * 0.01;
    UserSettingShow[2] = UserSetting[6] * 10 + UserSetting[7];
}