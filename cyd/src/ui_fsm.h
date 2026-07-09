#pragma once
#include "../../shared/protocol.h"

// ============================================================
//  UI FSM — Macchina a stati MMI per CYD
//  Stati: INIT -> MAIN <-> SETTINGS / ALARM / INFO
// ============================================================

enum class UiState : uint8_t {
    INIT,
    MAIN,
    SETTINGS,
    ALARM,
    INFO
};

class UiFsm {
public:
    void begin();
    void update(const Telemetry& data);
    void handleTouch(int x, int y);

    UiState state() const { return _state; }

    // Callback per inviare comandi verso S3
    typedef void (*SendCommandFn)(const Command&);
    void setSendCommand(SendCommandFn fn) { _sendCmd = fn; }

private:
    UiState       _state   = UiState::INIT;
    Telemetry     _last    = {};
    SendCommandFn _sendCmd = nullptr;

    void transition(UiState next);

    // --- MAIN ---
    void enterMain();
    void updateMain(const Telemetry& d);
    void handleTouchMain(int x, int y);

    // --- SETTINGS ---
    void enterSettings();
    void updateSettings(const Telemetry& d);
    void handleTouchSettings(int x, int y);

    // --- ALARM ---
    void enterAlarm();
    void updateAlarm(const Telemetry& d);
    void handleTouchAlarm(int x, int y);

    // --- INFO ---
    void enterInfo();
    void updateInfo(const Telemetry& d);
    void handleTouchInfo(int x, int y);

    // Helper display
    void drawHeader(const char* title);
    void drawButton(int x, int y, int w, int h, const char* label, uint16_t color);
};
