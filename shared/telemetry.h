#pragma once
#include "protocol.h"

// ============================================================
//  Telemetry helpers — serializzazione JSON
//  Usata su ESP32-S3 (invio) e CYD (ricezione)
//  Nessuna dipendenza esterna — sprintf/sscanf puri
// ============================================================

// Serializza Telemetry in JSON
// buf deve essere almeno 96 byte
// ritorna la lunghezza della stringa
int  telemetry_serialize(const Telemetry& t, char* buf, int buflen);

// Deserializza JSON in Telemetry
// ritorna true se ok
bool telemetry_parse(const char* json, Telemetry& out);

// Serializza Command in JSON
int  command_serialize(const Command& cmd, char* buf, int buflen);

// Deserializza Command da JSON
bool command_parse(const char* json, Command& out);
