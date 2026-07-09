#include "web_server.h"
#include "../../shared/telemetry.h"
#include <ESPAsyncWebServer.h>
#include <Arduino.h>

// ============================================================
//  Web Server — ESP32-S3
//  Pagina HTML inline (no SPIFFS)
//  WebSocket su /ws
// ============================================================

static AsyncWebServer  _server(80);
static AsyncWebSocket  _ws("/ws");
static WebCommandCallback _cmdCb = nullptr;

// --- Pagina HTML inline ---
static const char HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="it">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Incubatrice</title>
<script src="https://cdnjs.cloudflare.com/ajax/libs/Chart.js/4.4.1/chart.umd.js"></script>
<style>
* { box-sizing: border-box; margin: 0; padding: 0; }
body { font-family: sans-serif; background: #1a1a1a; color: #eee; padding: 1rem; }
h2 { font-weight: 500; margin-bottom: 1rem; }
.grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(120px, 1fr)); gap: 10px; margin-bottom: 1rem; }
.metric { background: #2a2a2a; border-radius: 8px; padding: 0.75rem; }
.metric-label { font-size: 12px; color: #888; margin-bottom: 4px; }
.metric-value { font-size: 22px; font-weight: 500; }
.alarm { color: #e55; }
.chart-wrap { position: relative; height: 200px; margin-bottom: 1rem; }
.controls { background: #2a2a2a; border-radius: 8px; padding: 1rem; margin-bottom: 1rem; }
.ctrl-row { display: flex; align-items: center; gap: 10px; margin-bottom: 8px; }
.ctrl-row label { width: 160px; font-size: 13px; color: #aaa; }
.ctrl-row input[type=number] { background: #333; border: 1px solid #555; color: #eee;
  padding: 4px 8px; border-radius: 4px; width: 80px; }
.btn { background: #333; color: #eee; border: 1px solid #555; padding: 6px 16px;
  border-radius: 6px; cursor: pointer; font-size: 14px; }
.btn:hover { background: #444; }
.btn-red { border-color: #e55; color: #e55; }
.btn-green { border-color: #5e5; color: #5e5; }
.status { font-size: 12px; color: #888; margin-bottom: 1rem; }
#dot { display:inline-block; width:8px; height:8px; border-radius:50%; background:#e55; margin-right:4px; }
#dot.on { background:#5e5; }
</style>
</head>
<body>
<h2>🥚 Incubatrice</h2>
<div class="status"><span id="dot"></span><span id="connStatus">Disconnesso</span> &nbsp;|&nbsp; IP: <span id="ipAddr">--</span></div>

<div class="grid">
  <div class="metric"><div class="metric-label">Temperatura</div><div class="metric-value" id="mT">--</div></div>
  <div class="metric"><div class="metric-label">Setpoint</div><div class="metric-value" id="mSP">--</div></div>
  <div class="metric"><div class="metric-label">Soglia allarme</div><div class="metric-value" id="mAT">--</div></div>
  <div class="metric"><div class="metric-label">Umidità</div><div class="metric-value" id="mH">--</div></div>
  <div class="metric"><div class="metric-label">PWM</div><div class="metric-value" id="mPWM">--</div></div>
  <div class="metric"><div class="metric-label">Ventola</div><div class="metric-value" id="mFan">--</div></div>
  <div class="metric"><div class="metric-label">Uptime</div><div class="metric-value" id="mUp">--</div></div>
  <div class="metric"><div class="metric-label">Allarme</div><div class="metric-value" id="mAl">--</div></div>
</div>

<div class="chart-wrap"><canvas id="cTemp"></canvas></div>
<div class="chart-wrap"><canvas id="cPWM"></canvas></div>

<div class="controls">
  <div class="ctrl-row">
    <label>Setpoint temp (°C)</label>
    <input type="number" id="inSP" step="0.5" min="25" max="42" value="37.5">
    <button class="btn btn-green" onclick="sendCmd(1, +document.getElementById('inSP').value)">Set</button>
  </div>
  <div class="ctrl-row">
    <label>Soglia allarme (°C)</label>
    <input type="number" id="inAT" step="0.5" min="35" max="45" value="40.0">
    <button class="btn btn-green" onclick="sendCmd(5, +document.getElementById('inAT').value)">Set</button>
  </div>
  <div class="ctrl-row">
    <label>Setpoint umidità (%)</label>
    <input type="number" id="inHS" step="1" min="40" max="80" value="58">
    <button class="btn btn-green" onclick="sendCmd(2, +document.getElementById('inHS').value)">Set</button>
  </div>
  <div class="ctrl-row">
    <button class="btn btn-red" onclick="sendCmd(3, 0)">Reset allarme</button>
  </div>
</div>

<script>
const MAX_PTS = 120;
const labels = [], dTemp = [], dSP = [], dPWM = [];
let ws = null;

function gridColor() { return 'rgba(255,255,255,0.08)'; }
function tickColor() { return '#888'; }

function makeChart(id, label, color, yMin, yMax, extra) {
  const datasets = [{ label, data: id==='cTemp'?dTemp:dPWM,
    borderColor: color, borderWidth: 1.5, pointRadius: 0, tension: 0.3, fill: false }];
  if (extra) datasets.push(extra);
  return new Chart(document.getElementById(id), {
    type: 'line', data: { labels, datasets },
    options: { responsive: true, maintainAspectRatio: false, animation: false,
      plugins: { legend: { display: id==='cTemp',
        labels: { color: tickColor(), font: { size: 11 }, boxWidth: 12 } } },
      scales: {
        x: { ticks: { maxTicksLimit: 8, color: tickColor(), font: { size: 11 } },
          grid: { color: gridColor() } },
        y: { min: yMin, max: yMax, ticks: { color: tickColor(), font: { size: 11 } },
          grid: { color: gridColor() } }
      }
    }
  });
}

const chartT = makeChart('cTemp', 'Temperatura (°C)', '#2a78d6', 15, 45,
  { label: 'Setpoint', data: dSP, borderColor: '#eda100', borderWidth: 1,
    borderDash: [4,3], pointRadius: 0, tension: 0, fill: false });
const chartP = makeChart('cPWM', 'PWM', '#1baf7a', 0, 255, null);

function addPoint(t, sp, pwm) {
  const now = new Date().toLocaleTimeString('it-IT');
  labels.push(now); dTemp.push(t); dSP.push(sp); dPWM.push(pwm);
  if (labels.length > MAX_PTS) { labels.shift(); dTemp.shift(); dSP.shift(); dPWM.shift(); }
  chartT.update(); chartP.update();
}

function updateMetrics(d) {
  document.getElementById('mT').textContent   = d.t.toFixed(2) + ' °C';
  document.getElementById('mT').className     = 'metric-value' + (d.al ? ' alarm' : '');
  document.getElementById('mSP').textContent  = d.sp.toFixed(1) + ' °C';
  document.getElementById('mAT').textContent  = d.at.toFixed(1) + ' °C';
  document.getElementById('mH').textContent   = d.h > -900 ? d.h.toFixed(1) + ' %' : '--';
  document.getElementById('mPWM').textContent = d.pwm;
  document.getElementById('mFan').textContent = d.fan ? 'ON' : 'off';
  document.getElementById('mAl').textContent  = d.al ? '⚠ ALLARME' : 'ok';
  document.getElementById('mAl').className    = 'metric-value' + (d.al ? ' alarm' : '');
  document.getElementById('mUp').textContent  = d.up + 's';
  // Aggiorna campi input con valori correnti
  document.getElementById('inSP').value = d.sp.toFixed(1);
  document.getElementById('inAT').value = d.at.toFixed(1);
}

function sendCmd(type, value) {
  if (!ws || ws.readyState !== WebSocket.OPEN) return;
  const msg = JSON.stringify({ cmd: type, val: value, kp: 0, ki: 0, kd: 0 });
  ws.send(msg);
}

function connect() {
  ws = new WebSocket('ws://' + location.host + '/ws');
  document.getElementById('dot').className = '';
  document.getElementById('connStatus').textContent = 'Connessione...';

  ws.onopen = () => {
    document.getElementById('dot').className = 'on';
    document.getElementById('connStatus').textContent = 'Connesso';
    document.getElementById('ipAddr').textContent = location.host;
  };

  ws.onmessage = (e) => {
    try {
      const d = JSON.parse(e.data);
      updateMetrics(d);
      addPoint(d.t, d.sp, d.pwm);
    } catch(err) {}
  };

  ws.onclose = () => {
    document.getElementById('dot').className = '';
    document.getElementById('connStatus').textContent = 'Disconnesso — riconnessione...';
    setTimeout(connect, 3000);
  };
}

connect();
</script>
</body>
</html>
)rawliteral";

// --- Gestione messaggi WebSocket ---
static void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                      AwsEventType type, void* arg, uint8_t* data, size_t len) {
    if (type == WS_EVT_DATA) {
        AwsFrameInfo* info = (AwsFrameInfo*)arg;
        if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
            char buf[128];
            size_t n = len < sizeof(buf)-1 ? len : sizeof(buf)-1;
            memcpy(buf, data, n);
            buf[n] = '\0';
            Command cmd;
            if (_cmdCb && command_parse(buf, cmd)) {
                _cmdCb(cmd);
            }
        }
    }
}

void web_server_init(WebCommandCallback cb) {
    _cmdCb = cb;

    _ws.onEvent(onWsEvent);
    _server.addHandler(&_ws);

    _server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        //req->send_P(200, "text/html", HTML);
        req->send(200, "text/html", HTML);
    });

    _server.begin();
    Serial.println("[WEB] server avviato su porta 80");
}

void web_server_send(const Telemetry& t) {
    if (_ws.count() == 0) return;
    char buf[128];
    telemetry_serialize(t, buf, sizeof(buf));
    _ws.textAll(buf);
}
