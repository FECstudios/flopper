#include <U8g2lib.h>
#include <Wire.h>
#include <MPU6050_light.h>
#include <RotaryEncoder.h>
#include <Preferences.h>

MPU6050 mpu(Wire);
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

#define CLK_PIN D0 
#define DT_PIN  D1 

RotaryEncoder encoder(CLK_PIN, DT_PIN, RotaryEncoder::LatchMode::FOUR3);
Preferences preferences;

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

// For factory reset via upside-down gesture
unsigned long upsideDownStartTime = 0;
bool isUpsideDown = false;

float tapThreshold = 1.9;  
float shakeThreshold = 3.0; 

int  alarmSetMinutes = 25;    
long alarmRemainingSec = 25 * 60;
long tytRemainingSec = 165 * 60;
long aytRemainingSec = 180 * 60;  
unsigned long lastScreenTime = 0;

// --- Function Prototypes ---
void calibrateSensors();
void readSensors();
void detectTap();
void detectTilt(); 
void detectShake(); 
void detectUpsideDownReset(); 
void updateTimers();
void drawPageDots();
void resetAllData(); 
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
  
  delay(1000);
}

void loop() {
  readSensors();      
  detectTap(); 
  detectTilt(); 
  detectShake(); 
  detectUpsideDownReset(); 
  
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

  // Save to persistent memory every 60 seconds
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

    // 1. MODE NAME (Centered at the top, no uptime text anymore)
    u8g2.setFont(u8g2_font_8x13_mr);
    int textWidth = strlen(modeNames[currentMode]) * 8; 
    u8g2.drawStr((128 - textWidth) / 2, 10, modeNames[currentMode]);
    
    u8g2.drawLine(0, 16, 128, 16); 

    // 2. STATISTICS SCREEN (Mode 7)
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

    // 3. TIMER DISPLAY (Modes 1 to 6)
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
      
      if (currentMode == 1) sessionStudySec = 0; 
      else if (currentMode == 2) distractionSec = 0; 
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
      Serial.println("-> SHAKE DETECTED! Current mode timer reset.");
    }
  }
}

void detectUpsideDownReset() {
  // Device is upside down when Z axis is approximately -1.0
  if (accelZ < -0.7) { 
    if (!isUpsideDown) {
      isUpsideDown = true;
      upsideDownStartTime = millis();
    } else {
      // If held upside down for 5 seconds (5000 ms)
      if (millis() - upsideDownStartTime > 5000) {
        resetAllData();
        isUpsideDown = false; // Reset trigger after action
      }
    }
  } else {
    isUpsideDown = false; // Cancel if device is righted
  }
}

void resetAllData() {
  // 1. Clear persistent memory
  preferences.begin("study-data", false);
  preferences.putULong("totalStudy", 0);
  preferences.putULong("distTime", 0);
  preferences.putULong("tytTime", 0);
  preferences.putULong("aytTime", 0);
  preferences.putULong("pomTime", 0);
  preferences.end();

  // 2. Reset live variables
  totalStudySec = 0;
  sessionStudySec = 0;
  distractionSec = 0;
  tytElapsedSec = 0;
  aytElapsedSec = 0;
  pomodoroElapsedSec = 0;
  
  pomodoroRemainingSec = pomodoroWork * 60;
  isPomodoroBreak = false;
  currentPomodoroCycle = 1;

  // 3. Show confirmation on screen
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_8x13_mr);
  u8g2.drawStr(0, 25, "ALL DATA RESET!");
  u8g2.drawStr(0, 45, "FACTORY DEFAULT");
  u8g2.sendBuffer();
  delay(2000);
  
  Serial.println("-> ALL DATA RESET VIA UPSIDE-DOWN GESTURE");
}

String formatTime(long totalSeconds) {
  int h = totalSeconds / 3600;
  int m = (totalSeconds % 3600) / 60;
  int s = totalSeconds % 60;
  char buf[10];
  sprintf(buf, "%02d:%02d:%02d", h, m, s);
  return String(buf);
}
