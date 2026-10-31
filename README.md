# SMART-EV-CHARGER-TRACKER-BLYNK-WOKWII-
# EV Range Monitoring System (ESP32 + GPS + MPU6050 + Blynk)

An IoT-based electric vehicle (EV) monitoring system that estimates driving range, tracks location, and warns the driver about **range anxiety** when the remaining battery may not be enough to reach the nearest charging station. Built on an ESP32 and simulated in Wokwi, with a live dashboard on Blynk IoT.

![Platform](https://img.shields.io/badge/platform-ESP32-blue)
![Simulator](https://img.shields.io/badge/simulator-Wokwi-green)
![Cloud](https://img.shields.io/badge/cloud-Blynk-orange)

---

## Features

- **GPS tracking** – Reads NMEA sentences from a NEO-6M GPS module (custom Wokwi chip) using TinyGPS++.
- **Speed estimation** – Integrates MPU6050 Y-axis acceleration to estimate vehicle speed (km/h).
- **Battery monitoring** – Potentiometer on GPIO36 simulates the battery level (0–100%).
- **Range prediction** – Speed-dependent energy consumption model to estimate remaining range.
- **Regenerative braking** – Hard deceleration (< -1.5 m/s²) lowers the consumption rate.
- **Distance to charger** – Calculates distance from the current GPS position to a predefined charger.
- **Range anxiety alert** – Warns when predicted range < distance to charger + 10 km safety buffer.
- **Charger proximity alert** – Notifies when the charger is within 0.5 km.
- **Motion-aware GPS** – GPS is only parsed while the vehicle is moving (> 0.5 km/h), with a signal-loss warning after 5 s.
- **Remote ON/OFF** – System can be paused or resumed from the Blynk app.
- **Performance analysis** – Reports loop latency, Wi-Fi RSSI (reliability), and 1 Hz transmission rate (energy efficiency).

---

## Hardware Components

| Component | Purpose |
|---|---|
| ESP32 DevKit V1 | Main microcontroller with Wi-Fi |
| GPS NEO-6M (custom Wokwi chip) | Location data via UART |
| MPU6050 | Accelerometer for speed and braking detection |
| Potentiometer | Simulates battery level |

---

## Wiring

| Device | Pin | ESP32 Pin |
|---|---|---|
| GPS NEO-6M | VCC | 3V3 |
| | GND | GND |
| | TX | GPIO16 (RX2) |
| | RX | GPIO15 |
| MPU6050 | VCC | 3V3 |
| | GND | GND |
| | SDA | GPIO21 |
| | SCL | GPIO22 |
| Potentiometer | VCC | 3V3 |
| | GND | GND |
| | SIG | GPIO36 (VP) |

---

## Blynk Dashboard (Virtual Pins)

| Virtual Pin | Data | Widget suggestion |
|---|---|---|
| V0 | Battery (%) | Gauge |
| V1 | Predicted range (km) | Value display |
| V2 | Distance to charger (km) | Value display |
| V3 | Speed (km/h) | Gauge |
| V4 | Range anxiety alert (0/1) | LED (red) |
| V5 | Near charger (0/255) | LED (green) |
| V6 | System ON/OFF | Switch |
| V7 | System latency (ms) | Value display / chart |

---

## How It Works

1. **Battery** – `batteryPercent = (analogRead(36) / 4095) × 100`
2. **Speed** – `speed += accelY × dt × 3.6` (only when |accelY| > 0.5 m/s²)
3. **Consumption rate** – `0.14 + speed × 0.0012` (%/km), clamped between 0.1 and 2.0; drops to 0.05 during regenerative braking
4. **Predicted range** – `batteryPercent / consumptionRate`
5. **Distance to charger** – Euclidean distance between GPS position and charger coordinates (× 111 km/degree, amplified ×50 so the short simulated route produces visible changes)
6. **Alerts** – Range alert if `range < distance + 10 km`; proximity alert if `distance ≤ 0.5 km`

Data is printed to the Serial Monitor and pushed to Blynk once per second.

---

## Libraries

- TinyGPSPlus
- Adafruit MPU6050
- Adafruit Unified Sensor
- Blynk

---

## Getting Started

### Run in Wokwi (simulation)
1. Open the project: https://wokwi.com/projects/467365237088312321
2. Set your own Blynk credentials in `ESP32_GPS.ino`:
   ```cpp
   #define BLYNK_TEMPLATE_ID   "YOUR_TEMPLATE_ID"
   #define BLYNK_TEMPLATE_NAME "YOUR_TEMPLATE_NAME"
   #define BLYNK_AUTH_TOKEN    "YOUR_AUTH_TOKEN"
   ```
3. Start the simulation.
4. Turn on the **V6** switch in the Blynk app to start monitoring.
5. Adjust the potentiometer (battery) and the MPU6050 acceleration to test different scenarios.

### Blynk setup
1. Create a template in [Blynk Console](https://blynk.cloud/).
2. Add datastreams V0–V7 as listed in the table above.
3. Build a dashboard with the suggested widgets.
4. Create a device from the template and copy its auth token into the code.

---

## Project Structure

```
├── ESP32_GPS.ino          # Main firmware
├── diagram.json           # Wokwi circuit diagram
├── gps-neo6m.chip.c       # Custom GPS chip (sends simulated NMEA data)
├── gps-neo6m.chip.json    # Custom chip pin definition
├── libraries.txt          # Wokwi library list
└── README.md
```

---

## Notes

- GPS data is hypothetical, looping through 25 coordinate points near the predefined charger.
- The battery is simulated with a potentiometer; a real build would use a voltage divider on the battery pack.

---
