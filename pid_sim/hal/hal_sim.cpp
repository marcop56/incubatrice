#include "hal.h"
#include <cstdio>
#include <chrono>

// ============================================================
//  HAL_SIM — modello termico primo ordine
//  Stessi parametri del simulatore JavaScript validato
// ============================================================

// Parametri modello termico
static const float TAMB    = 20.0f;   // temperatura ambiente °C
static const float PMAX    =  8.0f;   // W riscaldatore
static const float PCOOL   =  4.0f;   // W raffreddatore
static const float TAU     = 600.0f;  // costante di tempo termica (secondi)
static const float PWM_BIAS= 128.0f;

// Stato modello
static float _temp    = 20.0f;  // temperatura simulata
static float _pwm     = 128.0f; // output corrente
static float _disturb = 0.0f;   // disturbo esterno °C/s

// Timer
static auto _t0 = std::chrono::steady_clock::now();
static auto _tLast = std::chrono::steady_clock::now();

// Imposta disturbo termico (test apertura coperchio)
void hal_sim_setDisturb(float dCps) { _disturb = dCps; }
float hal_sim_getTemp()             { return _temp; }

void hal_init() {
    _temp  = 20.0f;
    _pwm   = PWM_BIAS;
    _t0    = std::chrono::steady_clock::now();
    _tLast = _t0;
    printf("[HAL_SIM] init — T=%.1f°C tau=%.0fs\n", _temp, TAU);
}

float hal_readTemp() {
    // Aggiorna modello termico con dt reale trascorso
    auto now  = std::chrono::steady_clock::now();
    float dt  = std::chrono::duration<float>(now - _tLast).count();
    _tLast    = now;

    float heatPwr = _pwm >= PWM_BIAS ? ((_pwm - PWM_BIAS) / PWM_BIAS) * PMAX  : 0.0f;
    float coolPwr = _pwm <  PWM_BIAS ? ((PWM_BIAS - _pwm) / PWM_BIAS) * PCOOL : 0.0f;
    float loss    = (_temp - TAMB) / TAU;
    float dT      = (heatPwr - coolPwr - loss) * dt + _disturb * dt;

    _temp += dT;
    if (_temp < 15.0f) _temp = 15.0f;
    if (_temp > 45.0f) _temp = 45.0f;

    return _temp;
}

void hal_writeOutput(float pwm) {
    _pwm = pwm;
}

unsigned long hal_millis() {
    auto now = std::chrono::steady_clock::now();
    return (unsigned long)std::chrono::duration_cast<std::chrono::milliseconds>(now - _t0).count();
}
