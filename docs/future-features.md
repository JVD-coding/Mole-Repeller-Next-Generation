# Future Features — Next Generation Mole Repeller

Features listed here require further development beyond the current Arduino Pro Mini
firmware. Each section describes what the feature does, why it is valuable, and the
hardware/software changes needed to implement it.

---

## 1. Mole Detection

### What it does
Detects actual mole activity in the soil near the device — tunnel digging, movement
vibrations, soil displacement — and uses that signal to trigger a repeller response
immediately rather than on a fixed schedule. Switches the device from passive (timer-based)
to reactive (event-driven).

### Why it matters
The current firmware fires patterns on a randomised timer regardless of whether a mole
is present. Detection would allow the device to respond the moment activity is sensed,
maximising the startle effect and conserving battery during long quiet periods.

### How to implement

**Sensor options:**

| Sensor | What it detects | Interface |
|---|---|---|
| Piezoelectric vibration sensor (e.g. SW-420) | Ground vibration / shocks | Digital GPIO |
| Geophone (e.g. SM-24) | Low-frequency seismic events 10–240 Hz | Analog ADC |
| Capacitive soil displacement | Tunnel collapse / soil movement | Analog ADC |

**Recommended approach — geophone on A1:**
- Connect SM-24 geophone output to Arduino A1 via a simple op-amp pre-amplifier (LM358,
  gain ~40×) to bring the mV-level signal into the 0–5V ADC range.
- Sample A1 during each wake window and compute a rolling RMS of the readings.
- If RMS exceeds a calibrated threshold, trigger `runCycle()` immediately.
- Add a lockout timer (e.g. 10 s) after each triggered cycle to prevent re-triggering
  on the repeller's own output vibration.

**Firmware changes needed:**
```cpp
// In loop():
uint16_t raw = analogRead(A1);
rms = updateRollingRMS(raw);           // sliding window RMS
if (rms > DETECTION_THRESHOLD) {
    runCycle();
    delay(LOCKOUT_MS);                 // ignore own vibration
} else {
    deepSleepMs(rng_range(...));
}
```

**Hardware note:** The WDT deep sleep must be replaced with `SLEEP_MODE_IDLE` or
`SLEEP_MODE_ADC` so the ADC interrupt can wake the CPU for detection. This raises average
current from ~5µA to ~1–3mA — a battery life trade-off to evaluate against detection benefit.

---

## 2. Mobile App Support

### What it does
Allows the user to monitor device status, configure parameters (sleep interval, pattern
types, frequency range, detection threshold), and receive alerts via a smartphone app
without physically accessing the buried device.

### Why it matters
Outdoor buried devices are hard to check. Currently there is no way to know if the device
is working, what its battery level is, or whether it has detected activity. An app
connection makes the system observable and configurable from a phone.

### How to implement

**Communication options:**

| Technology | Range | Power draw | Module |
|---|---|---|---|
| Bluetooth LE (BLE) | ~10–30 m | Very low | HM-10 / HC-08 (UART, AT commands) |
| WiFi | ~50 m (router needed) | High | ESP-01 / ESP8266 |
| LoRa | ~1–5 km | Low | Ra-02 / RFM95W |

**Recommended approach — HM-10 Bluetooth LE:**
- HM-10 communicates over UART at 3.3V logic. Use a voltage divider on the RX line for
  the 5V Pro Mini.
- Connect: HM-10 TX → Arduino D2 (SoftwareSerial RX), HM-10 RX → Arduino D3 via divider.
- Build a simple app in MIT App Inventor, Flutter, or Swift/Kotlin that connects via BLE
  and sends/receives ASCII commands.

**Example command protocol:**
```
App → Device:    "STATUS\n"           → "BAT:3.8V ACT:42 DET:7\n"
App → Device:    "SET SLEEP 60\n"     → adjust SLEEP_MIN_MS at runtime
App → Device:    "SET FREQ 100 600\n" → adjust FREQ_MIN / FREQ_MAX
App → Device:    "TRIGGER\n"          → force immediate runCycle()
```

**Firmware changes needed:**
- Add `SoftwareSerial ble(2, 3)` and a lightweight command parser in `loop()`.
- Track activity count and detection events in global variables.
- Use INT0 on D2 to wake from deep sleep when the app sends a command.

