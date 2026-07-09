#pragma once
#include <stdint.h>

// ============================================================
//  Protocol — struttura dati condivisa ESP32-S3 <-> CYD
// ============================================================

struct Telemetry {
    float    temp;
    float    humidity;
    float    setpoint;
    float    alarmTemp;   // limite allarme temperatura
    uint8_t  pwm;
    uint8_t  fan;
    uint8_t  alarm;
    uint32_t uptime;
};

enum class CmdType : uint8_t {
    SET_SETPOINT   = 0x01,
    SET_HUM_SP     = 0x02,
    RESET_ALARM    = 0x03,
    PID_PARAMS     = 0x04,
    SET_ALARM_TEMP = 0x05,   // nuovo: imposta limite allarme
};

struct Command {
    CmdType  type;
    float    value;
    float    kp;
    float    ki;
    float    kd;
};
