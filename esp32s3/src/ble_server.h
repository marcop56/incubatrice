#pragma once
#include "../../shared/protocol.h"

// ============================================================
//  BLE Server — ESP32-S3
//  Nordic UART Service (NUS)
//  TX notify  : Telemetry -> CYD  (ogni PID_DT secondi)
//  RX write   : Command   <- CYD
// ============================================================

// Callback chiamata quando arriva un Command dal CYD
typedef void (*CommandCallback)(const Command& cmd);

void ble_server_init(const char* deviceName, CommandCallback cb);
void ble_server_send(const Telemetry& t);
bool ble_server_connected();
