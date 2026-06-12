#include <U8g2lib.h>
#include <Wire.h>
#include <MPU6050_light.h>
#include <RotaryEncoder.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <time.h> 

MPU6050 mpu(Wire);
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

#define CLK_PIN D0 
#define DT_PIN  D1 

RotaryEncoder encoder(CLK_PIN, DT_PIN, RotaryEncoder::LatchMode::FOUR3);

// --- WiFi and Web Server Settings ---
const char* ssid = "Kuzey";             
const char* wifiPassword = "kuzeykuzey"; 

WebServer server(80);
Preferences preferences;
bool webPortalActive = true; 
bool wifiConnected = false;

// --- Pomodoro Settings ---
int pomodoroWork = 25;
int pomodoroShortBreak = 5;
int pomodoroLongBreak = 15;
int pomodoroCycles = 4;

int currentPomodoroCycle = 1;
bool isPomodoroBreak = false;
long pomodoroRemainingSec = 25 * 60;
long pomodoroElapsedSec = 0;

// --- Time Tracking Variables ---
long totalStudySec = 0;      
long sessionStudySec = 0;    
long distractionSec = 0;     

long tytElapsedSec = 0;
long aytElapsedSec = 0;

// --- Other Global Variables ---
float accelX, accelY, accelZ;
float gyroX, gyroY, gyroZ;

int currentMode = 1;  
const char* modeNames[] = {"", "STUDY", "DISTRACT", "ALARM SET", "TYT EXAM", "AYT EXAM", "POMODORO", "STATISTICS"};

bool timerRunning = false;
bool alarmSetModeActive = false;  
bool tiltTriggered = false; 

unsigned long lastSecondTime = 0;
unsigned long lastTapTime = 0;
unsigned long lastShakeTime = 0; 
unsigned long lastSaveTime = 0;
unsigned long lastSetModeActivityTime = 0; 

float tapThreshold = 1.9;  
float shakeThreshold = 3.0; 

int  alarmSetMinutes = 25;    
long alarmRemainingSec = 25 * 60;
long tytRemainingSec = 165 * 60;
long aytRemainingSec = 180 * 60;  
unsigned long lastScreenTime = 0;

