# Flopper

> A distraction-free habit tracker you can hold in your hand.

**Flopper** is a physical productivity gadget that makes focus tracking tangible and fun. Shake it to start a session, turn the encoder to switch modes, and watch your study habits build up on a live web dashboard — all without touching your phone.

Built by three 14-year-olds from Turkey who wanted a smarter, more satisfying way to track study time.

---

## Gallery

> 📸 Add your images here!

```
<img width="1530" height="2040" alt="WhatsApp Image 2026-06-07 at 21 51 37 (2)" src="https://github.com/user-attachments/assets/3692959e-49ab-4fb8-85d9-1a0f0a466c2a" />
<img width="728" height="724" alt="Screenshot 2026-06-06 143729" src="https://github.com/user-attachments/assets/2f544b1e-6bd7-4126-8eb8-e83cceeff3e7" />
<img width="584" height="575" alt="Screenshot 2026-06-10 132850" src="https://github.com/user-attachments/assets/8b9bfd99-fc73-4234-ace0-a1fbbda55781" />


---

## Features

- 🤙 **Shake to start** — tilt or shake the device to begin a session (via IMU)
- 🔘 **Encoder click** — alternative way to start/stop without shaking
- 🔄 **Mode switching** — rotate the encoder to pick an activity
- ⏱️ **Automatic time tracking** — starts counting the moment you trigger it
- 📊 **Live web dashboard** — hosted directly on the ESP32, no internet needed
- 📡 **Wi-Fi** — connect from any device on the same network
- 🔔 **Buzzer alarm** — alerts you when a session ends
- 🔋 **Portable** — runs on a 1S LiPo battery

### Activity modes

| Mode | Icon |
|------|------|
| Study | 📖 |
| Coding | 💻 |
| Reading | 📚 |
| Break | ☕ |
| Exercise | 🏃 |
| Free Time | 🎮 |

---

## Hardware

| Component | Purpose |
|---|---|
| Seeed Studio XIAO ESP32-C6 | Main controller + Wi-Fi |
| MPU6050 (GY-521) | Shake & tilt detection |
| 1.3" SH1106 OLED Display | User interface |
| Rotary encoder | Mode selection + click to start |
| Buzzer | Session alarm |
| 1S LiPo Battery | Portable power |

---

## How It Works

Flopper has two ways to interact with it:

### Starting a session

**Option 1 — Shake it:**
The MPU6050 IMU detects motion. Pick up Flopper and give it a shake or tilt — the session starts immediately and the OLED confirms the active mode.

**Option 2 — Click the encoder:**
Press the rotary encoder button. Same result, no motion needed.

### Switching modes

Rotate the encoder to scroll through activity modes shown on the OLED screen. The selected mode is highlighted before you start.

### Session end & alarm

When the timer runs out, the buzzer fires an alarm. Shake the device again (or click) to dismiss it.

### Web dashboard

Flopper hosts a live web server directly on the ESP32. No cloud, no app needed.

1. Connect your phone or laptop to the **same Wi-Fi network** as Flopper
2. Open a browser and go to the IP address shown on the OLED (e.g. `192.168.1.42`)
3. See your session history, time per mode, and daily totals in real time

---

## Getting Started

### What you need

- Arduino IDE (2.x recommended)
- ESP32 board package installed
- Libraries: `Adafruit SH110X`, `MPU6050_light`, `ESPAsyncWebServer`

### Setup

1. Clone this repo
2. Open `flopper/flopper.ino` in Arduino IDE
3. Edit the Wi-Fi credentials at the top of the file:
   ```cpp
   const char* ssid = "YOUR_WIFI_NAME";
   const char* password = "YOUR_WIFI_PASSWORD";
   ```
4. Select **XIAO ESP32-C6** as the board and flash
5. The OLED will show the IP address once connected — open it in a browser

---

## Why Flopper?

Most productivity apps live on the same phone you're trying to avoid. Flopper is a physical object that sits on your desk. You pick it up, shake it, and you're working — no unlock screen, no notifications.

The goal is simple:
> Shake it. Start working.

We also wanted to see how much you could fit into a small 3D-printed enclosure: a display, motion sensor, rotary input, Wi-Fi, and a battery — all in something that fits in your palm.

---

## Future Roadmap

- [ ] User accounts
- [ ] Daily streaks & achievement system  
- [ ] Pomodoro mode
- [ ] Statistics and analytics graphs on dashboard
- [ ] OTA firmware updates

---

## License

This project is licensed under the [Apache License 2.0](LICENSE).
