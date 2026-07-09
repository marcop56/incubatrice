#include "ble_client.h"
#include "../../shared/telemetry.h"
#include <NimBLEDevice.h>

// ============================================================
//  BLE Client CYD — NimBLE-Arduino
//  Riconnessione su task FreeRTOS separato
//  Loop principale mai bloccato
// ============================================================

#define NUS_SERVICE_UUID  "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define NUS_TX_UUID       "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
#define NUS_RX_UUID       "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

static NimBLEClient*               _client    = nullptr;
static NimBLERemoteCharacteristic* _rxChar    = nullptr;
static volatile bool               _connected = false;
static volatile bool               _scanning  = false;
static TelemetryCallback           _telemCb   = nullptr;
static std::string                 _serverName;

// --- Notifica dati in arrivo dall'S3 ---
static void onNotify(NimBLERemoteCharacteristic* ch,
                     uint8_t* data, size_t len, bool isNotify) {
    if (!_telemCb) return;
    char buf[128];
    size_t n = len < sizeof(buf)-1 ? len : sizeof(buf)-1;
    memcpy(buf, data, n);
    buf[n] = '\0';
    Telemetry t;
    if (telemetry_parse(buf, t)) _telemCb(t);
}

// --- Callbacks connessione ---
class ClientCallbacks : public NimBLEClientCallbacks {
    void onDisconnect(NimBLEClient* c) override {
        _connected = false;
        _rxChar    = nullptr;
    }
};

static bool doConnect() {
    NimBLEScan* scan = NimBLEDevice::getScan();
    scan->setActiveScan(false);
    NimBLEScanResults results = scan->start(3, false);  // 3s max

    for (int i = 0; i < results.getCount(); i++) {
        NimBLEAdvertisedDevice dev = results.getDevice(i);
        if (dev.getName() != _serverName) continue;

        if (!_client) {
            _client = NimBLEDevice::createClient();
        //    _client->setCallbacks(new ClientCallbacks());
        _client->setClientCallbacks(new ClientCallbacks());
        }
        if (!_client->connect(&dev)) return false;

        NimBLERemoteService* svc = _client->getService(NUS_SERVICE_UUID);
        if (!svc) { _client->disconnect(); return false; }

        NimBLERemoteCharacteristic* txCh = svc->getCharacteristic(NUS_TX_UUID);
        if (!txCh) { _client->disconnect(); return false; }
        txCh->subscribe(true, onNotify);

        _rxChar = svc->getCharacteristic(NUS_RX_UUID);
        if (!_rxChar) { _client->disconnect(); return false; }

        _connected = true;
        return true;
    }
    return false;
}

// --- Task FreeRTOS per riconnessione ---
static void bleTask(void* param) {
    while (true) {
        if (!_connected) {
            _scanning = true;
            doConnect();
            _scanning = false;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));  // riprova ogni secondo
    }
}

void ble_client_init(const char* serverName, TelemetryCallback cb) {
    _serverName = serverName;
    _telemCb    = cb;
    NimBLEDevice::init("");

    // Task BLE su core 0, loop principale su core 1
    xTaskCreatePinnedToCore(bleTask, "BLE", 4096, nullptr, 1, nullptr, 0);
}

void ble_client_loop() {
    // Non fa più niente — la riconnessione è sul task
}

bool ble_client_connected() {
    return _connected;
}

void ble_client_send(const Command& cmd) {
    if (!_connected || !_rxChar) return;
    char buf[96];
    command_serialize(cmd, buf, sizeof(buf));
    _rxChar->writeValue((uint8_t*)buf, strlen(buf), false);
}