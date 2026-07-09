#include "../../shared/hal.h"
#include <cstdio>
#include <chrono>
#include <cmath>

// ============================================================
//  HAL_SIM — modello termico + igrometrico primo ordine
//  Parametri validati con il simulatore JavaScript
// ============================================================

static const float TAMB     = 20.0f;
static const float PMAX     =  8.0f;
static const float PCOOL    =  4.0f;
static const float TAU      = 600.0f;
static const float PWM_BIAS = 128.0f;
static const float HAMB     = 40.0f;
static const float HTAU     = 300.0f;

static float _temp  = 20.0f;
static float _hum   = 40.0f;
static float _pwm   = PWM_BIAS;
static bool  _fan   = false;

static auto _t0    = std::chrono::steady_clock::now();
static auto _tLast = std::chrono::steady_clock::now();

void hal_init() {
    _temp = 20.0f; _hum = 40.0f; _pwm = PWM_BIAS; _fan = false;
    _t0 = _tLast = std::chrono::steady_clock::now();
    printf("[HAL_SIM] init — T=%.1f°C H=%.0f%% tau=%.0fs\n", _temp, _hum, TAU);
}

float hal_readTemp() {
    auto  now = std::chrono::steady_clock::now();
    float dt  = std::chrono::duration<float>(now - _tLast).count();
    _tLast    = now;

    float heatPwr = _pwm >= PWM_BIAS ? ((_pwm - PWM_BIAS) / PWM_BIAS) * PMAX  : 0.0f;
    float coolPwr = _pwm <  PWM_BIAS ? ((PWM_BIAS - _pwm) / PWM_BIAS) * PCOOL : 0.0f;
    float loss    = (_temp - TAMB) / TAU;
    _temp += (heatPwr - coolPwr - loss) * dt;
    if (_temp < 15.0f) _temp = 15.0f;
    if (_temp > 55.0f) _temp = 55.0f;

    float humTarget = _fan ? HAMB : 70.0f;
    _hum += (_hum - humTarget) / (-HTAU) * dt;
    if (_hum <   0.0f) _hum =   0.0f;
    if (_hum > 100.0f) _hum = 100.0f;

    return _temp;
}

float hal_readHumidity()        { return _hum; }
void  hal_writeOutput(float pwm){ _pwm = pwm; }
void  hal_writeFan(bool on)     { _fan = on; }

unsigned long hal_millis() {
    auto now = std::chrono::steady_clock::now();
    return (unsigned long)std::chrono::duration_cast<std::chrono::milliseconds>(now - _t0).count();
}
