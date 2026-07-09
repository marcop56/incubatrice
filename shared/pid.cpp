#include "pid.h"
#include <cmath>

// ============================================================
//  PidController
//  outBias: punto di lavoro (es. 128 su scala 0-255)
//  anti-windup: integrale clampato a ±(outRange/Ki)
// ============================================================

PidController::PidController(float kp, float ki, float kd,
                             float dt,
                             float outMin, float outMax, float outBias)
    : Kp(kp), Ki(ki), Kd(kd),
      setpoint(0.0f),
      _dt(dt),
      _outMin(outMin), _outMax(outMax), _outBias(outBias),
      _integral(0.0f), _lastError(0.0f), _lastOutput(outBias)
{}

float PidController::compute(float measured) {
    // Quantizzazione 0.1°C — il D non amplifica rumore sub-decimale
    float mq = quantize(measured, 0.1f);

    float err      = setpoint - mq;
    float outRange = (_outMax - _outMin) / 2.0f;

    // Integrale con anti-windup
    _integral += err * _dt;
    _integral  = clamp(_integral,
                       -outRange / (Ki > 0.0f ? Ki : 1e-6f),
                       +outRange / (Ki > 0.0f ? Ki : 1e-6f));

    // Derivativo
    float deriv = (err - _lastError) / _dt;
    _lastError  = err;

    float out = _outBias + Kp * err + Ki * _integral + Kd * deriv;
    _lastOutput = clamp(out, _outMin, _outMax);
    return _lastOutput;
}

void PidController::reset() {
    _integral   = 0.0f;
    _lastError  = 0.0f;
    _lastOutput = _outBias;
}

float PidController::quantize(float v, float step) const {
    return std::round(v / step) * step;
}

float PidController::clamp(float v, float lo, float hi) const {
    return v < lo ? lo : (v > hi ? hi : v);
}