**Hardware note:** The Pro Mini has 2KB RAM — a BLE parser fits comfortably alone, but
combining BLE + detection + logging may be tight. Consider upgrading to an
**Arduino Nano 33 IoT** (built-in BLE, 32KB RAM, same footprint) for this feature.

---

## 3. Mesh Network Support

### What it does
Links multiple repeller units so they can coordinate firing patterns, share detection
events, and cover a large garden as a unified system. When one unit detects a mole, it
alerts nearby units to fire in a staggered sequence — surrounding the mole with stimulus
from multiple directions.

### Why it matters
A single unit covers a limited radius (1–5 m effective in typical garden soil). A mesh
of 4–8 units spaced across a garden creates a coordinated deterrent zone. Coordination
also prevents units from firing simultaneously (wasted battery) or in a predictable
spatial pattern that a mole could learn to route around.

### How to implement

**Communication options:**

| Technology | Range | Power draw | Module |
|---|---|---|---|
| 433 MHz (HC-12) | 50–200 m | Very low | HC-12 (UART, transparent) |
| LoRa mesh | 100 m–1 km per hop | Low | Ra-02 / RFM95W + RadioHead lib |
| ESP-NOW | ~200 m open air | Medium | ESP8266 / ESP32 |

**Recommended approach — HC-12 433 MHz:**
- HC-12 uses simple UART (same wiring as BLE above) and supports multi-point
  transparent communication with configurable channels and power levels.
- Each unit stores a unique node ID in EEPROM byte 0 (set once at flash time).
- Units broadcast detection events; neighbours receive and fire with a random delay
  to avoid simultaneous triggering.

**Example mesh behaviour:**
```
Unit A detects mole activity
  → broadcasts "DET:A\n" over HC-12
Unit B receives "DET:A\n"
  → waits rng_range(500, 3000) ms    // staggered to avoid simultaneous firing
  → runs runCycle()
  → broadcasts "FIRED:B\n"
Unit C receives "FIRED:B\n"
  → updates sleep timer to avoid firing too soon after B
```

**Firmware changes needed:**
- Read node ID from `EEPROM.read(0)` at startup.
- Add HC-12 UART listener via SoftwareSerial.
- Implement a simple flood-broadcast with a hop counter to prevent message loops.
- Coordinate sleep windows so at least one unit stays awake as a relay at all times.

**Hardware note:** Running SoftwareSerial for BLE + SoftwareSerial for HC-12 + ADC
detection simultaneously exceeds the Pro Mini's capabilities. At this feature level
migrate to an **Arduino Nano Every** (2× hardware UART, 6KB RAM) or **ESP32**
(built-in BLE + WiFi, dual-core, 520KB RAM, deep sleep at ~10µA).

---

## 4. MCU Platform — Why ESP32 is the Current Choice

The firmware already targets the **ESP32 DevKit C** as of this version. Here is why it
was chosen over the original Arduino Pro Mini:

| Requirement | Pro Mini | ESP32 |
|---|---|---|
| Mole detection (geophone ADC) | Marginal — sleep mode conflict | Yes — ADC + deep sleep coexist |
| BLE mobile app | No — needs external HM-10 module | Yes — built-in BLE 4.2 |
| Mesh network (ESP-NOW) | No — needs external radio | Yes — built-in WiFi/ESP-NOW |
| Hardware UART count | 1 | 3 |
| RAM | 2KB | 520KB |
| Deep sleep current | ~5µA | ~10µA |
| 3.3V GPIO (relay note) | No — 5V GPIO | Yes — use 3.3V-compatible relay module |
| Arduino IDE support | Yes | Yes (esp32 board package) |
| Cost | ~€3 | ~€5–8 |

All pattern, PRNG, and sleep logic from the original Pro Mini sketch ports directly to
ESP32 with only the sleep API changed (`esp_deep_sleep_start()` replaces WDT ticks).

**3.3V relay note:** ESP32 GPIO is 3.3V. Use a relay module rated for 3.3V input (e.g.
KEYES with HEF4001 optocoupler), or add a 2N2222 transistor level shifter between
GPIO25 and the relay coil — see `hardware/schematic.txt` Circuit 1 for details.
