#include <cstdio>
#include <cmath>
#include <unistd.h>   // usleep

#include "pid.h"
#include "hal.h"

// ============================================================
//  Configurazione PID — stessi valori validati in simulazione
// ============================================================
static const float SETPOINT     = 37.5f;
static const float KP           = 10.0f;
static const float KI           =  0.05f;
static const float KD           =  0.0f;
static const float PID_DT       =  1.0f;   // secondi
static const float PWM_MIN      =  0.0f;
static const float PWM_MAX      = 255.0f;
static const float PWM_BIAS     = 128.0f;
static const float ALARM_TEMP   = 40.0f;

// Stampa header CSV
static void printHeader() {
    printf("%-8s %-8s %-8s %-8s %-8s %-8s\n",
           "t(s)", "T(°C)", "SET", "ERR", "PWM", "CANALE");
    printf("%-8s %-8s %-8s %-8s %-8s %-8s\n",
           "--------","--------","--------","--------","--------","--------");
}

int main() {
    hal_init();

    PidController pid(KP, KI, KD, PID_DT, PWM_MIN, PWM_MAX, PWM_BIAS);
    pid.setpoint = SETPOINT;

    printHeader();

    unsigned long tStart   = hal_millis();
    unsigned long tLastPid = tStart;
    unsigned long tLastLog = tStart;

    bool alarm = false;

    while (true) {
        unsigned long now = hal_millis();

        // PID ogni PID_DT secondi
        if ((now - tLastPid) >= (unsigned long)(PID_DT * 1000)) {
            tLastPid = now;

            float temp = hal_readTemp();
            alarm = (temp >= ALARM_TEMP);

            float pwm;
            if (alarm) {
                pwm = 0.0f;   // spegni tutto
                pid.reset();
            } else {
                pwm = pid.compute(temp);
            }
            hal_writeOutput(pwm);
        }

        // Log ogni 2 secondi
        if ((now - tLastLog) >= 2000) {
            tLastLog = now;
            float t   = (now - tStart) / 1000.0f;
            float temp = hal_readTemp();
            float pwm  = pid.getLastOutput();
            float err  = pid.setpoint - temp;
            const char* chan = alarm       ? "ALLARME" :
                               pwm >= 128  ? "CALDO"   : "FREDDO";

            printf("%-8.1f %-8.2f %-8.1f %-8.2f %-8.0f %-8s\n",
                   t, temp, pid.setpoint, err, pwm, chan);

            if (alarm) printf("!! ALLARME TEMPERATURA !!\n");
        }

        // Tick 100ms
        usleep(100000);
    }

    return 0;
}
