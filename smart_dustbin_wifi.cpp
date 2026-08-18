/*
  Smart Dustbin - Fill Level Monitor + Web Dashboard
  ------------------------------------------------------
  Measures how full the dustbin is using an ultrasonic sensor mounted
  at the top of the bin (facing down at the trash). Calculates fill
  percentage, turns on a RED LED when fill level crosses 80%, and
  hosts a live web dashboard (graph) showing fill level over time.

  Components:
  - ESP32 Dev Board (needed for WiFi)
  - HC-SR04 Ultrasonic Sensor (mounted at the top, facing down)
  - Red LED + 220 ohm resistor
  - (Optional) Servo motor for lid, jumper wires, breadboard

  How fill % is calculated:
  - binHeight = distance from sensor to the EMPTY bin floor (cm)
  - currentDistance = live reading from sensor to top of trash
  - fillPercent = ((binHeight - currentDistance) / binHeight) * 100

  Author: Nevlin Chetty
*/

#include <WiFi.h>
#include <WebServer.h>

// ---------------- WiFi credentials ----------------
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// ---------------- Pin definitions ----------------
const int trigPin = 5;
const int echoPin = 18;
const int redLedPin = 4;

// ---------------- Bin configuration ----------------
// Measure this once: distance (cm) from sensor to the bottom of an EMPTY bin
const float binHeight = 30.0;

// Threshold at which the red LED turns on
const int fullThreshold = 80; // percent

WebServer server(80);

// Store last 30 readings for the graph
const int HISTORY_SIZE = 30;
int fillHistory[HISTORY_SIZE];
int historyIndex = 0;

int currentFillPercent = 0;

void setup() {
  Serial.begin(115200);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(redLedPin, OUTPUT);
  digitalWrite(redLedPin, LOW);

  for (int i = 0; i < HISTORY_SIZE; i++) fillHistory[i] = 0;

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! Dashboard IP address: ");
  Serial.println(WiFi.localIP());

  // Web routes
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
}

void loop() {
  server.handleClient();

  static unsigned long lastReading = 0;
  if (millis() - lastReading > 3000) { // update every 3 seconds
    lastReading = millis();
    updateFillLevel();
  }
}

void updateFillLevel() {
  float distance = getDistanceCM();
  if (distance <= 0) return; // ignore bad readings

  float fill = ((binHeight - distance) / binHeight) * 100.0;
  fill = constrain(fill, 0, 100);
  currentFillPercent = (int)fill;

  // Red LED warning when bin is 80% full or more
  if (currentFillPercent >= fullThreshold) {
    digitalWrite(redLedPin, HIGH);
  } else {
    digitalWrite(redLedPin, LOW);
  }

  // Save to history for the graph
  fillHistory[historyIndex] = currentFillPercent;
  historyIndex = (historyIndex + 1) % HISTORY_SIZE;

  Serial.print("Fill level: ");
  Serial.print(currentFillPercent);
  Serial.println("%");
}

float getDistanceCM() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);
  if (duration == 0) return -1;

  return duration * 0.0343 / 2;
}

// ---------------- Web server handlers ----------------

// Returns current fill % and history as JSON
void handleData() {
  String json = "{\"current\":" + String(currentFillPercent) + ",\"history\":[";
  for (int i = 0; i < HISTORY_SIZE; i++) {
    int idx = (historyIndex + i) % HISTORY_SIZE;
    json += String(fillHistory[idx]);
    if (i < HISTORY_SIZE - 1) json += ",";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

// Serves the dashboard HTML page with a live-updating graph (no external libraries needed)
void handleRoot() {
  String html = R"HTML(
<!DOCTYPE html>
<html>
<head>
  <title>Smart Dustbin Dashboard</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial, sans-serif; background:#1e1e1e; color:#eee; text-align:center; padding:20px; }
    h1 { color:#4fc3f7; }
    #fillValue { font-size: 48px; font-weight:bold; margin: 10px 0; }
    #status { font-size: 20px; margin-bottom: 20px; }
    canvas { background:#2a2a2a; border-radius:8px; }
    .full { color:#ff4444; }
    .ok { color:#4caf50; }
  </style>
</head>
<body>
  <h1>Smart Dustbin - Fill Level</h1>
  <div id="fillValue">--%</div>
  <div id="status">Loading...</div>
  <canvas id="chart" width="360" height="200"></canvas>

  <script>
    async function fetchData() {
      const res = await fetch('/data');
      const data = await res.json();
      document.getElementById('fillValue').textContent = data.current + '%';

      const statusEl = document.getElementById('status');
      if (data.current >= 80) {
        statusEl.textContent = 'Bin is nearly full - please empty soon';
        statusEl.className = 'full';
      } else {
        statusEl.textContent = 'Bin level normal';
        statusEl.className = 'ok';
      }

      drawGraph(data.history);
    }

    function drawGraph(history) {
      const canvas = document.getElementById('chart');
      const ctx = canvas.getContext('2d');
      ctx.clearRect(0, 0, canvas.width, canvas.height);

      const w = canvas.width, h = canvas.height;
      const step = w / (history.length - 1);

      // Draw 80% threshold line
      ctx.strokeStyle = '#ff4444';
      ctx.setLineDash([5, 5]);
      const thresholdY = h - (80 / 100) * h;
      ctx.beginPath();
      ctx.moveTo(0, thresholdY);
      ctx.lineTo(w, thresholdY);
      ctx.stroke();
      ctx.setLineDash([]);

      // Draw fill history line
      ctx.strokeStyle = '#4fc3f7';
      ctx.lineWidth = 2;
      ctx.beginPath();
      history.forEach((val, i) => {
        const x = i * step;
        const y = h - (val / 100) * h;
        if (i === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
      });
      ctx.stroke();
    }

    fetchData();
    setInterval(fetchData, 3000); // refresh every 3 seconds
  </script>
</body>
</html>
)HTML";
  server.send(200, "text/html", html);
}
