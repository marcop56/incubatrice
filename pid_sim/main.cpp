#include <cstdio>
#include <cmath>
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

int main() {
    hal_init();

    PidController pid(KP, KI, KD, PID_DT, PWM_MIN, PWM_MAX, PWM_BIAS);
    pid.setpoint = SETPOINT;

    printf("%-8s %-8s %-8s %-8s %-8s %-8s %-8s\n",
           "t(s)","T(C)","SET","ERR","PWM","H(%)","FAN");

    unsigned long tStart   = hal_millis();
    unsigned long tLastPid = tStart;
    unsigned long tLastLog = tStart;
    bool alarm = false;

    while (true) {
        unsigned long now = hal_millis();

        if ((now - tLastPid) >= (unsigned long)(PID_DT * 1000)) {
            tLastPid = now;

            float temp = hal_readTemp();
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

            if (alarm) printf("!! ALLARME TEMPERATURA !!\n");
        }

        usleep(100000);
    }
    return 0;
}
