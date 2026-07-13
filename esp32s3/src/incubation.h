#pragma once
#include <stdint.h>
#include <stdbool.h>

// ============================================================
//  Incubation — gestione ciclo di incubazione
//  Clock provvisorio: millis() — sostituire con DS3231
//  Cicli predefiniti salvati in NVS
// ============================================================

#define MAX_PHASES  4
#define MAX_CYCLES  8
#define CYCLE_NAME_LEN 20

// --- Fase del ciclo ---
struct IncubationPhase {
    uint8_t  days;           // durata in giorni
    float    tempSP;         // setpoint temperatura °C
    float    humSP;          // setpoint umidità %RH
    bool     turningOn;      // girauova attivo
    uint8_t  turnIntervalH;  // ore tra una girata e l'altra
};

// --- Ciclo completo ---
struct IncubationCycle {
    char             name[CYCLE_NAME_LEN];
    uint8_t          numPhases;
    IncubationPhase  phases[MAX_PHASES];
};

// --- Stato runtime ---
enum class IncubationState : uint8_t {
    IDLE,       // nessun ciclo attivo
    RUNNING,    // ciclo in corso
    PAUSED,     // in pausa
    COMPLETED   // ciclo completato
};

struct IncubationStatus {
    IncubationState state;
    uint8_t  cycleIndex;     // indice ciclo attivo
    uint8_t  currentPhase;   // fase corrente (0-based)
    uint32_t startTime;      // millis() o epoch RTC all'avvio
    uint32_t elapsedDays;    // giorni trascorsi
    uint32_t elapsedHours;   // ore trascorse nella fase
    bool     turnerPos;      // posizione girauova (false=A, true=B)
    uint32_t lastTurnTime;   // millis() ultima girata
};

// --- API ---
void incubation_init();
void incubation_loop();                          // chiamata dal loop

bool incubation_start(uint8_t cycleIndex);
void incubation_pause();
void incubation_resume();
void incubation_stop();

const IncubationStatus& incubation_status();
const IncubationCycle&  incubation_cycle(uint8_t index);
uint8_t                 incubation_numCycles();

// Callback — il modulo chiama queste funzioni quando servono
typedef void (*SetpointCallback)(float tempSP, float humSP);
typedef void (*TurnerCallback)(bool posB);   // false=pos A, true=pos B

void incubation_setCallbacks(SetpointCallback sp, TurnerCallback tr);
