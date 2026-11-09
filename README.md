# Krestel — ESP32 Non-Invasive Energy Submeter

**Krestel** is an ESP32-based energy monitor that optically reads the pulse LED
of a household electricity meter, counts pulses, converts them to kWh, and
serves a local web dashboard over WiFi.

The target installation is a Dragonfly HGL-888 submeter (230–240 V, 10(60) A,
1600 imp/kWh) in a rented unit. Pulses come from an LM393 phototransistor
module taped over the meter's pulse LED, so nothing is wired into the mains.

---

## Current position

Krestel is a working bench prototype with a real dashboard and a partially
finished baseline feature. The physical install has not happened yet.

**Working**

- Falling-edge interrupt pulse counting on GPIO 4, with a 50 ms debounce and
  `IRAM_ATTR` ISR. Bench-validated at 1:1 counting.
- kWh derived from the pulse count (`totalPulses / pulseConstant`) and a cost
  estimate from a configurable flat utility rate.
- Persistence in ESP32 NVS (`Preferences`, namespace `sensorData`) for pulse
  count, pulse constant, utility rate, and the two baseline values.
- WiFi connection with a 20-second timeout, falling back to an access point
  named `Krestel-Setup` (password `Krestel123`).
- LittleFS + ESPAsyncWebServer: static dashboard from `/dashboard/`, plus
  `GET /api/metrics`, `GET /api/config`, `POST /api/pulse-config`, and
  `POST /api/rate-config`.
- Dashboard UI: dark/light theme, timeframe switcher, SVG chart with tooltips,
  pulse-constant and rate forms wired to the API, cumulative-pulse readout,
  onboarding dialog, and a first-run baseline form.

**In progress**

- Baseline reading endpoint. `receiveBaseline()` exists in
  `src/web/handlers.cpp`, `lib/Storage` already persists
  `baselineKwh`/`baselinePulses`, but the handler is incomplete and does not
  compile; no route is registered; the dashboard still posts JSON to
  `/api/reading`, which currently 404s. See **Known gaps**.
- `lib/Meter/` — empty placeholder for the refactor that will move meter state
  and kWh derivation out of `PulseSensor`.

**Not started**

- Timeframe, daily-average, projection, and chart data. Those cards and the
  chart render `--`; `dataByTimeframe` in `script.js` is placeholder-only.
- Physical install: wire and crimping tool not yet bought, LM393 onboard
  PWR/DO LEDs not yet disabled, optical baffle not fitted, housing is printed.
- Soak test against the meter's own register.

---

## Hardware

| Component | Notes |
|---|---|
| **ESP32 dev board** | `esp32dev`; WiFi and LittleFS are both required |
| **LM393 phototransistor comparator module** | Optical read of the meter's pulse LED; digital `DO` output to the ESP32 |
| **Dragonfly HGL-888 submeter** | 230–240 V, 10(60) A, 1600 imp/kWh; source of the pulses |
| **Status LED (optional)** | GPIO 18; blinks as a visual activity indicator |
| **3D-printed PETG housing** | Black; mounts the module over the meter's LED |

## Wiring

| Signal | ESP32 pin | Defined in |
|---|---|---|
| Sensor `DO` (pulse output) | GPIO 4 | `SENSOR_PIN`, `lib/PulseSensor/pulseSensor.h` |
| Status LED | GPIO 18 | `LED_PIN`, `lib/PulseLEDSim/pulseLEDSim.h` |

The interrupt is attached on `FALLING`; the pulse is active-low.

---

## Project structure

```
.
├── include/
│   ├── header.h              # Umbrella header: framework + project headers
│   └── README                # What belongs in include/
├── lib/
│   ├── Display/              # I2C LCD driver — dormant, not built by main.cpp
│   ├── Meter/                # Empty placeholder for the meter refactor
│   ├── PulseLEDSim/          # Status LED blink pattern
│   ├── PulseSensor/          # Interrupt counting, kWh/rate derivation, globals
│   ├── Storage/              # Preferences (NVS) persistence
│   └── README                # Library index and per-library notes
├── src/
│   ├── main.cpp              # setup()/loop()
│   └── web/
│       ├── server.h          # WiFi + web server declarations
│       ├── server.cpp        # WiFi/AP fallback, LittleFS mount, route table
│       ├── handlers.h        # API handler declarations
│       ├── handlers.cpp      # metrics/config/pulse-config/rate-config handlers
│       ├── credentials.h     # WiFi credentials (gitignored)
│       └── credentials.h.example
├── data/dashboard/           # Uploaded to LittleFS, not compiled into firmware
│   ├── index.html            # Single-page dashboard
│   ├── styles.css            # Dashboard styles
│   └── script.js             # Polling, forms, chart, theme, onboarding
├── test/
│   └── README                # Test runner dir — currently no tests
├── platformio.ini
├── PLAN.md                   # Design notes and work order (draft)
├── LICENSE
└── README.md
```

---

## How it works

### Pulse counting

`initSensor()` configures `SENSOR_PIN` as an input and attaches `readPulse()` on
the falling edge. The ISR:

1. Reads `millis()` and increments `rawFires` (every firing, debounced or not).
2. Returns immediately if the edge arrived within `DEBOUNCE_MILLIS` (50 ms) of
   the last accepted pulse.
3. Otherwise records the time and increments `totalPulses`.

`totalPulses` is the only source of truth for energy; kWh is always derived,
never persisted.

### Safe reads

`loop()` calls `updateSafeKwh()` and `updateSafeRate()`, which snapshot the
volatile counter with interrupts disabled and publish:

```
safeKwhRead         = totalPulses / pulseConstant
safeEstimateReading = utilityRate * safeKwhRead
```

The web handlers read those snapshots rather than the ISR-written globals.

### Persistence

`initStorage()` opens `Preferences` namespace `sensorData` and restores the
pulse count, pulse constant, utility rate, and baseline into the globals. Note
the read fallbacks: `totalPulses` 0, `pulseConstant` **1000**, `utilityRate`
**10.0**, `baselineKwh` 0.0, `baselinePulses` 0. On a fresh device the effective
constant is therefore 1000, not `DEFAULT_PULSE_CONSTANT` (1600).

`loop()` then calls `savePulseCount(totalPulses)` on every iteration, which is
every ~220 ms because `pulseLED()` blocks for that long.

### Serving

`server.cpp` connects to WiFi (40 × 500 ms attempts). On failure it switches to
AP mode and serves at `192.168.4.1`. LittleFS is mounted with format-on-fail, and
`/` is served statically from `/dashboard/` with `index.html` as the default
file. There is no template processor, so the `%IP_ADDRESS%` / `%POWER_NOW%`
placeholders in `index.html` are replaced at runtime by `script.js` from the API
responses.

---

## Configuration

Firmware constants:

| Constant | Value | Where | Meaning |
|---|---|---|---|
| `SENSOR_PIN` | 4 | `lib/PulseSensor/pulseSensor.h` | Pulse input GPIO |
| `DEBOUNCE_MILLIS` | 50 | `lib/PulseSensor/pulseSensor.h` | Minimum ms between accepted pulses |
| `DEFAULT_PULSE_CONSTANT` | 1600 | `lib/PulseSensor/pulseSensor.h` | Fallback pulses per kWh (overridden on boot by the NVS read default of 1000) |
| `DEFAULT_RATE` | 10.5 | `lib/PulseSensor/pulseSensor.h` | Fallback currency unit per kWh (overridden by the NVS default of 10.0) |
| `LED_PIN` | 18 | `lib/PulseLEDSim/pulseLEDSim.h` | Status LED GPIO |

Runtime-configurable values (persisted under `sensorData`):

| Key | Type | Default on first boot | Set by |
|---|---|---|---|
| `totalPulses` | `ULong` | 0 | `loop()` |
| `pulseConstant` | `UShort` | 1000 | `POST /api/pulse-config` |
| `utilityRate` | `Float` | 10.0 | `POST /api/rate-config` |
| `baselineKwh` | `Double` | 0.0 | `POST /api/baseline` (not routed yet) |
| `baselinePulses` | `ULong` | 0 | `POST /api/baseline` (not routed yet) |

WiFi credentials: copy `src/web/credentials.h.example` to
`src/web/credentials.h` and fill in `WIFI_SSID` / `WIFI_PASSWORD`. That file is
gitignored.

---

## Building & uploading

