#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "Kuzey";
const char* password = "kuzeykuzey";

WebServer server(80);

// -------------------- STATE --------------------
bool studying = false;
unsigned long startMillis = 0;
unsigned long totalMillis = 0;

// -------------------- HELPERS --------------------
unsigned long getCurrentTime() {
  if (studying) {
    return totalMillis + (millis() - startMillis);
  }
  return totalMillis;
}

// -------------------- UI --------------------
String htmlPage() {
  return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Flopper</title>
  <style>
    body {
      background:#0f0f0f;
      color:white;
      font-family:Arial;
      text-align:center;
      padding-top:60px;
    }
    .time {
      font-size:64px;
      margin:20px;
    }
    button {
      padding:10px 20px;
      margin:10px;
      font-size:18px;
      cursor:pointer;
    }
  </style>
</head>
<body>

<h1>🧠 Flopper Focus System</h1>

<div id="status">Loading...</div>
<div class="time" id="time">0</div>

<button onclick="fetch('/start', {method:'POST'})">Start</button>
<button onclick="fetch('/stop', {method:'POST'})">Stop</button>
<button onclick="fetch('/reset', {method:'POST'})">Reset</button>

<script>
async function update() {
  const res = await fetch('/state');
  const data = await res.json();

  document.getElementById("time").innerText = data.time + " ms";
  document.getElementById("status").innerText =
    data.studying ? "Studying 🟢" : "Idle 🔴";
}

setInterval(update, 500);
update();
</script>

</body>
</html>
)rawliteral";
}

// -------------------- ROUTES --------------------
void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

void handleState() {
  String json = "{";
  json += "\"studying\":" + String(studying ? "true" : "false") + ",";
  json += "\"time\":" + String(getCurrentTime());
  json += "}";
  server.send(200, "application/json", json);
}

void handleStart() {
  if (!studying) {
    studying = true;
    startMillis = millis();
  }
  server.send(200, "text/plain", "started");
}

void handleStop() {
  if (studying) {
    studying = false;
    totalMillis += millis() - startMillis;
  }
  server.send(200, "text/plain", "stopped");
}

void handleReset() {
  studying = false;
  totalMillis = 0;
  startMillis = 0;
  server.send(200, "text/plain", "reset");
}

// -------------------- SETUP --------------------
void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/state", handleState);
  server.on("/start", handleStart);
  server.on("/stop", handleStop);
  server.on("/reset", handleReset);

  server.begin();
}

// -------------------- LOOP --------------------
void loop() {
  server.handleClient();
}
