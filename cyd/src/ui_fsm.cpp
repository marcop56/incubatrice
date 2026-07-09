#include "ui_fsm.h"
#include "ble_client.h"
#include <TFT_eSPI.h>

extern TFT_eSPI tft;

#define COL_BG      TFT_BLACK
#define COL_HDR     0x1F5A
#define COL_TEXT    TFT_WHITE
#define COL_GREEN   0x07E0
#define COL_RED     TFT_RED
#define COL_YELLOW  TFT_YELLOW
#define COL_GRAY    0x4208
#define COL_BTN     0x2104
#define COL_ORANGE  0xFD20

#define HDR_H   30
#define BTN_H   40
#define BTN_W   90

void UiFsm::begin() {
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(COL_BG);
    transition(UiState::MAIN);
}

void UiFsm::transition(UiState next) {
    _state = next;
    switch (_state) {
        case UiState::MAIN:     enterMain();     break;
        case UiState::SETTINGS: enterSettings(); break;
        case UiState::ALARM:    enterAlarm();    break;
        case UiState::INFO:     enterInfo();     break;
        default: break;
    }
}

void UiFsm::update(const Telemetry& data) {
    _last = data;
    if (data.alarm && _state != UiState::ALARM) {
        transition(UiState::ALARM);
        return;
    }
    if (!data.alarm && _state == UiState::ALARM) {
        transition(UiState::MAIN);
        return;
    }
    switch (_state) {
        case UiState::MAIN:     updateMain(data);     break;
        case UiState::SETTINGS: updateSettings(data); break;
        case UiState::ALARM:    updateAlarm(data);    break;
        case UiState::INFO:     updateInfo(data);     break;
        default: break;
    }
}

void UiFsm::handleTouch(int x, int y) {
    switch (_state) {
        case UiState::MAIN:     handleTouchMain(x, y);     break;
        case UiState::SETTINGS: handleTouchSettings(x, y); break;
        case UiState::ALARM:    handleTouchAlarm(x, y);    break;
        case UiState::INFO:     handleTouchInfo(x, y);     break;
        default: break;
    }
}

void UiFsm::drawHeader(const char* title) {
    tft.fillRect(0, 0, 320, HDR_H, COL_HDR);
    tft.setTextColor(COL_TEXT, COL_HDR);
    tft.setTextSize(2);
    tft.setCursor(8, 7);
    tft.print(title);
}

void UiFsm::drawButton(int x, int y, int w, int h, const char* label, uint16_t color) {
    tft.fillRoundRect(x, y, w, h, 6, color);
    tft.setTextColor(COL_TEXT, color);
    tft.setTextSize(1);
    int tw = strlen(label) * 6;
    tft.setCursor(x + (w - tw) / 2, y + (h - 8) / 2);
    tft.print(label);
}

// -----------------------------------------------------------
//  MAIN
// -----------------------------------------------------------
void UiFsm::enterMain() {
    tft.fillScreen(COL_BG);
    drawHeader("Incubatrice");
    tft.setTextColor(COL_GRAY, COL_BG);
    tft.setTextSize(1);
    tft.setCursor(8,  44); tft.print("Temperatura");
    tft.setCursor(8, 104); tft.print("Umidita'");
    tft.setCursor(8, 164); tft.print("PWM");
    drawButton(230,  40, BTN_W, BTN_H, "Settings", COL_BTN);
    drawButton(230, 100, BTN_W, BTN_H, "Info",     COL_BTN);
}

void UiFsm::updateMain(const Telemetry& d) {
    tft.setTextSize(4);
    tft.setTextColor(d.alarm ? COL_RED : COL_GREEN, COL_BG);
    tft.setCursor(8, 54);
    tft.printf("%.2f C  ", d.temp);

    tft.setTextSize(2);
    tft.setTextColor(COL_YELLOW, COL_BG);
    tft.setCursor(8, 90);
    tft.printf("SP:%.1f AL:%.1f   ", d.setpoint, d.alarmTemp);

    tft.setTextSize(3);
    tft.setTextColor(COL_TEXT, COL_BG);
    tft.setCursor(8, 114);
    tft.printf("%.1f %%  ", d.humidity);

    tft.setTextSize(1);
    tft.setTextColor(d.fan ? COL_GREEN : COL_GRAY, COL_BG);
    tft.setCursor(8, 148);
    tft.print(d.fan ? "FAN ON " : "FAN OFF");

    tft.setTextSize(2);
    tft.setTextColor(COL_TEXT, COL_BG);
    tft.setCursor(8, 172);
    tft.printf("PWM:%3d  ", d.pwm);
    int barW = map(d.pwm, 0, 255, 0, 200);
    tft.fillRect(8, 190, barW, 10, COL_GREEN);
    tft.fillRect(8 + barW, 190, 200 - barW, 10, COL_GRAY);
}

void UiFsm::handleTouchMain(int x, int y) {
    if (x > 230 && y > 40  && y < 80)  transition(UiState::SETTINGS);
    if (x > 230 && y > 100 && y < 140) transition(UiState::INFO);
}

