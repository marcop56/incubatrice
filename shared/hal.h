#pragma once

// ============================================================
//  HAL — Hardware Abstraction Layer
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
unsigned long hal_millis();
