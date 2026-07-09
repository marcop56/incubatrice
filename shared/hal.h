#pragma once

// ============================================================
//  HAL — Hardware Abstraction Layer
// ============================================================

float         hal_readTemp();
float         hal_readHumidity();
void          hal_writeOutput(float pwm);
void          hal_writeFan(bool on);
void          hal_updateSsr();    // aggiorna pin SSR e LED dal loop
void          hal_init();
unsigned long hal_millis();
