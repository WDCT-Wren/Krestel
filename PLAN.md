# Krestel — Project Plan (draft, 2026-11-03)

> Reconstructed from project notes, not from your original plan file. Items marked **(verify)** are things I could not confirm are current. Edit freely.

## 1. Goal

Optically read the pulse LED of the apartment's Dragonfly HGL-888 submeter (230–240V, 10(60)A, 1600 imp/kWh), count energy on an ESP32, and serve a single-user web dashboard from the ESP32 over WiFi showing cumulative kWh and cost at a flat landlord rate.

Success criterion you set: genuine hardware competence, not just a working prototype.

Longer-term (not in current scope): scale to a multi-unit utility management system and pitch it to the landlord.

## 2. Architecture (current decisions)

- **Sensing:** phototransistor LM393 comparator module on the Pulse LED, falling-edge interrupt (active-low confirmed), 50 ms debounce, `volatile` + `IRAM_ATTR` ISR.
- **Compute:** only `totalPulses` is persisted; kWh is always derived (`totalPulses / PULSE_CONSTANT`), never stored.
- **Storage:** no database. ESP32 `Preferences` (NVS) holds pulse count, pulse constant, utility rate. Writes batched every ~10–20 pulses (bounded loss on power failure accepted).
- **Serving:** ESPAsyncWebServer + LittleFS. Dashboard files in `data/dashboard/` (Stitch-generated, Pico CSS, single page).
- **Hardware:** 3D-printed black PETG housing, ~2 m AWG wire through the window to the ESP32 (breadboard for now).

## 3. Status

