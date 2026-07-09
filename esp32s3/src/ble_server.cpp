#include "ble_server.h"
#include "../../shared/telemetry.h"
#include <NimBLEDevice.h>

// ============================================================
//  BLE Server — Nordic UART Service (NUS)
//  Libreria: NimBLE-Arduino
//  UUID standard NUS
// ============================================================

#define NUS_SERVICE_UUID  "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define NUS_TX_UUID       "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  // notify S3->CYD
#define NUS_RX_UUID       "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  // write  CYD->S3

static NimBLECharacteristic* _txChar = nullptr;
static bool                  _connected = false;
static CommandCallback       _cmdCb = nullptr;

// --- Callbacks connessione ---
class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* srv) override {
        _connected = true;
    }
    void onDisconnect(NimBLEServer* srv) override {
        _connected = false;
        NimBLEDevice::startAdvertising();
    }
};

// --- Callbacks ricezione dati ---
class RxCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* ch) override {
        std::string val = ch->getValue();
        if (val.empty() || !_cmdCb) return;
        Command cmd;
        if (command_parse(val.c_str(), cmd)) {
            _cmdCb(cmd);
        }
    }
};

void ble_server_init(const char* deviceName, CommandCallback cb) {
    _cmdCb = cb;

    NimBLEDevice::init(deviceName);
    NimBLEServer* server = NimBLEDevice::createServer();
    server->setCallbacks(new ServerCallbacks());

    NimBLEService* svc = server->createService(NUS_SERVICE_UUID);

    // TX — notify verso CYD
    _txChar = svc->createCharacteristic(
        NUS_TX_UUID,
        NIMBLE_PROPERTY::NOTIFY
    );

    // RX — write dal CYD
    NimBLECharacteristic* rxChar = svc->createCharacteristic(
        NUS_RX_UUID,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    rxChar->setCallbacks(new RxCallbacks());

    svc->start();
    NimBLEDevice::startAdvertising();
}

void ble_server_send(const Telemetry& t) {
    if (!_connected || !_txChar) return;
    char buf[96];
    telemetry_serialize(t, buf, sizeof(buf));
    _txChar->setValue((uint8_t*)buf, strlen(buf));
    _txChar->notify();
}

bool ble_server_connected() {
    return _connected;
}
