# Krestel — Non-Invasive Energy Submeter

**Krestel** is an Arduino/ESP32-based energy monitoring system that tracks electrical consumption through a non-invasive pulse sensor, computes kilowatt-hour totals from pulse counts, and exposes a live dashboard via a built-in web interface.

---

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Hardware Requirements](#hardware-requirements)
- [Supported Boards](#supported-boards)
- [Project Structure](#project-structure)
- [How It Works](#how-it-works)
- [Configuration](#configuration)
- [Building & Uploading](#building--uploading)
- [Web Dashboard](#web-dashboard)
- [Library Reference](#library-reference)
- [Calibration](#calibration)
- [Notes & Limitations](#notes--limitations)

---

## Overview

Krestel hooks into the pulse output of a utility-grade energy sensor (or a simulated pulse source during testing). Each pulse represents a fixed fraction of a kilowatt-hour (default: 1 pulse per 1/1600 kWh). The firmware:

1. Counts pulses via a hardware interrupt.
2. Debounces spurious edges.
3. Persists the total pulse count across reboots using EEPROM.
4. Blinks an LED as a visual pulse indicator.
5. Serves a polished HTML dashboard via LittleFS and ESPAsyncWebServer for remote monitoring over WiFi.

The end goal is a submeter that can estimate electricity bills from real consumption data without breaking the electrical circuit, accessible from any device on your network.

---

## Features

- **Interrupt-driven pulse counting** — accurate, low-latency detection on the chosen sensor pin.
- **Debouncing** — ignores edges within 50 ms of each other to avoid noise inflation.
- **Safe read of volatile kWh data** — `updateSafeKwh()` snapshots the volatile counter under `noInterrupts()` so the web API can read a stable value.
- **EEPROM persistence** — pulse count survives power cycles; first boot initializes EEPROM with a magic flag.
- **Visual pulse LED** — GPIO 13 blinks to indicate pulse activity, useful for debugging.
- **ESP32 web dashboard** — a full-featured single-page dashboard (Pico CSS + Material Symbols) served from LittleFS with JSON API endpoints for live consumption data.
- **LittleFS file serving** — dashboard files (HTML, CSS, JS) are stored in flash and served by ESPAsyncWebServer, with separate JSON endpoints for metrics and chart data.

---

## Hardware Requirements

| Component | Notes |
|---|---|
| **Microcontroller** | ESP32 dev board (WiFi + LittleFS required for dashboard) |
| **Photodiode Sensor Module** | A sensor that detects a pulse output proportional to kWh (e.g., a photoresistor/photodiode reading an optical meter output, or a dedicated CT-based module with pulse out) |
| **LED (optional)** | Connected to GPIO 13 (default), or whatever `LED_PIN` is set to |
| **EEPROM** | Built-in on the microcontroller |

---

## Supported Boards

| Environment | Platform | Framework | Extra deps |
|---|---|---|---|
| `esp32` | ESP32 (esp32dev) | Arduino | `ESPAsyncWebServer` |

Build and upload:

```bash
pio run -e esp32 -t upload
```


## Project Structure

```
.
├── include/
│   └── header.h              # Shared includes and forward declarations
├── lib/
│   ├── PulseLEDSim/
│   │   ├── pulseLEDSim.h
│   │   └── pulseLEDSim.cpp   # LED pulse simulation (debug aid)
│   ├── PulseSensor/
│   │   ├── pulseSensor.h
│   │   └── pulseSensor.cpp   # Interrupt-based pulse counting + debounce
│   ├── Storage/
│   │   ├── storage.h
│   │   └── storage.cpp       # EEPROM init/save/restore of pulse count
│   └── README
├── src/
│   ├── main.cpp              # setup()/loop() — ties all libraries together
│   ├── config.h              # Pin definitions, constants
│   └── web/
│       ├── server.h          # WiFi config, web server declarations
│       ├── server.cpp        # ESPAsyncWebServer + LittleFS + JSON API
│       ├── credentials.h     # WiFi credentials (gitignored)
│       └── credentials.h.example  # Template for credentials.h
├── data/
│   └── dashboard/
│       ├── index.html        # Dashboard markup (uploaded to LittleFS)
│       ├── pico.min.css      # Pico CSS v2 (vendored framework)
│       ├── styles.css        # Dashboard styles
│       └── script.js         # Dashboard interactivity
├── platformio.ini
├── .gitignore
├── LICENSE
└── README.md
```

---

## How It Works

### Pulse counting

The sensor is wired to `SENSOR_PIN` (default: GPIO 2). An interrupt fires on the **falling edge** of the signal, calling `readPulse()`. The handler:

1. Records the current `millis()`.
2. Increments `rawFires`.
3. Rejects the pulse if fewer than `DEBOUNCE_MILLIS` (50 ms) have elapsed since the last valid pulse.
4. On a valid pulse, increments `totalPulses` and recomputes `kwh = totalPulses / PULSE_CONSTANT`.

### Safe read

Because `kwh` is `volatile` and updated in an ISR, the main loop never reads it directly. Instead:

- `updateSafeKwh()` copies `kwh` into `safeKwhRead` with interrupts disabled.
- The web API reads `safeKwhRead` to serve stable values to the dashboard.

### Persistence

On boot, `initStorage()` checks a magic byte at EEPROM address 0. If it doesn't match `EEPROM_INIT_MAGIC` (0xA5), the pulse count is reset to 0 and the magic is written. Otherwise the saved count at address 1 is loaded back into `totalPulses`.

---

## Configuration

Key constants live in `lib/PulseSensor/pulseSensor.h`:

| Constant | Default | Meaning |
|---|---|---|
| `SENSOR_PIN` | 2 | GPIO connected to the sensor pulse output |
| `PULSE_CONSTANT` | 1600 | Pulses per kWh (adjust to match your meter's `imp/kWh` rating) |
| `DEBOUNCE_MILLIS` | 50 | Minimum ms between valid pulses |

LED pin is set in `lib/PulseLEDSim/pulseLEDSim.h`:

| Constant | Default | Meaning |
|---|---|---|
| `LED_PIN` | 13 | GPIO driving the debug/status LED |

WiFi credentials are set in `src/web/credentials.h` (gitignored). Copy `credentials.h.example` to `credentials.h` and fill in your network details.

EEPROM addresses are defined in `lib/Storage/storage.h`:

| Macro | Value | Meaning |
|---|---|---|
| `EEPROM_INIT_ADDR` | 0 | Magic-flag byte location |
| `EEPROM_INIT_MAGIC` | 0xA5 | Value that marks EEPROM as initialized |
| `EEPROM_PULSE_ADDR` | 1 | Where the pulse count is stored |

---

## Building & Uploading

1. Install [PlatformIO](https://platformio.org/) (VS Code extension or CLI).
2. Copy `src/web/credentials.h.example` to `src/web/credentials.h` and fill in your WiFi credentials.
3. Open the project folder in VS Code (or run `pio run` from the CLI).
4. Connect your ESP32 via USB.
5. Upload the dashboard files to LittleFS flash:

```bash
pio run -e esp32 -t uploadfs
```

6. Build and upload the firmware:

```bash
pio run -e esp32 -t upload
```

7. Open the serial monitor at 115200 baud to watch debug output:

```bash
pio device monitor -e esp32
```

The device will connect to WiFi (or fall back to AP mode) and serve the dashboard at the IP shown in the serial monitor.

---

## Web Dashboard

The dashboard lives in `data/dashboard/` and is served from the ESP32's flash via LittleFS. It includes:

- A sidebar navigation with Home Energy, Submeters & Devices, and Settings sections.
- Metric cards for **Power Now**, **Used This Month**, **Estimated Month-End Bill**, and **Active Submeters**.
- An animated SVG energy-consumption curve with recorded and forecasted segments.
- Interactive data points that show a tooltip with date, draw, and estimated cost on hover.
- A smart-home tip card recommending off-peak EV charging.

### API Endpoints

The web server (`src/web/server.cpp`) exposes:

| Endpoint | Method | Description |
|---|---|---|
| `/` | GET | Serves the dashboard (HTML/CSS/JS from LittleFS) |
| `/api/metrics` | GET | Returns JSON: `pulses`, `kwh`, `uptime`, `wifi_rssi` |
| `/api/chart` | GET | Returns JSON: chart labels and values |

### Updating the dashboard

Edit the files in `data/dashboard/` (HTML, CSS, JS) as needed, then re-upload to flash:

```bash
pio run -e esp32 -t uploadfs
```

No firmware rebuild is needed for UI-only changes.

---

## Library Reference

### PulseSensor (`lib/PulseSensor/`)

| Symbol | Type | Description |
|---|---|---|
| `SENSOR_PIN` | constexpr uint8_t | Sensor GPIO |
| `PULSE_CONSTANT` | constexpr uint16_t | Pulses per kWh |
| `DEBOUNCE_MILLIS` | const unsigned long | Debounce window |
| `kwh` | volatile unsigned long | Live kWh estimate (ISR-written) |
| `totalPulses` | volatile unsigned long | Total pulses counted since boot/reset |
| `rawFires` | volatile unsigned long | Total ISR firings (including debounced-out ones) |
| `lastPulseTime` | volatile unsigned long | `millis()` of last valid pulse |
| `safeKwhRead` | unsigned long | Snapshot of `kwh` taken under `noInterrupts()` |
| `initSensor()` | void | Configures pin and attaches interrupt |
| `readPulse()` | void (ISR) | Handles a falling edge |
| `updateSafeKwh()` | void | Copies `kwh` → `safeKwhRead` safely |

### PulseLEDSim (`lib/PulseLEDSim/`)

| Symbol | Description |
|---|---|
| `initLED()` | Sets `LED_PIN` as output |
| `pulseLED()` | Blinks the LED in a double-transition pattern with a 200 ms pause |

This is a simulation/debug aid — it mirrors the pulse cadence visually. Replace it with real hardware linking if your LED should react directly to sensor pulses.

### Storage (`lib/Storage/`)

| Symbol | Description |
|---|---|
| `initStorage()` | On first boot: resets pulse count and writes magic. Otherwise: restores saved count into `totalPulses` |
| `savePulseCount(unsigned long)` | Writes the pulse count to EEPROM |
| `readSavedPulseCount()` | Reads and returns the saved pulse count from EEPROM |

---

## Calibration

Your energy sensor's pulse constant may differ from the default 1600 imp/kWh. To calibrate:

1. Check your meter's datasheet or label for its `imp/kWh` (or `kWh/imp`) rating.
2. Set `PULSE_CONSTANT` in `lib/PulseSensor/pulseSensor.h` to that value.
3. If your sensor outputs a proportional pulse stream (not a fixed imp/kWh), measure a known consumption period, count pulses, and derive `PULSE_CONSTANT = totalPulses / kWh_consumed`.

---

## Notes & Limitations

- **EEPROM wear**: `savePulseCount()` writes on every loop iteration (every ~255 ms). For long-term deployment, consider throttling writes to, e.g., once per minute or on significant pulse-count changes to reduce EEPROM wear.
- **Single-zone metering**: the current firmware tracks one pulse source. Multi-submeter support (shown in the dashboard UI) requires extending pulse counting and storage to multiple channels.
- **Interrupt safety**: `kwh` and `totalPulses` are `volatile` and updated in an ISR. Any new code reading them directly must either disable interrupts or use `updateSafeKwh()`/equivalent.
- **WiFi credentials**: never commit `src/web/credentials.h`. It is gitignored. Use `credentials.h.example` as a template.

---

## License

See `LICENSE`.