**Working**
- Pulse counting core, bench-validated (1:1 counting, debounce checked against pulse width and 60A inter-pulse interval)
- Persistence via Preferences
- Firmware routes: `GET /api/metrics`, `GET /api/ip_address`, `POST /api/pulse-config`, `POST /api/rate-config`
- Dashboard wiring done by hand: save buttons, rate section, live metrics, IP fetch
- `GET /api/config` + config load on page (notes say finished 2026-10-11 — **verify**, since you've called it the next task)
- Dashboard cleanup: watts removed, "Power Now" relabeled to cumulative meter reading, fake values replaced with `--`

**In progress / not started**
- `POST /api/reading` (baseline sync from onboarding form) — handler is an empty stub in `server.cpp`, not routed, nothing persisted. **Design settled in §8; implementation not started.**
- Timeframe switcher, chart tooltips, onboarding/baseline still use Stitch placeholder data
- Housing printed; wire and crimping tool not yet acquired
- Sensor mounting: adhesive approved by landlord; overhang depth unmeasured **(verify)**

## 4. Known issues and gaps

1. `kwh` typed as `unsigned long`, so the float cast truncates. **(verify whether fixed; may be moot if kWh is now derived only in `updateSafeKwh()`)** — confirmed moot: it is a `double` derived in `updateSafeKwh()`.
2. Reconciliation gap: ESP32 count can drift from the meter's register. No design yet. (Baseline is the first half — see §8.)
3. LM393 onboard PWR/DO LEDs may contaminate optical sensing; disable via cuttable trace before final assembly.
4. Baffling needed to isolate the Pulse LED from the adjacent Rev LED.
5. You haven't read much of the generated `script.js`; unread code is a debugging liability.
6. Batched NVS writes lose up to ~10–20 pulses on power loss (accepted tradeoff).
7. Git index holds both `PulseSensor.h` and `pulseSensor.h` (and both `.cpp` casings) while only one file exists on disk, because `core.ignorecase=true`. `#include "pulseSensor.h"` therefore resolves only on Windows; a case-sensitive checkout would fail to build. Fix in its own commit, not as part of the baseline work.

## 5. Proposed order of work (one thing at a time)

1. **Confirm baseline:** run the current firmware + dashboard; list what actually works end to end.
2. **`GET /api/config`:** finish or re-verify.
3. **`POST /api/reading`:** baseline sync (the reconciliation gap's first half). Design: §8.
4. **Replace placeholder dashboard data:** timeframe switcher, tooltips, onboarding.
5. **Physical install:** buy wire and crimper, disable module LEDs, baffle, mount, test on the real meter.
6. **Soak test:** run for several days, compare dashboard kWh against the meter's displayed register.
7. **Then decide:** reconciliation design, KiCad PCB redesign, scaling work.

## 6. Dropped / changed from earlier plans

- I2C LCD: dropped; ESP32 serves a website instead
- SIM900A / GSM weekly summaries: dropped (2G phase-out)
- Live watts display: dropped
- Project name: Wattrack → Krestel
- Arduino Uno: bench power supply only

## 7. Open questions before restructuring

- What exactly is being restructured: code layout, scope, or the roadmap?
- Is the "6 weeks" from 2026-10-11 still the target? That would end around Nov 22, with a two-week gap in the middle.
- Does the ERD (6 entities, scalable to multiple units) describe the build or a future version? The firmware has no database.
- Which parts must be hand-written for learning, and which can be delegated? (draft rule: hand-write sensing, ISR, config flow; delegate CSS and boilerplate you can fully explain)

---

## 8. Baseline sync — design for `POST /api/reading`

### 8.1 What a baseline actually is

Two numbers captured as one snapshot, and nothing else:

| Value | Meaning | Source |
|---|---|---|
| `baselineKwh` | the utility register exactly as the user types it, e.g. `12450.37` | the form |
| `baselinePulses` | `totalPulses` at the instant the request is handled | the device |

Everything absolute is derived, never stored:

```
meterKwh         = baselineKwh + (totalPulses - baselinePulses) / pulseConstant
sinceBaselineKwh = (totalPulses - baselinePulses) / pulseConstant
```

Do **not** zero `totalPulses` on setup. The counter already holds pre-setup pulses, a second browser could re-post, and clearing it from the HTTP task would race the ISR. A baseline moves the origin; it does not reset the odometer.

### 8.2 Wire format

New route `POST /api/reading`, `application/x-www-form-urlencoded`:

- `reading` — kWh, required, absolute, non-negative decimal
- `date` — `YYYY-MM-DD`, optional metadata (see §8.8)

Rationale: the three existing handlers already use `request->getParam(name, true)` and ESPAsyncWebServer parses urlencoded bodies itself. A JSON body would need a registered `onBody` callback, manual chunk buffering, and a parser dependency to pull out two fields. Not worth it.

**The dashboard currently posts JSON to `/api/reading`.** That fetch must change or every submission 400s — the most likely way to "finish" the handler and still see nothing work.

### 8.3 Handler contract

| Case | Response |
|---|---|
| `reading` param absent | `400 text/plain`, wording matching `handlePulseConfig` |
| `reading` non-numeric, negative, or out of range | `400 text/plain` |
| accepted | `200 text/plain`, body `OK` |

Shape of the handler, in the style of the neighbours already in `server.cpp`:

```cpp
static void handleBaseline(AsyncWebServerRequest *request) {
  const AsyncWebParameter *param = request->getParam("reading", true);
  if (param == nullptr) { request->send(400, "text/plain", "Missing 'reading' parameter"); return; }

  if (!isDecimal(param->value())) {                        // see §8.4
    request->send(400, "text/plain", "Invalid 'reading' value");
    return;
  }
  double reading = param->value().toDouble();
  if (reading > 9999999.0) { request->send(400, "text/plain", "Invalid 'reading' value"); return; }

  unsigned long pulsesAtBaseline;
  noInterrupts();
  pulsesAtBaseline = totalPulses;                          // snapshot under the ISR guard
  interrupts();

  baselineKwh = reading;
  baselinePulses = pulsesAtBaseline;
  saveBaseline(reading, pulsesAtBaseline);
  updateSafeKwh();                                         // publish now, don't wait for loop()

  request->send(200, "text/plain", "OK");
}
```

Register it beside the other posts:

```cpp
server.on("/api/reading", HTTP_POST, handleBaseline);
```

`savePulseCount()` is deliberately **not** called here — `totalPulses` is unchanged, so there is nothing new to persist.

### 8.4 The parsing trap

`param->value().toFloat()` returns `0.0` for `"banana"`, and `0.0` is a *legal* reading on a brand-new meter. Checking `reading == 0` as an invalid signal (the pattern used by `handlePulseConfig`/`handleUtilityRateConfig`) would either reject a real zero or accept garbage. Validate the characters first — a scan allowing digits plus at most one `.` — then convert. Otherwise one typo silently writes a baseline of 0 and the absolute reading collapses to device-relative energy.

### 8.5 NVS atomicity / brown-out

`saveBaseline()` writes two keys, and two `put*` calls are not atomic. A brown-out between them pairs a fresh `baselineKwh` with a stale `baselinePulses`, and the absolute reading jumps by that pulse gap. Cheap fix: a validity flag written last —

```
baselineValid = false → putDouble(baselineKwh) → putULong(baselinePulses) → baselineValid = true
```

and serve an absolute reading only when the flag is true. Alternative: one `putBytes` blob holding `{version, kwh, pulses}` as a single NVS write. Either is fine; this runs roughly once per installation, so wear is a non-issue. What matters is that a torn write is *detectable* rather than presented as truth.

### 8.6 Re-baselining and retries

Firmware policy: last write wins, no overwrite token. A deliberate re-baseline later (comparing against the meter's register) gets its own control, not a protocol.

That pushes two rules onto the client:

1. The onboarding form is only *revealed* when the device reports no baseline. Re-posting the same reading later is not harmless — it re-snapshots `baselinePulses` and silently eats the consumption in between.
2. Do not blind-retry the POST on timeout. If the device saved it but the response was lost, a retry moves the origin forward. Re-read `GET /api/config` instead.

### 8.7 Read paths that must learn about the baseline

**`GET /api/config`** — add `has_baseline`, `baseline_kwh`, `baseline_pulses` (and `baseline_date` only if §8.8 says to persist it). The dashboard needs this on load to decide whether to show the setup form, and the last-known values let the form populate instead of starting blank.

**`GET /api/metrics`** — add `since_baseline_kwh` and `meter_kwh`, plus `has_baseline`. Keep `kwh` meaning exactly what it means today (`totalPulses / pulseConstant`, device-relative): `powerNowValue` is labelled "Current Meter Reading" and should switch to `meter_kwh`, but silently redefining an existing field breaks anything else reading it.

**`pulseSensor.cpp`** — derive all snapshots in one critical section in `updateSafeKwh()`:

```cpp
void updateSafeKwh() {
  noInterrupts();
  unsigned long pulses = totalPulses;
  unsigned long base   = baselinePulses;
  interrupts();

  double delta = (pulses >= base)
      ? static_cast<double>(pulses - base) / pulseConstant
      : 0.0;                    // NVS wiped / reflashed: never report a 4-billion-pulse jump
  safeKwhRead          = static_cast<double>(pulses) / pulseConstant;
  safeSinceBaselineKwh = delta;
  safeMeterKwhRead     = baselineKwh + delta;
}
```

With no valid baseline, `meter_kwh` must render as `--`, not as `baselineKwh + delta` with `baselineKwh = 0` — the latter looks like a real reading of the wrong magnitude. That is what `has_baseline` is for.

The existing `extern unsigned long baselinePulses = 0;` in `PulseSensor.cpp` is a definition-with-initializer; make both globals plain definitions in one translation unit (`pulseSensor.cpp`) and keep only `extern` declarations in the header, so storage and server link to the same objects cleanly.

### 8.8 The date field

There is no clock in the firmware and no NTP call anywhere in the project. So `date` can only be metadata: an NVS string displayed back, never used in arithmetic. Two defensible choices — accept-and-ignore (fewest moving parts, cannot store a wrong date) or store-for-display (a nice "baseline taken 2026-11-04" line, but a device without an RTC will happily show a date the browser got wrong). My vote: accept it, validate it, don't store it until something displays it.

### 8.9 Dashboard changes

1. Send `application/x-www-form-urlencoded` with `reading=<value>` (and `date=` if kept). Drop `unit`: `#baselineUnit` is an empty `<label>` whose `for=` points at nothing, so `payload.unit` is already `undefined` and the restore path writes an expando property onto a label. The meter is kWh; the guide already says so.
2. Await the response and show `Baseline saved to this device` only on 2xx. Today a 500 ms `setTimeout` reports success unconditionally — whether the device answered, 404'd, or was unplugged.
3. Decide first-run from `config.has_baseline`, not `localStorage['Krestel-onboarded']`. Local storage is per-browser; whether *this meter* has a baseline is device state. A second browser must not be offered a new baseline.
4. On success: hide the setup section and refresh `meter_kwh`/`since_baseline_kwh` immediately instead of waiting for the 2 s poll.
5. Fix the copy: the guide note still reads "posted to the ESP32 once a firmware endpoint for it exists", and the status pill claims "Baseline saved to this device" before the device has confirmed anything.
6. Same file, adjacent function: `console.Error(err)` in the pulse-config save path throws a `TypeError` inside its own `catch`, turning a handled network error into an unhandled one.

### 8.10 Known accuracy limit (accept and document)

The reading is taken by eye and typed afterwards, so `totalPulses` advances during data entry and those pulses get counted as "after baseline" twice. At 2.4 kW that is 0.00067 kWh/s ≈ 0.04 kWh per minute of typing — below one 1600 imp/kWh step (0.000625 kWh). An RTC or a "reading taken at …" timestamp buys nothing measurable. Put it in the README's limitations instead of engineering around it.

### 8.11 Explicitly out of scope

No timestamped history, no daily/weekly buckets. `Used This Month`, `dailyAvg`, the projection card, and the chart still have no data source; a baseline does not create one. They stay placeholders until after the physical install and soak test.

### 8.12 Open questions for you

- Persist `date`, or accept-and-ignore? (§8.8 — my vote: ignore for now.)
- Accept `POST /api/reading` when a valid baseline already exists (last write wins, simpler) or reject it so only an explicit re-baseline can overwrite? (§8.6)
- Does "Current Meter Reading" stay the absolute register (`meter_kwh`), with `Used This Month` becoming since-baseline consumption once history exists? If so, `since_baseline_kwh` is the number that grows into that card.

### 8.13 Smallest correct first slice

If this gets built in one sitting: `handleBaseline` + its route, the `saveBaseline` validity flag, the `updateSafeKwh` delta with the underflow guard, `meter_kwh`/`has_baseline` in both GET endpoints, and the urlencoded dashboard POST with a real success check. That is enough to enter a reading once and watch the absolute register move off it; everything in §8.8 and §8.11 can wait.