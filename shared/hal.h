#pragma once

// ============================================================
//  HAL — Hardware Abstraction Layer
<<<<<<< HEAD
// ============================================================

float         hal_readTemp();
float         hal_readHumidity();
void          hal_writeOutput(float pwm);
void          hal_writeFan(bool on);
void          hal_updateSsr();    // aggiorna pin SSR e LED dal loop
void          hal_init();
=======
//  Due funzioni: leggi temperatura, scrivi output
//  Implementazione diversa per sim e ESP32, interfaccia identica
// ============================================================

// Legge la temperatura in °C
// - Sim:   restituisce temperatura dal modello termico
// - ESP32: legge TMP117 via I2C
float hal_readTemp();

// Scrive l'output PWM (0.0 - 255.0)
// - Sim:   alimenta il modello termico
// - ESP32: ledcWrite() su GPIO 26
void  hal_writeOutput(float pwm);

// Inizializzazione hardware (sensore, PWM, I2C...)
void  hal_init();

// Tempo in millisecondi (millis() su ESP32, clock su Mac)
>>>>>>> a7d1cfe5efe32590e0df10c3c2501b4d83b302b3
unsigned long hal_millis();
