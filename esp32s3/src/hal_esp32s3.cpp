#include "../../shared/hal.h"
#include <Arduino.h>
#include <Wire.h>
#include <DHT.h>
#include "esp_timer.h"
#include "freertos/portmacro.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ============================================================
//  HAL ESP32-S3 — hardware reale
//
//  Core 0:
//    - Task sensori: legge TMP117 e DHT22 in background
//    - Timer SSR: aggiorna flag _ssrOn ogni 100ms
//
//  Core 1 (loop):
//    - hal_readTemp()     — legge _lastTemp (volatile)
//    - hal_readHumidity() — legge _lastHum  (volatile)
//    - hal_updateSsr()    — aggiorna GPIO e LED
//    - hal_writeOutput()  — aggiorna _ssrPwm
// ============================================================

#define TMP117_ADDR     0x48
#define TMP117_REG_TEMP 0x00
#define TMP117_REG_CFG  0x01
#define TMP117_REG_OFF  0x07

#define PIN_SDA         8
#define PIN_SCL         9
#define PIN_SSR         26
#define PIN_FAN         11
#define PIN_DHT         10
#define PIN_LED         48
#define DHT_TYPE        DHT22

#define SSR_TICKS       100
#define SSR_TICK_MS     100000  // 100ms in microsecondi

// --- Valori sensori condivisi ---
static volatile float    _lastTemp = -999.0f;
static volatile float    _lastHum  = -999.0f;
static portMUX_TYPE      _sensMux  = portMUX_INITIALIZER_UNLOCKED;

// --- SSR ---
static volatile uint8_t  _ssrPwm  = 0;
static volatile uint32_t _ssrTick = 0;
static volatile bool     _ssrOn   = false;
static portMUX_TYPE      _ssrMux  = portMUX_INITIALIZER_UNLOCKED;

static DHT dht(PIN_DHT, DHT_TYPE);
static esp_timer_handle_t _ssrTimer = nullptr;

// --- Timer SSR — IRAM_ATTR, core 0 ---
static void IRAM_ATTR ssrTimerCb(void* arg) {
    portENTER_CRITICAL_ISR(&_ssrMux);
    uint32_t tick   = _ssrTick % SSR_TICKS;
    uint32_t onTick = (_ssrPwm * SSR_TICKS) / 255;
    _ssrOn  = (tick < onTick);
    _ssrTick++;
    portEXIT_CRITICAL_ISR(&_ssrMux);
}

// --- Configurazione TMP117 ---
static void tmp117_configure() {
    Wire.beginTransmission(TMP117_ADDR);
    Wire.write(TMP117_REG_CFG);
    Wire.write(0x00);
    Wire.write(0x20);   // AVG 32 campioni
    Wire.endTransmission(true);
    delay(10);

    Wire.beginTransmission(TMP117_ADDR);
    Wire.write(TMP117_REG_OFF);
    Wire.write(0xFF);
    Wire.write(0xF3);   // offset -13 LSB = -0.1016°C
    Wire.endTransmission(true);
    delay(10);
}

// --- Lettura TMP117 — bloccante, chiamata dal task ---
static float tmp117_read() {
    // Aspetta Data_Ready (bit 13 reg 0x01)
    uint16_t cfg = 0;
    int tentativi = 0;
    do {
        Wire.beginTransmission(TMP117_ADDR);
        Wire.write(TMP117_REG_CFG);
        Wire.endTransmission(true);
        vTaskDelay(pdMS_TO_TICKS(10));
        Wire.requestFrom(TMP117_ADDR, 2);
        if (Wire.available() == 2) {
            cfg = (Wire.read() << 8) | Wire.read();
        }
        if (++tentativi > 150) return -999.0f;  // timeout 1.5s
    } while (!(cfg & (1 << 13)));

    Wire.beginTransmission(TMP117_ADDR);
    Wire.write(TMP117_REG_TEMP);
    Wire.endTransmission(true);
    Wire.requestFrom(TMP117_ADDR, 2);
    if (Wire.available() == 2) {
        int16_t raw = (Wire.read() << 8) | Wire.read();
        return raw * 0.0078125f;
    }
    return -999.0f;
}

// --- Task sensori su core 0 ---
static void sensorTask(void* arg) {
    while (true) {
        // Leggi TMP117
        float t = tmp117_read();
        if (t > -900.0f) {
            portENTER_CRITICAL(&_sensMux);
            _lastTemp = t;
            portEXIT_CRITICAL(&_sensMux);
        }

        // Leggi DHT22 ogni 2s (sensore lento)
        static int dhCount = 0;
        if (++dhCount >= 2) {
            dhCount = 0;
            float h = dht.readHumidity();
            if (!isnan(h)) {
                portENTER_CRITICAL(&_sensMux);
                _lastHum = h;
                portEXIT_CRITICAL(&_sensMux);
            }
        }
        // Nessun delay — il task si blocca dentro tmp117_read() su vTaskDelay
    }
}

// --- Aggiorna SSR e LED — loop core 1 ---
void hal_updateSsr() {
    static bool          lastOn      = false;
    static unsigned long tLastChange = 0;

    bool on;
    portENTER_CRITICAL(&_ssrMux);
    on = _ssrOn;
    portEXIT_CRITICAL(&_ssrMux);

    unsigned long now = millis();
    if (on != lastOn && (now - tLastChange) > 50) {
        lastOn      = on;
        tLastChange = now;
        digitalWrite(PIN_SSR, on ? HIGH : LOW);
        neopixelWrite(PIN_LED, on ? 128 : 0, 0, 0);
    }
}

void hal_init() {
    Wire.begin(PIN_SDA, PIN_SCL);
    delay(100);
    tmp117_configure();

    dht.begin();

    pinMode(PIN_SSR, OUTPUT);
    digitalWrite(PIN_SSR, LOW);

    pinMode(PIN_FAN, OUTPUT);
    digitalWrite(PIN_FAN, LOW);

    neopixelWrite(PIN_LED, 0, 0, 0);

    // Timer SSR su core 0
    esp_timer_create_args_t args = {};
    args.callback = ssrTimerCb;
    args.name     = "ssr_pwm";
    esp_timer_create(&args, &_ssrTimer);
    esp_timer_start_periodic(_ssrTimer, SSR_TICK_MS);

    // Task sensori su core 0, stack 4KB
    xTaskCreatePinnedToCore(sensorTask, "sensors", 4096, nullptr, 2, nullptr, 0);

    Serial.println("[HAL] init OK — sensori su core 0, SSR timer 100ms");
}

float hal_readTemp() {
    portENTER_CRITICAL(&_sensMux);
    float t = _lastTemp;
    portEXIT_CRITICAL(&_sensMux);
    return t;
}

float hal_readHumidity() {
    portENTER_CRITICAL(&_sensMux);
    float h = _lastHum;
    portEXIT_CRITICAL(&_sensMux);
    return h;
}

void hal_writeOutput(float pwm) {
    int v = (int)pwm;
    if (v < 0)   v = 0;
    if (v > 255) v = 255;
    portENTER_CRITICAL(&_ssrMux);
    _ssrPwm = (uint8_t)v;
    portEXIT_CRITICAL(&_ssrMux);
}

void hal_writeFan(bool on) {
    digitalWrite(PIN_FAN, on ? HIGH : LOW);
}

unsigned long hal_millis() {
    return millis();
}