// --- Web Portal HTML Code ---
const char PROGMEM_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Study Portal</title>
    <meta name='viewport' content='width=device-width, initial-scale=1'>
    <style>
        body { font-family: Arial, sans-serif; text-align: center; background: #f4f4f9; color: #333; }
        .container { max-width: 600px; margin: 20px auto; background: #fff; padding: 20px; border-radius: 8px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }
        h2 { color: #4CAF50; }
        .stat { font-size: 1.1em; margin: 10px 0; display: flex; justify-content: space-between; padding: 0 20px; }
        .stat span { font-weight: bold; color: #2196F3; }
        .stat small { color: #666; font-weight: normal; font-size: 0.8em; }
        .stat.distract span { color: #f44336; }
        form { margin-top: 20px; text-align: left; display: inline-block; width: 100%; max-width: 300px; }
        label { display: block; margin: 10px 0 5px; font-weight: bold; }
        input[type=number] { width: 100%; padding: 8px; margin-bottom: 10px; border: 1px solid #ccc; border-radius: 4px; box-sizing: border-box; }
        input[type=submit] { background: #4CAF50; color: white; padding: 10px 15px; border: none; border-radius: 4px; cursor: pointer; width: 100%; font-size: 1.1em; }
        input[type=submit]:hover { background: #45a049; }
        .danger-zone { margin-top: 40px; border-top: 2px dashed #f44336; padding-top: 20px; }
        .danger-zone h2 { color: #f44336; font-size: 1.2em; }
        .danger-zone input[type=submit] { background: #f44336; }
        .danger-zone input[type=submit]:hover { background: #d32f2f; }
    </style>
</head>
<body>
    <div class='container'>
        <h2>Productivity Statistics</h2>
        <div class='stat'>Total Study: <span>%TOTAL_STUDY%</span></div>
        <div class='stat'>Current Session: <span>%SESSION_STUDY%</span> <small>(Resets on shake)</small></div>
        <div class='stat distract'>Distraction / Fun: <span>%DISTRACT_TIME%</span> <small>(Wasted time)</small></div>
        <div class='stat'>TYT Exam: <span>%TYT_TIME%</span></div>
        <div class='stat'>AYT Exam: <span>%AYT_TIME%</span></div>
        <div class='stat'>Pomodoro: <span>%POM_TIME%</span></div>
        
        <h2>Pomodoro Settings</h2>
        <form action='/save' method='POST'>
            <label>Work Duration (min):</label>
            <input type='number' name='work' value='%POM_WORK%' min='1' max='120'>
            <label>Short Break (min):</label>
            <input type='number' name='shortBreak' value='%POM_SHORT%' min='1' max='60'>
            <label>Long Break (min):</label>
            <input type='number' name='longBreak' value='%POM_LONG%' min='1' max='60'>
            <label>Long Break Interval (Cycles):</label>
            <input type='number' name='cycles' value='%POM_CYCLES_SET%' min='2' max='10'>
            <input type='submit' value='Save Settings'>
        </form>

        <!-- YENİ: SIFIRLAMA BÖLÜMÜ -->
        <div class='danger-zone'>
            <h2>Danger Zone</h2>
            <form action='/reset' method='POST' onsubmit="return confirm('Are you SURE you want to reset ALL study data? This CANNOT be undone!');">
                <input type='submit' value='Reset All Data'>
            </form>
        </div>
    </div>
</body>
</html>
)rawliteral";

// --- Function Prototypes ---
void calibrateSensors();
void readSensors();
void detectTap();
void detectTilt(); 
void detectShake(); 
void updateTimers();
void drawPageDots();
void connectToWiFi();
void handleRoot();
void handleSave();
void handleReset(); // YENİ
String formatTime(long totalSeconds);

void setup() {
  Serial.begin(115200);  
  Wire.begin(22, 23);  
  u8g2.begin();
  
  pinMode(CLK_PIN, INPUT_PULLUP);
  pinMode(DT_PIN, INPUT_PULLUP);
  
  byte status = mpu.begin();
  if (status != 0) {
    while (1) {
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_8x13_mr);
      u8g2.drawStr(0, 15, "NO SENSOR!");
      u8g2.sendBuffer();
      delay(1000);
    }
  }
  
  calibrateSensors();
  
  // Load data from persistent memory
  preferences.begin("study-data", true);
  totalStudySec = preferences.getULong("totalStudy", 0);
  distractionSec = preferences.getULong("distTime", 0); 
  tytElapsedSec = preferences.getULong("tytTime", 0);
  aytElapsedSec = preferences.getULong("aytTime", 0);
  pomodoroElapsedSec = preferences.getULong("pomTime", 0);
  pomodoroWork = preferences.getInt("pomWork", 25);
  pomodoroShortBreak = preferences.getInt("pomShort", 5);
  pomodoroLongBreak = preferences.getInt("pomLong", 15);
  pomodoroCycles = preferences.getInt("pomCycles", 4);
  preferences.end();
  
  sessionStudySec = 0; 
  
  pomodoroRemainingSec = pomodoroWork * 60;
  alarmRemainingSec = alarmSetMinutes * 60;

  connectToWiFi(); 
  
  if (wifiConnected) {
      server.on("/", handleRoot);
      server.on("/save", HTTP_POST, handleSave);
      server.on("/reset", HTTP_POST, handleReset); // YENİ: Reset rotası
      server.begin();
  }
  
  delay(1000);
}

void loop() {
  if (wifiConnected) {
      server.handleClient(); 
  }

  readSensors();      
  detectTap(); 
  detectTilt(); 
  detectShake(); 
  
  encoder.tick();
  int direction = (int)(encoder.getDirection());

  if (direction != 0) {
    if (timerRunning) {
      Serial.println("-> Timer is running! Mode cannot be changed.");
    }  
    else {
      if (currentMode == 3 && alarmSetModeActive) {
        if (direction < 0) alarmSetMinutes++;
        else alarmSetMinutes--;
        
        if (alarmSetMinutes < 1) alarmSetMinutes = 1;
        if (alarmSetMinutes > 180) alarmSetMinutes = 180;
        
        alarmRemainingSec = (long)alarmSetMinutes * 60;
        lastSetModeActivityTime = millis(); 
        
      } else {
        if (direction < 0) currentMode++;
        else currentMode--;
        
        if (currentMode < 1) currentMode = 1;
        if (currentMode > 7) currentMode = 7; 

        alarmSetModeActive = false;  
        Serial.print("-> New Mode: "); Serial.println(currentMode);
      }
    }
  }

  updateTimers();

  if (millis() - lastSaveTime > 60000) {
      lastSaveTime = millis();
      preferences.begin("study-data", false);
      preferences.putULong("totalStudy", totalStudySec);
      preferences.putULong("distTime", distractionSec); 
      preferences.putULong("tytTime", tytElapsedSec);
      preferences.putULong("aytTime", aytElapsedSec);
      preferences.putULong("pomTime", pomodoroElapsedSec);
      preferences.end();
  }

  // --- OLED DRAWING AREA ---
  if (millis() - lastScreenTime > 150) {
    lastScreenTime = millis();
    u8g2.clearBuffer();
    
    if (webPortalActive && wifiConnected) {
        u8g2.setFont(u8g2_font_8x13_mr);
        u8g2.drawStr(0, 12, "Waiting for Portal");
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(0, 30, "IP Address:");
        u8g2.drawStr(0, 45, WiFi.localIP().toString().c_str());
        u8g2.drawStr(0, 60, "Open Website...");
        u8g2.sendBuffer();
        return; 
    }

    // 1. REAL-TIME CLOCK (Top Left)
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    char timeStr[10];
    if (timeinfo != nullptr) {
        strftime(timeStr, sizeof(timeStr), "%H:%M", timeinfo); 
    } else {
        strcpy(timeStr, "--:--");
    }
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(0, 10, timeStr);

    // 2. MODE NAME (Centered)
    u8g2.setFont(u8g2_font_8x13_mr);
    int textWidth = strlen(modeNames[currentMode]) * 8; 
    u8g2.drawStr((128 - textWidth) / 2, 10, modeNames[currentMode]);
    
    u8g2.drawLine(0, 16, 128, 16); 

    // 3. STATISTICS SCREEN (Mode 7)
    if (currentMode == 7) {
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(0, 30, "Total  :");
        u8g2.drawStr(70, 30, formatTime(totalStudySec).c_str());
        
        u8g2.drawStr(0, 40, "Distrct:");
        u8g2.drawStr(70, 40, formatTime(distractionSec).c_str());
        
        u8g2.drawStr(0, 50, "TYT    :");
        u8g2.drawStr(70, 50, formatTime(tytElapsedSec).c_str());
        
        u8g2.drawStr(0, 60, "AYT    :");
        u8g2.drawStr(70, 60, formatTime(aytElapsedSec).c_str());
        
        u8g2.sendBuffer();
        return; 
    }

    // 4. TIMER DISPLAY (Modes 1 to 6)
    long hours = 0, minutes = 0, seconds = 0;

    if (currentMode == 1) {  
      hours = sessionStudySec / 3600;
      minutes = (sessionStudySec % 3600) / 60;
      seconds = sessionStudySec % 60;
    }  
    else if (currentMode == 2) { 
      hours = distractionSec / 3600;
      minutes = (distractionSec % 3600) / 60;
      seconds = distractionSec % 60;
    }
    else if (currentMode == 3) {  
      hours = alarmRemainingSec / 3600;
      minutes = (alarmRemainingSec % 3600) / 60;
      seconds = alarmRemainingSec % 60;
    }
    else if (currentMode == 4) {  
      hours = tytRemainingSec / 3600;
      minutes = (tytRemainingSec % 3600) / 60;
      seconds = tytRemainingSec % 60;
    }
    else if (currentMode == 5) {  
      hours = aytRemainingSec / 3600;
      minutes = (aytRemainingSec % 3600) / 60;
      seconds = aytRemainingSec % 60;
    }
    else if (currentMode == 6) { 
      hours = pomodoroRemainingSec / 3600;
      minutes = (pomodoroRemainingSec % 3600) / 60;
      seconds = pomodoroRemainingSec % 60;
    }

    if (currentMode != 7) { 
      u8g2.setFont(u8g2_font_10x20_tn);  
      u8g2.setCursor(24, 40);
      
      if (hours < 10) u8g2.print("0"); u8g2.print(hours); u8g2.print(":");
      if (minutes < 10) u8g2.print("0"); u8g2.print(minutes); u8g2.print(":");
      if (seconds < 10) u8g2.print("0"); u8g2.print(seconds);
      
      u8g2.setFont(u8g2_font_6x10_tf);
      if (timerRunning) {
        if (currentMode == 6) {
            u8g2.drawStr(32, 55, isPomodoroBreak ? "[ BREAK ]" : "[ WORK ]");
        } else {
            u8g2.drawStr(32, 55, "[ RUNNING ]");
        }
      } else {
        if (currentMode == 3) {
          if (alarmSetModeActive) {
            u8g2.setCursor(10, 55);
            u8g2.print("[SET] "); u8g2.print(alarmSetMinutes); u8g2.print(" min");
          } else {
            u8g2.drawStr(0, 55, "Tilt:Set Shake:Reset"); 
          }
        } else if (currentMode == 6) {
            u8g2.drawStr(0, 55, "Tap:Start Shake:Reset"); 
        } else if (currentMode == 4 || currentMode == 5) {
            u8g2.drawStr(15, 55, "Shake -> Reset"); 
        } else if (currentMode == 1) {
            u8g2.drawStr(0, 55, "Tap:Start Shake:Reset Sess."); 
        } else if (currentMode == 2) {
            u8g2.drawStr(0, 55, "Tap:Start Shake:Reset Time"); 
        } else {
          u8g2.drawStr(35, 55, "[ PAUSED ]");
        }
      }
    }  

    drawPageDots();
    u8g2.sendBuffer();
  }
}

void drawPageDots() {
  u8g2.setFont(u8g2_font_6x10_tf);  
  int startX = 36; 
  int yCoord = 63;  

  for (int i = 1; i <= 7; i++) {
    u8g2.setCursor(startX + ((i - 1) * 9), yCoord);  
    if (i == currentMode) {
      u8g2.print((char)183);  
    } else {
      u8g2.print(".");  
    }
  }
}

void updateTimers() {
  if (timerRunning && (millis() - lastSecondTime >= 1000)) {
    lastSecondTime = millis();
    
    if (currentMode == 1) {
        sessionStudySec++;
        totalStudySec++; 
    }  
    else if (currentMode == 2) {
        distractionSec++; 
    }
    else if (currentMode == 3) {
      if (alarmRemainingSec > 0) alarmRemainingSec--;
      else timerRunning = false;  
    }
    else if (currentMode == 4) {
      if (tytRemainingSec > 0) {
          tytRemainingSec--;
          tytElapsedSec++;
      } else timerRunning = false;  
    }
    else if (currentMode == 5) {
      if (aytRemainingSec > 0) {
          aytRemainingSec--;
          aytElapsedSec++;
      } else timerRunning = false;  
    }
    else if (currentMode == 6) { 
        if (pomodoroRemainingSec > 0) {
            pomodoroRemainingSec--;
            pomodoroElapsedSec++;
        } else {
            if (isPomodoroBreak) {
                isPomodoroBreak = false;
                currentPomodoroCycle++;
                if (currentPomodoroCycle > pomodoroCycles) {
                    currentPomodoroCycle = 1; 
                    timerRunning = false;
                } else {
                    pomodoroRemainingSec = pomodoroWork * 60; 
                }
            } else {
                isPomodoroBreak = true;
                if (currentPomodoroCycle % pomodoroCycles == 0) {
                    pomodoroRemainingSec = pomodoroLongBreak * 60; 
                } else {
                    pomodoroRemainingSec = pomodoroShortBreak * 60; 
                }
            }
        }
    }
  }
}

void calibrateSensors() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_8x13_mr);
  u8g2.drawStr(0, 15, "CALIBRATING...");
  u8g2.sendBuffer();
  delay(600);
  mpu.calcOffsets(true, true);  
  u8g2.clearBuffer();
  u8g2.drawStr(0, 15, "COMPLETE");
  u8g2.sendBuffer();
}

void readSensors() {
  mpu.update();
  accelX = mpu.getAccX();
  accelY = mpu.getAccY();
  accelZ = mpu.getAccZ();
  gyroX = mpu.getGyroX();
  gyroY = mpu.getGyroY();
  gyroZ = mpu.getGyroZ();
}

void detectTap() {
  float deviation = abs(accelZ - 1.0);
  if (deviation > tapThreshold) {
    if (millis() - lastTapTime > 600) {  
      timerRunning = !timerRunning;  
      lastTapTime = millis();
      lastSecondTime = millis();  
    }
  }
}

void detectTilt() {
  if (currentMode == 3 && !timerRunning) {
    float tiltMagnitude = abs(accelX) + abs(accelY);
    
    if (tiltMagnitude > 0.7) { 
      if (!tiltTriggered) {
        tiltTriggered = true;
        alarmSetModeActive = !alarmSetModeActive;
        lastSetModeActivityTime = millis(); 
      }
    } 
    else if (tiltMagnitude < 0.3) {
      tiltTriggered = false; 
    }

    if (alarmSetModeActive && (millis() - lastSetModeActivityTime > 5000)) {
      alarmSetModeActive = false;
      tiltTriggered = false; 
    }
  }
}

void detectShake() {
  if (currentMode == 1 || currentMode == 2 || currentMode == 3 || currentMode == 4 || currentMode == 5 || currentMode == 6) {
    float totalAccel = sqrt(accelX * accelX + accelY * accelY + accelZ * accelZ);
    
    if (totalAccel > shakeThreshold && (millis() - lastShakeTime > 1000)) {
      lastShakeTime = millis();
      
      if (currentMode == 1) {
        sessionStudySec = 0; 
      }
      else if (currentMode == 2) {
        distractionSec = 0; 
      }
      else if (currentMode == 3) {
        alarmRemainingSec = alarmSetMinutes * 60;
        timerRunning = false; 
      }
      else if (currentMode == 4) {
        tytRemainingSec = 165 * 60;
        timerRunning = false;
      }
      else if (currentMode == 5) {
        aytRemainingSec = 180 * 60;
        timerRunning = false;
      }
      else if (currentMode == 6) {
        pomodoroRemainingSec = pomodoroWork * 60;
        isPomodoroBreak = false;
        currentPomodoroCycle = 1;
        timerRunning = false;
      }
      Serial.println("-> SHAKE DETECTED! Timer reset.");
    }
  }
}

void connectToWiFi() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_8x13_mr);
  u8g2.drawStr(0, 12, "Connecting WiFi...");
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 28, "SSID:");
  u8g2.drawStr(0, 40, ssid);
  u8g2.drawStr(0, 52, "Pass:");
  u8g2.drawStr(0, 62, wifiPassword); 
  u8g2.sendBuffer();
  
  WiFi.begin(ssid, wifiPassword);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_8x13_mr);
    u8g2.drawStr(0, 12, "Connected!");
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(0, 30, "IP Address:");
    u8g2.drawStr(0, 45, WiFi.localIP().toString().c_str());
    u8g2.drawStr(0, 60, "Open Website...");
    u8g2.sendBuffer();
  } else {
    wifiConnected = false;
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_8x13_mr);
    u8g2.drawStr(0, 12, "WiFi Failed!");
    u8g2.sendBuffer();
    delay(2000);
  }
}

void handleRoot() {
  webPortalActive = false; 
  String html = PROGMEM_HTML;
  html.replace("%TOTAL_STUDY%", formatTime(totalStudySec));
  html.replace("%SESSION_STUDY%", formatTime(sessionStudySec));
  html.replace("%DISTRACT_TIME%", formatTime(distractionSec)); 
  html.replace("%TYT_TIME%", formatTime(tytElapsedSec));
  html.replace("%AYT_TIME%", formatTime(aytElapsedSec));
  html.replace("%POM_TIME%", formatTime(pomodoroElapsedSec));
  html.replace("%POM_WORK%", String(pomodoroWork));
  html.replace("%POM_SHORT%", String(pomodoroShortBreak));
  html.replace("%POM_LONG%", String(pomodoroLongBreak));
  html.replace("%POM_CYCLES_SET%", String(pomodoroCycles));
  server.send(200, "text/html", html);
}

void handleSave() {
  if (server.hasArg("work")) pomodoroWork = server.arg("work").toInt();
  if (server.hasArg("shortBreak")) pomodoroShortBreak = server.arg("shortBreak").toInt();
  if (server.hasArg("longBreak")) pomodoroLongBreak = server.arg("longBreak").toInt();
  if (server.hasArg("cycles")) pomodoroCycles = server.arg("cycles").toInt();
  
  preferences.begin("study-data", false);
  preferences.putInt("pomWork", pomodoroWork);
  preferences.putInt("pomShort", pomodoroShortBreak);
  preferences.putInt("pomLong", pomodoroLongBreak);
  preferences.putInt("pomCycles", pomodoroCycles);
  preferences.end();
  
  if (!timerRunning && currentMode == 6) {
      pomodoroRemainingSec = pomodoroWork * 60;
      isPomodoroBreak = false;
      currentPomodoroCycle = 1;
  }
  
  server.sendHeader("Location", "/");
  server.send(303);
}

// --- YENİ: TÜM VERİLERİ SIFIRLAMA FONKSİYONU ---
void handleReset() {
  // 1. Kalıcı hafızadaki süreleri sıfırla
  preferences.begin("study-data", false);
  preferences.putULong("totalStudy", 0);
  preferences.putULong("distTime", 0);
  preferences.putULong("tytTime", 0);
  preferences.putULong("aytTime", 0);
  preferences.putULong("pomTime", 0);
  preferences.end();

  // 2. Anlık global değişkenleri de sıfırla (cihaz yeniden başlatmaya gerek kalmaz)
  totalStudySec = 0;
  sessionStudySec = 0;
  distractionSec = 0;
  tytElapsedSec = 0;
  aytElapsedSec = 0;
  pomodoroElapsedSec = 0;
  
  // Pomodoro mevcut durumunu da başa sar
  pomodoroRemainingSec = pomodoroWork * 60;
  isPomodoroBreak = false;
  currentPomodoroCycle = 1;

  Serial.println("-> ALL DATA RESET FROM WEB PORTAL");
  
  // Ana sayfaya yönlendir
  server.sendHeader("Location", "/");
  server.send(303);
}

String formatTime(long totalSeconds) {
  int h = totalSeconds / 3600;
  int m = (totalSeconds % 3600) / 60;
  int s = totalSeconds % 60;
  char buf[10];
  sprintf(buf, "%02d:%02d:%02d", h, m, s);
  return String(buf);
}
