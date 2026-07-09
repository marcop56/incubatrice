#include <Arduino.h>
#include <Preferences.h>
#include "../../shared/hal.h"
#include "../../shared/pid.h"
#include "../../shared/protocol.h"
#include "../../shared/telemetry.h"
#include "ble_server.h"
#include "wifi_manager.h"
#include "web_server.h"

// ============================================================
//  ESP32-S3 — main
// ============================================================

extern void hal_updateSsr();

static const float DEFAULT_SETPOINT   = 37.5f;
static const float DEFAULT_ALARM_TEMP = 40.0f;
static const float DEFAULT_HUM_SP     = 58.0f;
static const float KP                 = 10.0f;
static const float KI                 =  0.05f;
static const float KD                 =  0.0f;
static const float PID_DT             =  1.0f;
static const float PWM_MIN            =  0.0f;
static const float PWM_MAX            = 255.0f;
static const float PWM_BIAS           = 128.0f;

static float         alarmTemp = DEFAULT_ALARM_TEMP;
static float         humSP     = DEFAULT_HUM_SP;
static PidController pid(KP, KI, KD, PID_DT, PWM_MIN, PWM_MAX, PWM_BIAS);
static Telemetry     telem;
static bool          tempAlarm = false;
static Preferences   prefs;

static void savePrefs() {
    prefs.begin("incubatrice", false);
    prefs.putFloat("setpoint",  pid.setpoint);
    prefs.putFloat("alarmTemp", alarmTemp);
    prefs.putFloat("humSP",     humSP);
    prefs.end();
    Serial.println("[NVS] salvato");
}

static void loadPrefs() {
    prefs.begin("incubatrice", true);
    pid.setpoint = prefs.getFloat("setpoint",  DEFAULT_SETPOINT);
    alarmTemp    = prefs.getFloat("alarmTemp", DEFAULT_ALARM_TEMP);
    humSP        = prefs.getFloat("humSP",     DEFAULT_HUM_SP);
    prefs.end();
    Serial.printf("[NVS] caricato: sp=%.1f at=%.1f hsp=%.1f\n",
                  pid.setpoint, alarmTemp, humSP);
}

// --- Callback comandi da BLE e WebSocket ---
void onCommand(const Command& cmd) {
    switch (cmd.type) {
        case CmdType::SET_SETPOINT:
            pid.setpoint = cmd.value;
            savePrefs();
            break;
        case CmdType::SET_HUM_SP:
            humSP = cmd.value;
            savePrefs();
            break;
        case CmdType::RESET_ALARM:
            tempAlarm = false;
            pid.reset();
            Serial.println("RESET_ALARM ricevuto");
            break;
        case CmdType::PID_PARAMS:
            pid.Kp = cmd.kp;
            pid.Ki = cmd.ki;
            pid.Kd = cmd.kd;
            pid.reset();
            savePrefs();
            break;
        case CmdType::SET_ALARM_TEMP:
            alarmTemp = cmd.value;
            Serial.printf("alarmTemp -> %.1f\n", alarmTemp);
            savePrefs();
            break;
    }
}

void setup() {
    Serial.begin(115200);
    loadPrefs();
    hal_init();

    // WiFi — blocca max 3 minuti per configurazione AP
    wifi_init();

    // Web server — solo se WiFi connesso
    if (wifi_connected()) {
        web_server_init(onCommand);
        Serial.printf("[WEB] apri http://%s nel browser\n", wifi_ip().c_str());
    }

    // BLE sempre attivo
    ble_server_init("Incubatrice", onCommand);

    Serial.println("[S3] pronto");
}

void loop() {
    static unsigned long tLastPid = 0;
    static unsigned long tLastLog = 0;

    unsigned long now = hal_millis();

    hal_updateSsr();

    if ((now - tLastPid) >= (unsigned long)(PID_DT * 1000)) {
        tLastPid = now;

        float temp = hal_readTemp();
        float hum  = hal_readHumidity();

        tempAlarm = (temp >= alarmTemp) || (temp < -900.0f);

        float pwm;
        if (tempAlarm) {
            pwm = 0.0f;
            pid.reset();
        } else {
            pwm = pid.compute(temp);
        }
        hal_writeOutput(pwm);

        bool fanOn = (hum > humSP + 1.0f);
        hal_writeFan(fanOn);

        telem.temp      = temp;
        telem.humidity  = hum;
        telem.setpoint  = pid.setpoint;
        telem.alarmTemp = alarmTemp;
        telem.pwm       = (uint8_t)pwm;
        telem.fan       = fanOn ? 1 : 0;
        telem.alarm     = tempAlarm ? 1 : 0;
        telem.uptime    = now / 1000;

        // Invia a BLE e WebSocket
        ble_server_send(telem);
        if (wifi_connected()) web_server_send(telem);
    }

    if ((now - tLastLog) >= 2000) {
        tLastLog = now;
        char buf[128];
        telemetry_serialize(telem, buf, sizeof(buf));
        Serial.print(buf);
    }
}
