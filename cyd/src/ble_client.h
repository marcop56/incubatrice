#pragma once
#include "../../shared/protocol.h"

// ============================================================
//  BLE Client — CYD
//  Si connette all'S3 (Nordic UART Service)
//  RX notify : riceve Telemetry dall'S3
//  TX write  : invia Command all'S3
// ============================================================

typedef void (*TelemetryCallback)(const Telemetry& t);

void ble_client_init(const char* serverName, TelemetryCallback cb);
void ble_client_loop();
bool ble_client_connected();
void ble_client_send(const Command& cmd);
