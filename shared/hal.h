#pragma once

// ============================================================
//  HAL — Hardware Abstraction Layer
//  Interfaccia identica su simulatore, ESP32-S3 e CYD
// ============================================================

float         hal_readTemp();
float         hal_readHumidity();
void          hal_writeOutput(float pwm);
void          hal_writeFan(bool on);
void          hal_updateSsr();
void          hal_init();
unsigned long hal_millis();