1. Install [PlatformIO](https://platformio.org/) (VS Code extension or CLI).
2. Create `src/web/credentials.h` from `credentials.h.example`.
3. Upload the dashboard files to LittleFS:

   ```bash
   pio run -e esp32 -t uploadfs
   ```

4. Build and flash the firmware:

   ```bash
   pio run -e esp32 -t upload
   ```

5. Watch the boot log at 115200 baud:

   ```bash
   pio device monitor -e esp32
   ```

The serial log prints the loaded values and the assigned IP address. If WiFi
fails, connect to `Krestel-Setup` and browse to `192.168.4.1`.

`platformio.ini` also still contains an `[env:uno]` block (ATmega328P with
`LiquidCrystal_I2C`) left over from the early bench build. It is not usable
today: `include/header.h` pulls in `WiFi.h` and `ESPAsyncWebServer.h`.

---

## Web dashboard

`data/dashboard/` is served from flash. UI-only changes need `uploadfs` but no
firmware rebuild.

Dashboard features wired to the device:

- **Current Meter Reading** card — shows `kwh` from `/api/metrics` (device
  relative, i.e. `totalPulses / pulseConstant`) and the cumulative pulse count.
- **Pulse constant** form — steps in 100 imp/kWh increments, posts to
  `/api/pulse-config`.
- **Utility rate** form — posts to `/api/rate-config`, then recomputes cost per
  pulse / hour / projected spend in the page.
- Theme toggle, sync button, and a 2-second metrics poll.

Still placeholder: the timeframe switcher (`Used Today/Week/Month/Billing`), the
daily-average and projected-spend cards, the progress bar, and the entire SVG
chart series. The "Live WS" pill is cosmetic — data is fetched by polling.

`index.html` also links `pico.min.css`, which is not in the repository yet. Copy
it into `data/dashboard/` before uploading, or the page will render unstyled.

### API

| Endpoint | Method | Body / response |
|---|---|---|
| `/` | GET | Dashboard static files from LittleFS |
| `/api/metrics` | GET | `pulses`, `kwh`, `since_baseline_kwh`, `meter_kwh`, `has_baseline`, `uptime` (seconds) |
| `/api/config` | GET | `constant`, `rate`, `baseline_kwh`, `baseline_pulses`, `ip_address` |
| `/api/pulse-config` | POST | form field `pulse-config` (urlencoded); `0`/missing → 400 |
| `/api/rate-config` | POST | form field `rate-config` (urlencoded); `0`/missing → 400 |
| `/api/baseline` | POST | declared in `handlers.h`, **not registered** on the server |
| `/api/reading` | POST | posted by the dashboard as JSON, **not implemented** (404) |

Unknown routes return `404 text/plain`.

---

## Library reference

| Library | Public surface |
|---|---|
| `PulseSensor` | `initSensor()`, `readPulse()` (ISR), `updateSafeKwh()`, `updateSafeRate()`; globals `totalPulses`, `rawFires`, `lastPulseTime`, `baselinePulses`, `baselineKwh`, `pulseConstant`, `utilityRate`, `safeKwhRead`, `safeEstimateReading` |
| `Storage` | `initStorage()`; `savePulseCount()`, `savePulseConstant()`, `saveUtilityRate()`, `saveBaseline(kwh, pulses)`; matching read functions |
| `PulseLEDSim` | `initLED()`, `pulseLED()` — two 5 ms blinks then a 200 ms pause |
| `Display` | `initDisplay()`, `displayData()` — parked; references a removed `PULSE_CONSTANT` |
| `Meter` | Empty; reserved for the meter-state refactor |

See `lib/README` for the per-library details and `include/README` for header
conventions.

---

## Calibration

1. Find your meter's `imp/kWh` rating — the HGL-888 is 1600.
2. Enter it in the dashboard's pulse-constant form, or change the NVS
   fallbacks in `lib/Storage/storage.cpp` if you want it correct on first boot.
3. With a known load, compare the dashboard's kWh delta against the meter's own
   register over a fixed interval; adjust if needed.

Because a baseline reading is typed in after the fact, pulses that occur while
you are entering the number get counted as post-baseline consumption. At 2.4 kW
that is roughly 0.04 kWh per minute of typing — below a single 1600 imp/kWh
step (0.000625 kWh), so no clock or timestamp is needed for it.

---

## Known gaps

These are the concrete things standing between the bench prototype and a
trustworthy submeter.

- **The baseline handler does not compile.** `receiveBaseline()` uses
  `currentKwh`, which is local to `sendMetrics()`; calls
  `saveBaseline(newBaseline)` although it takes two arguments; and calls
  `isDecimal(param->value())` while the helper is declared as
  `isDecimal()` with an empty body. Its numeric check is also inverted — it
  replies 400 when the value *is* numeric.
- **The baseline contract disagrees end to end.** The handler reads the form
  field `baseline-kwh`; the header comment and route name say `/api/baseline`;
  the dashboard posts JSON to `/api/reading`. One format has to win, and the
  route still has to be registered.
- **Metrics emits uninitialized values.** `sendMetrics()` declares
  `sinceBaselineKwh`, `meterKwh`, and `hasBaseline` but never assigns them, so
  those JSON fields are garbage and the absolute register cannot be shown yet.
- **NVS wear.** `savePulseCount()` runs every loop iteration (~220 ms). That is
  far too often for flash; batch writes to every N pulses or on a timer.
- **Dashboard placeholders.** Timeframe, daily average, projection, and chart
  data have no data source. A baseline alone does not create history.
- **Missing `pico.min.css`.** Referenced by `index.html`, absent from
  `data/dashboard/`.
- **Duplicate metric semantics.** `kwh` is device-relative; the "Current Meter
  Reading" card needs `meter_kwh` from the baseline, not `kwh`.
- **`console.Error(err)`** in the pulse-config save path throws a `TypeError`
  inside its own `catch`, turning a handled network failure into an unhandled
  one.
- **Sensor module LEDs.** The LM393 board's PWR/DO LEDs can contaminate the
  optical reading; the DO LED trace must be cut and a baffle fitted before the
  final assembly.
- **Case-sensitive checkouts.** The repository's index has held both
  `PulseSensor.h` and `pulseSensor.h` because `core.ignorecase=true`; a
  case-sensitive clone may fail to resolve includes. `lib/Display/display.h`
  still includes `"PulseSensor.h"`.
- **No tests.** See `test/README` for the intended first targets.

---

## Roadmap

From `PLAN.md`, in order: finish and route the baseline endpoint, replace the
placeholder dashboard data, complete the physical install (wire, crimper,
baffle, module LED mod), run a multi-day soak test against the meter register,
then decide on reconciliation and further hardware work.

---

## License

See `LICENSE`.
