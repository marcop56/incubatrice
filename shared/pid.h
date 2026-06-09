#pragma once

// ============================================================
//  PidController — classe C++ pura, zero dipendenze hardware
//  Stesso codice su ESP32 (PlatformIO) e Mac (clang++)
// ============================================================

class PidController {
public:
    PidController(float kp, float ki, float kd,
                  float dt,
                  float outMin, float outMax, float outBias);

    // Calcola output PID dato il valore misurato
    // Chiama ogni dt secondi
    float compute(float measured);

    // Azzera stato interno (integrale, lastError)
    void  reset();

    // Parametri modificabili a runtime
    float Kp, Ki, Kd;
    float setpoint;

    // Lettura stato
    float getIntegral()    const { return _integral; }
    float getLastError()   const { return _lastError; }
    float getLastOutput()  const { return _lastOutput; }

private:
    float _dt;
    float _outMin, _outMax, _outBias;
    float _integral;
    float _lastError;
    float _lastOutput;

    // Quantizzazione misura prima del PID (simula risoluzione sensore)
    float quantize(float v, float step) const;

    // Clamp
    float clamp(float v, float lo, float hi) const;
};
