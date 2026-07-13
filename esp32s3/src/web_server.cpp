#include "web_server.h"
#include "../../shared/telemetry.h"
#include "incubation.h"
#include <ESPAsyncWebServer.h>
#include <Arduino.h>

static AsyncWebServer  _server(80);
static AsyncWebSocket  _ws("/ws");
static WebCommandCallback _cmdCb = nullptr;

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
h3 { font-weight: 500; margin-bottom: 0.5rem; font-size: 14px; color: #aaa; }
.grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(110px, 1fr)); gap: 8px; margin-bottom: 1rem; }
.metric { background: #2a2a2a; border-radius: 8px; padding: 0.75rem; }
.metric-label { font-size: 11px; color: #888; margin-bottom: 4px; }
.metric-value { font-size: 20px; font-weight: 500; }
.alarm { color: #e55; }
.ok { color: #5e5; }
.chart-wrap { position: relative; height: 180px; margin-bottom: 1rem; }
.card { background: #2a2a2a; border-radius: 8px; padding: 1rem; margin-bottom: 1rem; }
.ctrl-row { display: flex; align-items: center; gap: 8px; margin-bottom: 8px; }
.ctrl-row label { width: 160px; font-size: 13px; color: #aaa; }
.ctrl-row input[type=number] { background: #333; border: 1px solid #555; color: #eee;
  padding: 4px 8px; border-radius: 4px; width: 80px; }
.ctrl-row select { background: #333; border: 1px solid #555; color: #eee;
  padding: 4px 8px; border-radius: 4px; flex: 1; }
.btn { background: #333; color: #eee; border: 1px solid #555; padding: 6px 14px;
  border-radius: 6px; cursor: pointer; font-size: 13px; margin-right: 4px; }
.btn:hover { background: #444; }
.btn-red    { border-color: #e55; color: #e55; }
.btn-green  { border-color: #5e5; color: #5e5; }
.btn-orange { border-color: #fa0; color: #fa0; }
.status { font-size: 12px; color: #888; margin-bottom: 1rem; }
#dot { display:inline-block; width:8px; height:8px; border-radius:50%; background:#e55; margin-right:4px; }
#dot.on { background:#5e5; }
.inc-bar { background:#333; border-radius:4px; height:8px; margin-top:6px; }
.inc-bar-fill { background:#2a78d6; height:8px; border-radius:4px; transition: width 0.5s; }
.state-idle    { color: #888; }
.state-running { color: #5e5; }
.state-paused  { color: #fa0; }
.state-done    { color: #2a78d6; }
</style>
</head>
<body>
<h2>🥚 Incubatrice</h2>
<div class="status"><span id="dot"></span><span id="connStatus">Disconnesso</span></div>

<div class="grid">
  <div class="metric"><div class="metric-label">Temperatura</div><div class="metric-value" id="mT">--</div></div>
  <div class="metric"><div class="metric-label">Setpoint</div><div class="metric-value" id="mSP">--</div></div>
  <div class="metric"><div class="metric-label">Soglia allarme</div><div class="metric-value" id="mAT">--</div></div>
  <div class="metric"><div class="metric-label">Umidità</div><div class="metric-value" id="mH">--</div></div>
  <div class="metric"><div class="metric-label">PWM</div><div class="metric-value" id="mPWM">--</div></div>
  <div class="metric"><div class="metric-label">Ventola</div><div class="metric-value" id="mFan">--</div></div>
  <div class="metric"><div class="metric-label">Allarme</div><div class="metric-value" id="mAl">--</div></div>
  <div class="metric"><div class="metric-label">Uptime</div><div class="metric-value" id="mUp">--</div></div>
</div>

<div class="chart-wrap"><canvas id="cTemp"></canvas></div>
<div class="chart-wrap"><canvas id="cPWM"></canvas></div>

<!-- Ciclo incubazione -->
<div class="card">
  <h3>Ciclo incubazione</h3>
  <div class="grid" style="grid-template-columns:repeat(auto-fit,minmax(100px,1fr));margin-bottom:0.75rem">
    <div class="metric"><div class="metric-label">Stato</div><div class="metric-value" id="iState">--</div></div>
    <div class="metric"><div class="metric-label">Ciclo</div><div class="metric-value" id="iCycle">--</div></div>
    <div class="metric"><div class="metric-label">Fase</div><div class="metric-value" id="iPhase">--</div></div>
    <div class="metric"><div class="metric-label">Giorno</div><div class="metric-value" id="iDay">--</div></div>
    <div class="metric"><div class="metric-label">Girauova</div><div class="metric-value" id="iTurner">--</div></div>
  </div>
  <div class="inc-bar"><div class="inc-bar-fill" id="iBar" style="width:0%"></div></div>
  <div style="margin-top:1rem">
    <div class="ctrl-row">
      <label>Seleziona ciclo</label>
      <select id="selCycle">
        <option value="0">Gallina (21 gg)</option>
        <option value="1">Anatra (28 gg)</option>
        <option value="2">Quaglia (16 gg)</option>
        <option value="3">Tacchino (28 gg)</option>
        <option value="4">Fagiano (24 gg)</option>
      </select>
    </div>
    <div class="ctrl-row">
      <button class="btn btn-green" onclick="sendCmd(6, +document.getElementById('selCycle').value)">▶ Avvia</button>
      <button class="btn btn-orange" onclick="sendCmd(8, 0)">⏸ Pausa</button>
      <button class="btn btn-green"  onclick="sendCmd(9, 0)">▶ Riprendi</button>
      <button class="btn btn-red"   onclick="sendCmd(7, 0)">■ Stop</button>
    </div>
  </div>
</div>

<!-- Impostazioni -->
<div class="card">
  <h3>Impostazioni</h3>
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
const INC_STATES = ['state-idle','state-running','state-paused','state-done'];
const INC_STATE_LABELS = ['Idle','In corso','In pausa','Completato'];
const INC_NAMES = ['Gallina','Anatra','Quaglia','Tacchino','Fagiano'];
const INC_TOTAL_DAYS = [21, 28, 16, 28, 24];
let ws = null;

function gridColor() { return 'rgba(255,255,255,0.08)'; }
function tickColor() { return '#888'; }

function makeChart(id, color, yMin, yMax, extra) {
  const data = id==='cTemp' ? dTemp : dPWM;
  const datasets = [{ label: id==='cTemp'?'Temperatura (°C)':'PWM',
    data, borderColor: color, borderWidth:1.5, pointRadius:0, tension:0.3, fill:false }];
  if (extra) datasets.push(extra);
  return new Chart(document.getElementById(id), {
    type:'line', data:{ labels, datasets },
    options:{ responsive:true, maintainAspectRatio:false, animation:false,
      plugins:{ legend:{ display:id==='cTemp', labels:{ color:tickColor(), font:{size:11}, boxWidth:12 }}},
      scales:{
        x:{ ticks:{ maxTicksLimit:8, color:tickColor(), font:{size:11}}, grid:{color:gridColor()}},
        y:{ min:yMin, max:yMax, ticks:{color:tickColor(), font:{size:11}}, grid:{color:gridColor()}}
      }
    }
  });
}

const chartT = makeChart('cTemp','#2a78d6',15,45,
  { label:'Setpoint', data:dSP, borderColor:'#eda100', borderWidth:1,
    borderDash:[4,3], pointRadius:0, tension:0, fill:false });
const chartP = makeChart('cPWM','#1baf7a',0,255,null);

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
  document.getElementById('mAl').className    = 'metric-value' + (d.al ? ' alarm' : ' ok');
  document.getElementById('mUp').textContent  = d.up + 's';
  //document.getElementById('inSP').value = d.sp.toFixed(1);
  if (document.activeElement !== document.getElementById('inSP'))
    document.getElementById('inSP').value = d.sp.toFixed(1);
  document.getElementById('inAT').value = d.at.toFixed(1);

  // Ciclo incubazione
  const st = d.is || 0;
  const stEl = document.getElementById('iState');
  stEl.textContent = INC_STATE_LABELS[st] || '--';
  stEl.className = 'metric-value ' + (INC_STATES[st] || '');

  document.getElementById('iCycle').textContent  = st > 0 ? (INC_NAMES[d.ic] || '--') : '--';
  document.getElementById('iPhase').textContent  = st > 0 ? (d.ip + 1) : '--';
  document.getElementById('iDay').textContent    = st > 0 ? ('Gg ' + d.id) : '--';
  document.getElementById('iTurner').textContent = st === 1 ? (d.tp ? 'Pos B' : 'Pos A') : '--';

  // Barra progresso
  if (st === 1 || st === 2) {
    const total = INC_TOTAL_DAYS[d.ic] || 21;
    // calcola giorno totale sommando fasi precedenti — approssimazione con id
    const pct = Math.min(100, (d.id / total) * 100);
    document.getElementById('iBar').style.width = pct + '%';
  }
}

function sendCmd(type, value) {
  if (!ws || ws.readyState !== WebSocket.OPEN) return;
  ws.send(JSON.stringify({ cmd: type, val: value, kp:0, ki:0, kd:0 }));
}

function connect() {
  ws = new WebSocket('ws://' + location.host + '/ws');
  document.getElementById('dot').className = '';
  document.getElementById('connStatus').textContent = 'Connessione...';
  ws.onopen = () => {
    document.getElementById('dot').className = 'on';
    document.getElementById('connStatus').textContent = 'Connesso — ' + location.host;
  };
  ws.onmessage = (e) => {
    try { const d = JSON.parse(e.data); updateMetrics(d); addPoint(d.t, d.sp, d.pwm); } catch(err) {}
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
            if (_cmdCb && command_parse(buf, cmd)) _cmdCb(cmd);
        }
    }
}

void web_server_init(WebCommandCallback cb) {
    _cmdCb = cb;
    _ws.onEvent(onWsEvent);
    _server.addHandler(&_ws);
    _server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "text/html", HTML);
    });
    _server.begin();
    Serial.println("[WEB] server avviato su porta 80");
}

void web_server_send(const Telemetry& t) {
    if (_ws.count() == 0) return;
    char buf[160];
    telemetry_serialize(t, buf, sizeof(buf));
    _ws.textAll(buf);
}
