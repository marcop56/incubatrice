#include "telemetry.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Formato JSON aggiornato con campi ciclo incubazione
// {"t":37.52,"h":58.1,"sp":37.5,"at":40.0,"pwm":142,"fan":0,"al":0,"up":3600,
//  "is":1,"ic":0,"ip":0,"id":5,"tp":0}

int telemetry_serialize(const Telemetry& t, char* buf, int buflen) {
    return snprintf(buf, buflen,
        "{\"t\":%.2f,\"h\":%.1f,\"sp\":%.2f,\"at\":%.1f,"
        "\"pwm\":%u,\"fan\":%u,\"al\":%u,\"up\":%lu,"
        "\"is\":%u,\"ic\":%u,\"ip\":%u,\"id\":%lu,\"tp\":%u}\n",
        t.temp, t.humidity, t.setpoint, t.alarmTemp,
        (unsigned)t.pwm, (unsigned)t.fan, (unsigned)t.alarm,
        (unsigned long)t.uptime,
        (unsigned)t.incState, (unsigned)t.incCycle,
        (unsigned)t.incPhase, (unsigned long)t.incDay,
        (unsigned)t.turnerPos);
}

static float jsonGetFloat(const char* json, const char* key, float def) {
    char needle[32];
    snprintf(needle, sizeof(needle), "\"%s\":", key);
    const char* p = strstr(json, needle);
    if (!p) return def;
    p += strlen(needle);
    return (float)atof(p);
}

static uint32_t jsonGetUint(const char* json, const char* key, uint32_t def) {
    char needle[32];
    snprintf(needle, sizeof(needle), "\"%s\":", key);
    const char* p = strstr(json, needle);
    if (!p) return def;
    p += strlen(needle);
    return (uint32_t)atol(p);
}

bool telemetry_parse(const char* json, Telemetry& out) {
    if (!json || json[0] != '{') return false;
    out.temp      = jsonGetFloat(json, "t",   -999.0f);
    out.humidity  = jsonGetFloat(json, "h",   -999.0f);
    out.setpoint  = jsonGetFloat(json, "sp",  -999.0f);
    out.alarmTemp = jsonGetFloat(json, "at",    40.0f);
    out.pwm       = (uint8_t) jsonGetUint(json, "pwm", 0);
    out.fan       = (uint8_t) jsonGetUint(json, "fan", 0);
    out.alarm     = (uint8_t) jsonGetUint(json, "al",  0);
    out.uptime    = (uint32_t)jsonGetUint(json, "up",  0);
    out.incState  = (uint8_t) jsonGetUint(json, "is",  0);
    out.incCycle  = (uint8_t) jsonGetUint(json, "ic",  0);
    out.incPhase  = (uint8_t) jsonGetUint(json, "ip",  0);
    out.incDay    = (uint32_t)jsonGetUint(json, "id",  0);
    out.turnerPos = (uint8_t) jsonGetUint(json, "tp",  0);
    return (out.temp > -998.0f);
}

int command_serialize(const Command& cmd, char* buf, int buflen) {
    return snprintf(buf, buflen,
        "{\"cmd\":%u,\"val\":%.2f,\"kp\":%.3f,\"ki\":%.4f,\"kd\":%.2f}\n",
        (unsigned)cmd.type, cmd.value, cmd.kp, cmd.ki, cmd.kd);
}

bool command_parse(const char* json, Command& out) {
    if (!json || json[0] != '{') return false;
    out.type  = (CmdType)jsonGetUint(json, "cmd", 0);
    out.value = jsonGetFloat(json, "val", 0.0f);
    out.kp    = jsonGetFloat(json, "kp",  0.0f);
    out.ki    = jsonGetFloat(json, "ki",  0.0f);
    out.kd    = jsonGetFloat(json, "kd",  0.0f);
    return true;
}
