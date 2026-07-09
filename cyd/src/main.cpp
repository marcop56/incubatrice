#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include "ble_client.h"
#include "ui_fsm.h"

// ============================================================
//  CYD main — loop veloce, niente delay fissi
//  Display aggiornato ogni 200ms
//  Touch polling ogni 50ms
//  BLE gestito in background
// ============================================================

#define TOUCH_CS    33
#define TOUCH_IRQ   36
#define TOUCH_MOSI  32
#define TOUCH_MISO  39
#define TOUCH_CLK   25
#define PIN_BL      21

#define INTERVAL_DISPLAY  200   // ms
#define INTERVAL_TOUCH     50   // ms
#define INTERVAL_BLE      500   // ms

SPIClass            touchSPI(HSPI);
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);
TFT_eSPI            tft;

UiFsm      ui;
Telemetry  lastTelem = {};

unsigned long tLastDisplay = 0;
unsigned long tLastTouch   = 0;
unsigned long tLastBle     = 0;

void onTelemetry(const Telemetry& t) {
    lastTelem = t;
}

void onSendCommand(const Command& cmd) {
    ble_client_send(cmd);
}

void setup() {
    Serial.begin(115200);

    pinMode(PIN_BL, OUTPUT);
    digitalWrite(PIN_BL, HIGH);

    touchSPI.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
    touch.begin(touchSPI);
    touch.setRotation(1);

    ble_client_init("Incubatrice", onTelemetry);

    ui.setSendCommand(onSendCommand);
    ui.begin();

    Serial.println("[CYD] pronto");
}

void loop() {
    unsigned long now = millis();

    // BLE — riconnessione ogni 500ms se persa
    if (now - tLastBle >= INTERVAL_BLE) {
        tLastBle = now;
        ble_client_loop();
    }

    // Touch — polling ogni 50ms
    if (now - tLastTouch >= INTERVAL_TOUCH) {
        tLastTouch = now;
        if (touch.tirqTouched() && touch.touched()) {
            TS_Point p = touch.getPoint();
            int x = constrain(map(p.x, 200, 3745, 0, 320), 0, 319);
            int y = constrain(map(p.y, 290, 3885, 0, 240), 0, 239);
            ui.handleTouch(x, y);
        }
    }

    // Display — aggiornamento ogni 200ms
    if (now - tLastDisplay >= INTERVAL_DISPLAY) {
        tLastDisplay = now;
        ui.update(lastTelem);
    }
}