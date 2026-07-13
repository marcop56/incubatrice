#include "incubation.h"
#include <Arduino.h>
#include <Preferences.h>
#include <string.h>

// ============================================================
//  Incubation — implementazione
//  Clock: millis() — provvisorio, sostituire con DS3231
// ============================================================

// --- Cicli predefiniti ---
static const IncubationCycle _defaultCycles[] = {
    {
        "Gallina", 2, {
            { 18, 37.5f, 57.0f, true,  4 },   // incubazione
            {  3, 37.2f, 68.0f, false, 0 }    // lockdown
        }
    },
    {
        "Anatra", 2, {
            { 25, 37.5f, 55.0f, true,  4 },
            {  3, 37.2f, 70.0f, false, 0 }
        }
    },
    {
        "Quaglia", 2, {
            { 14, 37.5f, 55.0f, true,  4 },
            {  2, 37.2f, 68.0f, false, 0 }
        }
    },
    {
        "Tacchino", 2, {
            { 25, 37.5f, 55.0f, true,  4 },
            {  3, 37.2f, 68.0f, false, 0 }
        }
    },
    {
        "Fagiano", 2, {
            { 21, 37.5f, 55.0f, true,  4 },
            {  3, 37.2f, 68.0f, false, 0 }
        }
    }
};

static const uint8_t NUM_DEFAULT_CYCLES = sizeof(_defaultCycles) / sizeof(_defaultCycles[0]);

// --- Stato runtime ---
static IncubationStatus  _status = { IncubationState::IDLE };
static SetpointCallback  _spCb   = nullptr;
static TurnerCallback    _trCb   = nullptr;
static Preferences       _prefs;

// --- Costanti tempo ---
static const uint32_t MS_PER_HOUR = 3600000UL;
static const uint32_t MS_PER_DAY  = 86400000UL;

// --- Salva stato in NVS ---
static void saveStatus() {
    _prefs.begin("incubation", false);
    _prefs.putUChar("state",      (uint8_t)_status.state);
    _prefs.putUChar("cycleIdx",   _status.cycleIndex);
    _prefs.putUChar("phase",      _status.currentPhase);
    _prefs.putULong("startTime",  _status.startTime);
    _prefs.putBool ("turnerPos",  _status.turnerPos);
    _prefs.putULong("lastTurn",   _status.lastTurnTime);
    _prefs.end();
}

// --- Carica stato da NVS ---
static void loadStatus() {
    _prefs.begin("incubation", true);
    _status.state         = (IncubationState)_prefs.getUChar("state", (uint8_t)IncubationState::IDLE);
    _status.cycleIndex    = _prefs.getUChar("cycleIdx",  0);
    _status.currentPhase  = _prefs.getUChar("phase",     0);
    _status.startTime     = _prefs.getULong("startTime", 0);
    _status.turnerPos     = _prefs.getBool ("turnerPos", false);
    _status.lastTurnTime  = _prefs.getULong("lastTurn",  0);
    _prefs.end();

    if (_status.state == IncubationState::RUNNING) {
        Serial.printf("[INC] ripreso ciclo %d fase %d\n",
                      _status.cycleIndex, _status.currentPhase);
    }
}

// --- Applica setpoint della fase corrente ---
static void applyPhase() {
    const IncubationCycle& cyc   = _defaultCycles[_status.cycleIndex];
    const IncubationPhase& phase = cyc.phases[_status.currentPhase];
    if (_spCb) _spCb(phase.tempSP, phase.humSP);
    Serial.printf("[INC] fase %d — T=%.1f H=%.0f girauova=%s\n",
                  _status.currentPhase, phase.tempSP, phase.humSP,
                  phase.turningOn ? "ON" : "OFF");
}

void incubation_init() {
    loadStatus();
    if (_status.state == IncubationState::RUNNING) {
        applyPhase();
    }
}

bool incubation_start(uint8_t cycleIndex) {
    if (cycleIndex >= NUM_DEFAULT_CYCLES) return false;
    _status.state        = IncubationState::RUNNING;
    _status.cycleIndex   = cycleIndex;
    _status.currentPhase = 0;
    _status.startTime    = millis();
    _status.elapsedDays  = 0;
    _status.elapsedHours = 0;
    _status.turnerPos    = false;
    _status.lastTurnTime = millis();
    saveStatus();
    applyPhase();
    Serial.printf("[INC] avviato ciclo: %s\n", _defaultCycles[cycleIndex].name);
    return true;
}

void incubation_pause() {
    if (_status.state == IncubationState::RUNNING) {
        _status.state = IncubationState::PAUSED;
        saveStatus();
    }
}

void incubation_resume() {
    if (_status.state == IncubationState::PAUSED) {
        _status.state = IncubationState::RUNNING;
        saveStatus();
        applyPhase();
    }
}

void incubation_stop() {
    _status.state = IncubationState::IDLE;
    saveStatus();
    Serial.println("[INC] ciclo fermato");
}

void incubation_loop() {
    if (_status.state != IncubationState::RUNNING) return;

    const IncubationCycle& cyc   = _defaultCycles[_status.cycleIndex];
    const IncubationPhase& phase = cyc.phases[_status.currentPhase];

    unsigned long now = millis();

    // Calcola giorni ed ore trascorse nella fase corrente
    uint32_t phaseStart = _status.startTime;
    for (uint8_t i = 0; i < _status.currentPhase; i++) {
        phaseStart += (uint32_t)cyc.phases[i].days * MS_PER_DAY;
    }
    uint32_t phaseElapsed = now - phaseStart;
    _status.elapsedDays  = phaseElapsed / MS_PER_DAY;
    _status.elapsedHours = (phaseElapsed % MS_PER_DAY) / MS_PER_HOUR;

    // Avanzamento fase
    if (_status.elapsedDays >= phase.days) {
        _status.currentPhase++;
        if (_status.currentPhase >= cyc.numPhases) {
            _status.state = IncubationState::COMPLETED;
            saveStatus();
            Serial.println("[INC] ciclo completato!");
            return;
        }
        saveStatus();
        applyPhase();
        return;
    }

    // Girauova
    if (phase.turningOn && phase.turnIntervalH > 0) {
        uint32_t interval = (uint32_t)phase.turnIntervalH * MS_PER_HOUR;
        if ((now - _status.lastTurnTime) >= interval) {
            _status.turnerPos    = !_status.turnerPos;
            _status.lastTurnTime = now;
            saveStatus();
            if (_trCb) _trCb(_status.turnerPos);
            Serial.printf("[INC] girata — pos %s\n", _status.turnerPos ? "B" : "A");
        }
    }
}

void incubation_setCallbacks(SetpointCallback sp, TurnerCallback tr) {
    _spCb = sp;
    _trCb = tr;
}

const IncubationStatus& incubation_status()            { return _status; }
const IncubationCycle&  incubation_cycle(uint8_t i)    { return _defaultCycles[i]; }
uint8_t                 incubation_numCycles()          { return NUM_DEFAULT_CYCLES; }
