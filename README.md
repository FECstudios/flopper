# Flopper

**Flopper** is a smart productivity gadget designed to make focus and habit tracking more engaging.

## Features

* 📖 Physical productivity interface
* 📚 Multiple activity modes
  * Study
  * Coding
  * Reading
  * Break
  * Exercise
  * Free Time
    
* ⏱️ Automatic time tracking
* 📊 Web dashboard integration
* 📡 Wi-Fi synchronization
* 🔋 Portable battery-powered design
* ⌚ Test time tracking

## Hardware

| Component                  | Purpose               |
| -------------------------- | --------------------- |
| Seeed Studio XIAO ESP32-C6 | Main controller       |
| MPU6050 (GY-521)           | Orientation detection |
| 1.3" SH1106 OLED Display   | User interface        |
| 1S LiPo Battery            | Portable power        |
| Buzzer                     | Alarm system          |
| Rotary encoder             | Setting the time      |

## How It Works

Flopper uses an MPU6050 IMU sensor and its rotation encoder to track your studying time and habbits

When the encoder is turned:

1. The active mode changes.
2. Time tracking starts for the selected activity.
3. Data is sent to the web dashboard for visualization and analytics.

## Why Flopper?

Many productivity tools require constant interaction with a phone or computer. Flopper provides a tangible and distraction-free way to track habits and stay focused.

The goal is simple:

> Turn the encoder. Start working.

## Future Roadmap

* [ ] User accounts
* [ ] Daily streaks
* [ ] Achievement system
* [ ] Pomodoro mode
* [ ] Statistics and analytics

## License

This project is licensed under the Apache License 2.0.
