#include <cstdio>
#include <cmath>
<<<<<<< HEAD
#include <unistd.h>

#include "../shared/pid.h"
#include "../shared/hal.h"
#include "../shared/protocol.h"
#include "../shared/telemetry.h"

static const float SETPOINT   = 37.5f;
static const float KP         = 10.0f;
static const float KI         =  0.05f;
static const float KD         =  0.0f;
static const float PID_DT     =  1.0f;
static const float PWM_MIN    =  0.0f;
static const float PWM_MAX    = 255.0f;
static const float PWM_BIAS   = 128.0f;
static const float ALARM_TEMP = 40.0f;
static const float HUM_SP     = 58.0f;
=======
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
>>>>>>> a7d1cfe5efe32590e0df10c3c2501b4d83b302b3

int main() {
    hal_init();

    PidController pid(KP, KI, KD, PID_DT, PWM_MIN, PWM_MAX, PWM_BIAS);
    pid.setpoint = SETPOINT;

<<<<<<< HEAD
    printf("%-8s %-8s %-8s %-8s %-8s %-8s %-8s\n",
           "t(s)","T(C)","SET","ERR","PWM","H(%)","FAN");
=======
    printHeader();
>>>>>>> a7d1cfe5efe32590e0df10c3c2501b4d83b302b3

    unsigned long tStart   = hal_millis();
    unsigned long tLastPid = tStart;
    unsigned long tLastLog = tStart;
<<<<<<< HEAD
=======

>>>>>>> a7d1cfe5efe32590e0df10c3c2501b4d83b302b3
    bool alarm = false;

    while (true) {
        unsigned long now = hal_millis();

<<<<<<< HEAD
=======
        // PID ogni PID_DT secondi
>>>>>>> a7d1cfe5efe32590e0df10c3c2501b4d83b302b3
        if ((now - tLastPid) >= (unsigned long)(PID_DT * 1000)) {
            tLastPid = now;

            float temp = hal_readTemp();
<<<<<<< HEAD
            float hum  = hal_readHumidity();
            alarm = (temp >= ALARM_TEMP);

            float pwm;
            if (alarm) { pwm = 0.0f; pid.reset(); }
            else        { pwm = pid.compute(temp); }
            hal_writeOutput(pwm);

            bool fanOn = (hum > HUM_SP + 1.0f);
            hal_writeFan(fanOn);
        }

        if ((now - tLastLog) >= 2000) {
            tLastLog = now;
            float t    = (now - tStart) / 1000.0f;
            float temp = hal_readTemp();
            float hum  = hal_readHumidity();
            float pwm  = pid.getLastOutput();


            // Output JSON — compatibile col monitor HTML
            Telemetry telem;
            telem.temp     = temp;
            telem.humidity = hum;
            telem.setpoint = pid.setpoint;
            telem.pwm      = (uint8_t)pwm;
            telem.fan      = (hum > HUM_SP + 1.0f) ? 1 : 0;
            telem.alarm    = alarm ? 1 : 0;
            telem.uptime   = (uint32_t)t;

            char buf[96];
            telemetry_serialize(telem, buf, sizeof(buf));
            printf("%s", buf);
=======
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
>>>>>>> a7d1cfe5efe32590e0df10c3c2501b4d83b302b3

            if (alarm) printf("!! ALLARME TEMPERATURA !!\n");
        }

<<<<<<< HEAD
        usleep(100000);
    }
=======
        // Tick 100ms
        usleep(100000);
    }

>>>>>>> a7d1cfe5efe32590e0df10c3c2501b4d83b302b3
    return 0;
}
