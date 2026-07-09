# Incubatrice ESP32

Incubatrice digitale per uova basata su ESP32-S3 con controllo PID temperatura,
umidità, girauova automatico e interfaccia web/BLE.

## Architettura

```
ESP32-S3 (master)
  ├── TMP117       — temperatura (I2C GPIO 8/9)
  ├── DHT22        — umidità (provvisorio, GPIO 10)
  ├── DS3231       — RTC con batteria CR2032 (I2C GPIO 8/9, da aggiungere)
  ├── SSR PWM      — resistenza riscaldante (GPIO 26)
  ├── Ventola      — controllo umidità on/off (GPIO 11)
  ├── Servo RC     — girauova (GPIO 12, da aggiungere)
  ├── BLE NUS      — telemetria JSON verso CYD
  └── WiFi/WebSocket — web UI su browser

CYD ESP32-2432S028 (display locale)
  ├── TFT 320x240 touch
  └── BLE NUS client
```

## Struttura progetto

```
incubatrice/
├── shared/          — HAL, PID, protocollo (condiviso tra target)
├── esp32s3/         — firmware ESP32-S3
├── cyd/             — firmware CYD display
└── pid_sim/         — simulatore PID su Mac/Linux
```

## Build simulatore (Mac/Linux)

```bash
cd pid_sim
make
./pid_sim
```

## Build firmware (PlatformIO)

Aprire `esp32s3/` o `cyd/` in VS Code con estensione PlatformIO.

## Web UI

Al primo avvio l'S3 crea un AP WiFi **"Incubatrice-Setup"**.
Connettersi e inserire le credenziali WiFi di casa.
Poi aprire `http://<IP>` nel browser.

## BLE

Compatibile con **nRF Connect** (iOS/Android) per debug.
Servizio: Nordic UART Service (NUS).

## Pinout ESP32-S3

| GPIO | Funzione        |
|------|-----------------|
| 8    | I2C SDA         |
| 9    | I2C SCL         |
| 10   | DHT22           |
| 11   | Ventola umidità |
| 12   | Servo girauova  |
| 13   | DS3231 INT      |
| 26   | SSR PWM         |
| 48   | LED NeoPixel    |

## Ciclo incubazione (uova gallina)

| Fase        | Giorni | Temp   | Umidità | Girauova  |
|-------------|--------|--------|---------|-----------|
| Incubazione | 1-18   | 37.5°C | 55-60%  | ogni 2-4h |
| Lockdown    | 19-21  | 37.2°C | 65-70%  | fermo     |

## Comandi utili

```bash
# Build simulatore
cd pid_sim && make && ./pid_sim

# Erase flash ESP32-S3
pio run -t erase -e esp32s3

# Git push
git add . && git commit -m "descrizione" && git push
```
