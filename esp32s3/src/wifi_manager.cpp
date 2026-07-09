#include "wifi_manager.h"
#include <WiFiManager.h>
#include <Arduino.h>

// ============================================================
//  WiFi Manager
// ============================================================

static bool _connected = false;

void wifi_init() {
    WiFiManager wm;

    // Timeout AP: 3 minuti, poi continua senza WiFi
    wm.setConfigPortalTimeout(180);

    // LED NeoPixel come indicatore — blu durante config AP
    neopixelWrite(48, 0, 0, 64);

    Serial.println("[WiFi] avvio WiFiManager...");

    bool ok = wm.autoConnect("Incubatrice-Setup");

    if (ok) {
        _connected = true;
        Serial.printf("[WiFi] connesso — IP: %s\n", WiFi.localIP().toString().c_str());
        neopixelWrite(48, 0, 32, 0);   // verde = connesso
        delay(1000);
        neopixelWrite(48, 0, 0, 0);    // spento
    } else {
        _connected = false;
        Serial.println("[WiFi] timeout — continuo senza WiFi");
        neopixelWrite(48, 0, 0, 0);
    }
}

bool wifi_connected() { return _connected && WiFi.status() == WL_CONNECTED; }
String wifi_ip()      { return WiFi.localIP().toString(); }
