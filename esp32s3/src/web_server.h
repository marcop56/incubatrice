#pragma once
#include "../../shared/protocol.h"

// ============================================================
//  Web Server — AsyncWebServer + WebSocket
//  Telemetria via WebSocket push ogni secondo
//  Comandi via WebSocket dal browser
// ============================================================

typedef void (*WebCommandCallback)(const Command& cmd);

void web_server_init(WebCommandCallback cb);
void web_server_send(const Telemetry& t);