// -----------------------------------------------------------
//  SETTINGS
// -----------------------------------------------------------
void UiFsm::enterSettings() {
    tft.fillScreen(COL_BG);
    drawHeader("Impostazioni");
    tft.setTextColor(COL_GRAY, COL_BG);
    tft.setTextSize(1);
    tft.setCursor(8,  38); tft.print("Setpoint temperatura");
    tft.setCursor(8,  88); tft.print("Limite allarme");
    tft.setCursor(8, 138); tft.print("Setpoint umidita'");
    drawButton( 10, 200, 80, BTN_H, "< Back",  COL_BTN);
    // Pulsanti SP temp
    drawButton(270, 44, 35, 28, "+", COL_GREEN);
    drawButton(230, 44, 35, 28, "-", COL_RED);
    // Pulsanti alarm temp
    drawButton(270, 94, 35, 28, "+", COL_ORANGE);
    drawButton(230, 94, 35, 28, "-", COL_ORANGE);
    // Pulsanti SP umidità
    drawButton(270,144, 35, 28, "+", COL_GREEN);
    drawButton(230,144, 35, 28, "-", COL_RED);
}

void UiFsm::updateSettings(const Telemetry& d) {
    tft.setTextSize(2);
    tft.setTextColor(COL_TEXT, COL_BG);
    tft.setCursor(8, 52);
    tft.printf("%.1f C    ", d.setpoint);
    tft.setCursor(8, 102);
    tft.setTextColor(COL_ORANGE, COL_BG);
    tft.printf("%.1f C    ", d.alarmTemp);
    tft.setTextColor(COL_TEXT, COL_BG);
    tft.setCursor(8, 152);
    tft.printf("58.0 %%  ");
}

void UiFsm::handleTouchSettings(int x, int y) {
    Serial.printf("touch settings x=%d y=%d\n", x, y);
    if (x > 10 && x < 90 && y > 200) {
        transition(UiState::MAIN);
        return;
    }
    if (!_sendCmd) return;
    Command cmd = {};

    // Setpoint temperatura
    if (x > 270 && y > 44 && y < 72) {
        cmd.type = CmdType::SET_SETPOINT;
        cmd.value = _last.setpoint + 0.5f;
        _sendCmd(cmd);
    }
    if (x > 230 && x < 265 && y > 44 && y < 72) {
        cmd.type = CmdType::SET_SETPOINT;
        cmd.value = _last.setpoint - 0.5f;
        _sendCmd(cmd);
    }

    // Limite allarme
    if (x > 270 && y > 94 && y < 122) {
    Serial.printf("alarm+ premuto, at=%.1f\n", _last.alarmTemp);
    cmd.type = CmdType::SET_ALARM_TEMP;
    cmd.value = _last.alarmTemp + 0.5f;
    _sendCmd(cmd);
}
    if (x > 230 && x < 265 && y > 94 && y < 122) {
        cmd.type = CmdType::SET_ALARM_TEMP;
        cmd.value = _last.alarmTemp - 0.5f;
        _sendCmd(cmd);
    }
}

// -----------------------------------------------------------
//  ALARM
// -----------------------------------------------------------
void UiFsm::enterAlarm() {
    tft.fillScreen(COL_RED);
    drawHeader("!! ALLARME !!");
    drawButton(80, 180, 160, BTN_H, "RESET ALLARME", COL_BTN);
}

void UiFsm::updateAlarm(const Telemetry& d) {
    tft.setTextSize(4);
    tft.setTextColor(COL_TEXT, COL_RED);
    tft.setCursor(40, 70);
    tft.printf("%.2f C", d.temp);
    tft.setTextSize(2);
    tft.setCursor(40, 130);
    tft.printf("Limite: %.1f C", d.alarmTemp);
}

void UiFsm::handleTouchAlarm(int x, int y) {
    if (x > 80 && x < 240 && y > 180) {
        if (_sendCmd) {
            Command cmd = {};
            cmd.type = CmdType::RESET_ALARM;
            _sendCmd(cmd);
        }
    }
}

// -----------------------------------------------------------
//  INFO
// -----------------------------------------------------------
void UiFsm::enterInfo() {
    tft.fillScreen(COL_BG);
    drawHeader("Info");
    drawButton(10, 200, 80, BTN_H, "< Back", COL_BTN);
}

void UiFsm::updateInfo(const Telemetry& d) {
    tft.setTextSize(1);
    tft.setTextColor(COL_TEXT, COL_BG);
    tft.setCursor(8, 44); tft.printf("Uptime: %lu s    ", d.uptime);
    tft.setCursor(8, 60);
    bool conn = ble_client_connected();
    tft.setTextColor(conn ? COL_GREEN : COL_RED, COL_BG);
    tft.printf("BLE: %s      ", conn ? "connesso" : "OFFLINE ");
    tft.setTextColor(COL_TEXT, COL_BG);
    tft.setCursor(8, 76); tft.printf("FW: v0.2            ");
    tft.setCursor(8, 92); tft.printf("AL limit: %.1f C    ", d.alarmTemp);
}

void UiFsm::handleTouchInfo(int x, int y) {
    if (x > 10 && x < 90 && y > 200) transition(UiState::MAIN);
}
