#include <Arduino.h>
#pragma once

// ============================================================
//  WiFi Manager — ESP32-S3
//  Al primo avvio: AP "Incubatrice-Setup" per configurazione
//  Credenziali salvate in NVS da WiFiManager
//  Connessione automatica ai boot successivi
// ============================================================

void wifi_init();
bool wifi_connected();
String wifi_ip();
