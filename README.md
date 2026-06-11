# Flopper
**A physical study tracker that lives on your desk, not your phone.**

<img width="397" height="609" alt="image" src="https://github.com/user-attachments/assets/7e26a6cb-287f-4ab8-ba36-4694bb92e77e" />

We built Flopper because we kept losing track of how long we actually studied vs. how long we *thought* we studied. It's a small 3D-printed device with an OLED screen, a rotary encoder, and an IMU sensor. Shake it to start a session, rotate the encoder to switch between modes (study, coding, break, etc.), and open a browser to see your stats on a dashboard the ESP32 hosts itself — no internet, no app, no account.

Three 14-year-olds from Turkey made this. It fits in your palm.

---

## Gallery

<p align="center">
  <img src="https://github.com/user-attachments/assets/3692959e-49ab-4fb8-85d9-1a0f0a466c2a"
       width="250"
       height="250"
       style="object-fit: cover;" />

  <img src="https://github.com/user-attachments/assets/2f544b1e-6bd7-4126-8eb8-e83cceeff3e7"
       width="250"
       height="250"
       style="object-fit: cover;" />

  <img src="https://github.com/user-attachments/assets/8b9bfd99-fc73-4234-ace0-a1fbbda55781"
       width="250"
       height="250"
       style="object-fit: cover;" />
</p>

---

## What it does

- Shake the device (or click the encoder) → session starts
- Rotate the encoder → switch activity mode
- Timer runs, buzzer goes off when time's up → shake again to dismiss
- Open `192.168.x.x` on any device on the same Wi-Fi → see your session history live

### Modes
Study · Coding · Reading · Break · Exercise · Free Time

---

## Hardware

| Component | What it's for |
|---|---|
| Seeed Studio XIAO ESP32-C6 | Runs everything, hosts the web server |
| MPU6050 (GY-521) | Detects shakes and tilts |
| 1.3" SH1106 OLED | Shows the current mode and timer |
| Rotary encoder | Scroll through modes, click to start |
| Buzzer | Alarm when session ends |
| 1S LiPo | Battery power |

---

## How it works

The MPU6050 constantly reads acceleration. When it detects a shake above a threshold, it starts the timer for whatever mode is selected. The rotary encoder lets you scroll through modes on the OLED before you start — or you can just click it instead of shaking if you want.

When the session ends, the buzzer fires. Shake or click to stop it.

The ESP32 runs a small web server over Wi-Fi. Once it connects to your network it shows its local IP on the OLED. Open that in a browser and you get a dashboard with your session history and time per mode.

---

## Setup

**You'll need:**
- Arduino IDE 2.x
- ESP32 board package
- Libraries: `Adafruit SH110X`, `MPU6050_light`, `ESPAsyncWebServer`

**Steps:**
1. Clone the repo
2. Open `flopper/online.ino` or `flopper/offline.ino`
3. Change the Wi-Fi credentials near the top if you prefer the online version:
   ```cpp
      const char* ssid = "YOUR_SSID";             
      const char* wifiPassword = "YOUR_PASSWORD"; 
   ```
4. Flash to XIAO ESP32-C6
5. IP address shows on the OLED once it connects — open it in a browser

---

## Why we made it

We wanted something physical. Every productivity app we tried was on the same device we were trying to avoid. With Flopper you just pick it up and shake it — that's the whole interaction. No screen to unlock, no notifications to scroll past.

Also we wanted to see if we could fit a display, motion sensor, encoder, Wi-Fi, and battery into something 3D-printed that actually looked decent.

---

## Roadmap

- [ ] Daily streaks
- [ ] Pomodoro mode
- [ ] Better analytics on the dashboard
- [ ] OTA updates
- [ ] Achievement system

---

## License

[Apache 2.0](LICENSE)
