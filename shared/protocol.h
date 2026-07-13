#pragma once
#include <stdint.h>

// ============================================================
//  Protocol — struttura dati condivisa ESP32-S3 <-> CYD
// ============================================================

struct Telemetry {
    float    temp;
    float    humidity;
    float    setpoint;
    float    alarmTemp;
    uint8_t  pwm;
    uint8_t  fan;
    uint8_t  alarm;
    uint32_t uptime;
    // Ciclo incubazione
    uint8_t  incState;      // 0=IDLE 1=RUNNING 2=PAUSED 3=COMPLETED
    uint8_t  incCycle;      // indice ciclo
    uint8_t  incPhase;      // fase corrente
    uint32_t incDay;        // giorno corrente nella fase
    uint8_t  turnerPos;     // 0=A 1=B
};

enum class CmdType : uint8_t {
    SET_SETPOINT   = 0x01,
    SET_HUM_SP     = 0x02,
    RESET_ALARM    = 0x03,
    PID_PARAMS     = 0x04,
    SET_ALARM_TEMP = 0x05,
    INC_START      = 0x06,   // val = indice ciclo
    INC_STOP       = 0x07,
    INC_PAUSE      = 0x08,
    INC_RESUME     = 0x09,
};

struct Command {
    CmdType  type;
    float    value;
    float    kp;
    float    ki;
    float    kd;
};
