#ifndef WEB_SERVER_H
#define WEB_SERVER_H
const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="vi">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP32 Heart Rate Monitor</title>
<link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0-alpha1/dist/css/bootstrap.min.css" rel="stylesheet">
<link href="https://fonts.googleapis.com/css2?family=Bebas+Neue&family=Manrope:wght@400;500;600;700;800&display=swap" rel="stylesheet">
<style>
* { box-sizing: border-box; }
body { margin: 0; padding: 0; background: #111315; color: #ffffff; font-family: 'Manrope', sans-serif; }
.container-main { max-width: 900px; margin: auto; padding: 20px; }
.title { font-family: 'Bebas Neue', sans-serif; font-size: 42px; letter-spacing: 2px; text-align: center; margin-bottom: 20px; }
.card-box { background: #1b1e21; border-radius: 18px; padding: 20px; margin-bottom: 20px; box-shadow: 0 5px 20px rgba(0,0,0,0.25); }
.section-title { font-size: 18px; font-weight: 700; margin-bottom: 15px; }
#myCanvas { width: 100%; max-width: 700px; height: 340px; display: block; margin: auto; background: #080909; border: 1px solid #34383c; border-radius: 10px; }
.bpm-value { text-align: center; font-size: 52px; font-weight: 800; margin-top: 15px; }
.status { text-align: center; font-size: 18px; margin-top: 5px; color: #b8bec5; }
.stat-box { text-align: center; background: #111315; border-radius: 12px; padding: 15px 5px; }
.stat-title { font-size: 14px; color: #9ca3aa; }
.stat-value { font-size: 27px; font-weight: 800; margin-top: 5px; }
.sample-info { text-align: center; margin-top: 15px; color: #aeb4ba; font-size: 14px; }
#BTN_Start_Stop { width: 100%; max-width: 350px; font-size: 20px; font-weight: 700; border-radius: 12px; padding: 12px; }
.info-text { text-align: center; color: #858c93; font-size: 13px; margin-top: 10px; }
</style>
</head>
<body>
<div class="container-main">
    <div class="title">HEART RATE MONITOR</div>
    <div class="card-box">
        <div class="section-title">Tín hiệu nhịp tim</div>
        <canvas id="myCanvas" width="700" height="340"></canvas>
        <div id="bpm_Show" class="bpm-value">0 BPM</div>
        <div id="statusText" class="status">Chưa đo</div>
    </div>
    <div class="card-box">
        <div class="section-title">Thống kê BPM</div>
        <div class="row g-2">
            <div class="col-4">
                <div class="stat-box">
                    <div class="stat-title">MAX</div>
                    <div id="maxBPM" class="stat-value">--</div>
                </div>
            </div>
            <div class="col-4">
                <div class="stat-box">
                    <div class="stat-title">MIN</div>
                    <div id="minBPM" class="stat-value">--</div>
                </div>
            </div>
            <div class="col-4">
                <div class="stat-box">
                    <div class="stat-title">AVG</div>
                    <div id="avgBPM" class="stat-value">--</div>
                </div>
            </div>
        </div>
        <div id="sampleInfo" class="sample-info">0 / 12 mẫu</div>
    </div>
    <div class="card-box text-center">
        <button id="BTN_Start_Stop" type="button" class="btn btn-success" onclick="send_BTN_Cmd()">Bắt đầu đo</button>
        <div class="info-text">Giá trị đo sau một phút</div>
    </div>
</div>
<script>
var canvas = document.getElementById("myCanvas");
var ctx = canvas.getContext("2d");
var GRAPH_LEFT = 10;
var GRAPH_RIGHT = 10;
var GRAPH_TOP = 25;
var GRAPH_BOTTOM = 25;
var GRAPH_WIDTH = canvas.width - GRAPH_LEFT - GRAPH_RIGHT;
var GRAPH_HEIGHT = canvas.height - GRAPH_TOP - GRAPH_BOTTOM;
var latestHeartbeatSignal = 0;
var latestBPM = 0;
var frozenBPM = 0;
var isMeasuring = false;
var signalHistory = [];
var MAX_GRAPH_POINTS = 240;
var ecgPhase = 0;
var TICK_MS = 35;
var ECG_MIN = -0.55;
var ECG_MAX = 1.2;
var sampleTimer = null;
var bpmSamples = [];
var MAX_SAMPLES = 12;
var SAMPLE_INTERVAL = 5000;
 
function mapValue(value, inMin, inMax, outMin, outMax) {
    return ((value - inMin) * (outMax - outMin) / (inMax - inMin)) + outMin;
}
function clearCanvas() {
    ctx.fillStyle = "#080909";
    ctx.fillRect(0, 0, canvas.width, canvas.height);
}
function drawGrid() {
    ctx.strokeStyle = "#25292d";
    ctx.lineWidth = 1;
    var verticalLines = 12;
    for (var i = 0; i <= verticalLines; i++) {
        var x = GRAPH_LEFT + (i / verticalLines) * GRAPH_WIDTH;
        ctx.beginPath();
        ctx.moveTo(x, GRAPH_TOP);
        ctx.lineTo(x, canvas.height - GRAPH_BOTTOM);
        ctx.stroke();
    }
    var horizontalLines = 6;
    for (var j = 0; j <= horizontalLines; j++) {
        var y = GRAPH_TOP + (j / horizontalLines) * GRAPH_HEIGHT;
        ctx.beginPath();
        ctx.moveTo(GRAPH_LEFT, y);
        ctx.lineTo(canvas.width - GRAPH_RIGHT, y);
        ctx.stroke();
    }
}
function reset_Graph() {
    clearCanvas();
    drawGrid();
    signalHistory = [];
    for (var i = 0; i < MAX_GRAPH_POINTS; i++) {
        signalHistory.push(0);
    }
    drawHeartbeatSignal();
}
function gaussianPulse(phase, center, width, amplitude) {
    var d = phase - center;
    return amplitude * Math.exp(-(d * d) / (2 * width * width));
}
function ecgWaveform(phase) {
    var value = 0;
    value += gaussianPulse(phase, 0.10, 0.020, 0.12);
    value += gaussianPulse(phase, 0.26, 0.008, -0.10);
    value += gaussianPulse(phase, 0.28, 0.007, 1.00);
    value += gaussianPulse(phase, 0.305, 0.010, -0.30);
    value += gaussianPulse(phase, 0.55, 0.045, 0.28);
    return value;
}
function drawHeartbeatSignal() {
    if (signalHistory.length < 2) return;
    var graphMin = ECG_MIN;
    var graphMax = ECG_MAX;
    var startIndex = 0;
    var displayCount = signalHistory.length;
    if (displayCount > MAX_GRAPH_POINTS) {
        startIndex = displayCount - MAX_GRAPH_POINTS;
        displayCount = MAX_GRAPH_POINTS;
    }
    ctx.strokeStyle = "#ff4d6d";
    ctx.lineWidth = 2;
    ctx.lineJoin = "round";
    ctx.lineCap = "round";
    ctx.beginPath();
    for (var p = 0; p < displayCount; p++) {
        var index = startIndex + p;
        var signal = signalHistory[index];
        var x = GRAPH_LEFT + (p / Math.max(1, displayCount - 1)) * GRAPH_WIDTH;
        var y = mapValue(signal, graphMin, graphMax, canvas.height - GRAPH_BOTTOM, GRAPH_TOP);
        y = Math.max(GRAPH_TOP, Math.min(canvas.height - GRAPH_BOTTOM, y));
        if (p === 0) {
            ctx.moveTo(x, y);
        } else {
            ctx.lineTo(x, y);
        }
    }
    ctx.stroke();
}
function updateGraph() {
    var value;
    if (!isMeasuring || latestBPM <= 0) {
        value = 0;
        ecgPhase = 0;
    } else {
        var beatDuration = 60000 / latestBPM;
        ecgPhase += TICK_MS / beatDuration;
        if (ecgPhase >= 1) {
            ecgPhase -= Math.floor(ecgPhase);
        }
        value = ecgWaveform(ecgPhase);
    }
    signalHistory.push(value);
    if (signalHistory.length > MAX_GRAPH_POINTS) {
        signalHistory.shift();
    }
    clearCanvas();
    drawGrid();
    drawHeartbeatSignal();
}
function myTimer() {
    updateGraph();
}
function updateBPMDisplay() {
    if (!isMeasuring) {
        if (frozenBPM <= 0) {
            document.getElementById("bpm_Show").innerHTML = "0 BPM";
        } else {
            document.getElementById("bpm_Show").innerHTML = Math.round(frozenBPM) + " BPM";
        }
        return;
    }
    if (latestBPM <= 0) {
        document.getElementById("bpm_Show").innerHTML = "0 BPM";
    } else {
        document.getElementById("bpm_Show").innerHTML = Math.round(latestBPM) + " BPM";
    }
}
function updateStatus() {
    var status = document.getElementById("statusText");
    if (!isMeasuring) {
        status.innerHTML = "Chưa đo";
        status.style.color = "#b8bec5";
        return;
    }
    if (latestBPM <= 0) {
        status.innerHTML = "Đang chờ tín hiệu...";
        status.style.color = "#f59e0b";
        return;
    }
    if (latestBPM < 60) {
        status.innerHTML = "Nhịp chậm";
        status.style.color = "#60a5fa";
    } else if (latestBPM <= 100) {
        status.innerHTML = "Bình thường";
        status.style.color = "#22c55e";
    } else {
        status.innerHTML = "Nhịp nhanh";
        status.style.color = "#ef4444";
    }
}
function updateStats() {
    if (bpmSamples.length === 0) {
        document.getElementById("maxBPM").innerHTML = "--";
        document.getElementById("minBPM").innerHTML = "--";
        document.getElementById("avgBPM").innerHTML = "--";
        document.getElementById("sampleInfo").innerHTML = "0 / 12 mẫu";
        return;
    }
    var maxValue = bpmSamples[0];
    var minValue = bpmSamples[0];
    var total = 0;
    for (var i = 0; i < bpmSamples.length; i++) {
        var value = bpmSamples[i];
        if (value > maxValue) maxValue = value;
        if (value < minValue) minValue = value;
        total += value;
    }
    var average = total / bpmSamples.length;
    document.getElementById("maxBPM").innerHTML = maxValue;
    document.getElementById("minBPM").innerHTML = minValue;
    document.getElementById("avgBPM").innerHTML = Math.ceil(average);
    document.getElementById("sampleInfo").innerHTML = bpmSamples.length + " / 12 mẫu";
}
function resetStatistics() {
    bpmSamples = [];
    document.getElementById("maxBPM").innerHTML = "--";
    document.getElementById("minBPM").innerHTML = "--";
    document.getElementById("avgBPM").innerHTML = "--";
    document.getElementById("sampleInfo").innerHTML = "0 / 12 mẫu";
}
function takeBPMSample() {
    if (!isMeasuring) return;
    if (latestBPM <= 0) return;
    bpmSamples.push(Math.round(latestBPM));
    if (bpmSamples.length > MAX_SAMPLES) {
        bpmSamples.shift();
    }
    updateStats();
}
function startSampleTimer() {
    if (sampleTimer !== null) {
        clearInterval(sampleTimer);
    }
    sampleTimer = setInterval(takeBPMSample, SAMPLE_INTERVAL);
}
function stopSampleTimer() {
    if (sampleTimer !== null) {
        clearInterval(sampleTimer);
        sampleTimer = null;
    }
}
function setMeasuringState(state) {
    if (state === isMeasuring) return;
    isMeasuring = state;
    var button = document.getElementById("BTN_Start_Stop");
    if (isMeasuring) {
        button.innerHTML = "Dừng đo";
        button.classList.remove("btn-success");
        button.classList.add("btn-danger");
        resetStatistics();
        startSampleTimer();
        ecgPhase = 0;
        signalHistory = [];
        for (var i = 0; i < MAX_GRAPH_POINTS; i++) {
            signalHistory.push(0);
        }
        document.getElementById("statusText").innerHTML = "Đang đo...";
        document.getElementById("statusText").style.color = "#f59e0b";
    } else {
        button.innerHTML = "Bắt đầu đo";
        button.classList.remove("btn-danger");
        button.classList.add("btn-success");
        stopSampleTimer();
        frozenBPM = latestBPM;
        ecgPhase = 0;
        signalHistory = [];
        for (var j = 0; j < MAX_GRAPH_POINTS; j++) {
            signalHistory.push(0);
        }
        updateStatus();
        updateBPMDisplay();
    }
}
function send_BTN_Cmd() {
    var command;
    if (isMeasuring) {
        command = "STOP";
    } else {
        command = "START";
    }
    fetch("/BTN_Comd?BTN_Start_Get_BPM=" + command)
    .then(function(response) { return response.text(); })
    .catch(function(error) { console.log(error); });
}
var source = new EventSource("/events");
source.addEventListener("allDataJSON", function(event) {
    try {
        var obj = JSON.parse(event.data);
        var heartbeat = Number(obj.heartbeat_Signal);
        var bpm = Number(obj.BPM_Val);
        var state = obj.BPM_State === true || obj.BPM_State === "true";
        if (isNaN(heartbeat)) heartbeat = 0;
        if (isNaN(bpm)) bpm = 0;
        latestHeartbeatSignal = heartbeat;
        latestBPM = bpm;
        setMeasuringState(state);
        updateBPMDisplay();
        updateStatus();
    } catch(error) {
        console.log("JSON error:", error);
    }
}, false);
source.onopen = function() {
    console.log("SSE connected");
};
source.onerror = function(error) {
    console.log("SSE connection error");
};
reset_Graph();
document.getElementById("bpm_Show").innerHTML = "0 BPM";
setInterval(myTimer, 35);
</script>
</body>
</html>
)=====";
#endif
