# Flopper

**Flopper** is a smart productivity cube designed to make focus and habit tracking more engaging.

Each face of the cube represents a different activity. Simply rotate the cube to switch modes, and Flopper automatically tracks your time, rewards XP, and synchronizes your progress with a web dashboard.

## Features

* 🎲 Physical productivity cube interface
* 📚 Multiple activity modes

  * Study
  * Coding
  * Reading
  * Break
  * Exercise
  * Free Time
* ⏱️ Automatic time tracking
* ⭐ XP and gamification system
* 📊 Web dashboard integration
* 📡 Wi-Fi synchronization
* 🔋 Portable battery-powered design

## Hardware

| Component                  | Purpose               |
| -------------------------- | --------------------- |
| Seeed Studio XIAO ESP32-C6 | Main controller       |
| MPU6050 (GY-521)           | Orientation detection |
| 1.3" SH1106 OLED Display   | User interface        |
| 1S LiPo Battery            | Portable power        |
| Perfboard                  | Custom assembly       |

## How It Works

Flopper uses an MPU6050 IMU sensor to determine which face of the cube is currently facing upward. Each face corresponds to a predefined activity mode.

When the cube is rotated:

1. The active mode changes.
2. Time tracking starts for the selected activity.
3. XP is awarded based on time spent.
4. Data is sent to the web dashboard for visualization and analytics.

## Why Flopper?

Many productivity tools require constant interaction with a phone or computer. Flopper provides a tangible and distraction-free way to track habits and stay focused.

The goal is simple:

> Flip the cube. Start working.

## Roadmap

* [ ] Web dashboard
* [ ] User accounts
* [ ] Daily streaks
* [ ] Achievement system
* [ ] Pomodoro mode
* [ ] Statistics and analytics
* [ ] Mobile app integration

## License

This project is licensed under the Apache License 2.0.
