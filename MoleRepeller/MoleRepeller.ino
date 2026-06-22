/*
 * Mole Repeller - Next Generation
 * Target: ESP32 DevKit C
 *
 * Hardware:
 *   GPIO25 → Relay module IN → 12V vibration motor (via NO/COM terminals)
 *            Relay module VCC → 5V, GND → GND
 *            1N4001 flyback diode across motor terminals
 *            Use a relay module rated for 3.3V input signal, or add a
 *            2N2222 level-shift transistor between GPIO25 and the relay coil
 *   GPIO26 → LM386 amplifier input → 8Ω speaker (1W min, 57mm+ cone)
 *   GPIO2  → Status LED (onboard LED on most ESP32 DevKit boards)
 *   GPIO34 → Floating ADC pin (input only) — entropy source for RNG seed
 *   12V supply → relay COM terminal → motor
 *
 * Strategy:
 *   Five pattern types (burst, sweep, staccato, rumble, combo) are randomly
 *   dispatched in groups of 1–4 per wake cycle. The relay switches the 12V
 *   motor on/off; the LM386 speaker provides simultaneous low-frequency audio.
 *   Between cycles the ESP32 enters deep sleep (~10µA) for 30–180 s.
 *   The PRNG is re-seeded from ADC noise on every wake so no two cycles repeat.
 */

#include <Arduino.h>
#include "esp_sleep.h"

// ── Pin assignments ──────────────────────────────────────────────────────────
static const uint8_t PIN_RELAY   = 25;  // Relay IN — switches 12V motor
static const uint8_t PIN_SPEAKER = 26;  // Speaker via LM386 amplifier
static const uint8_t PIN_LED     = 2;   // Onboard LED (active HIGH on DevKit C)
static const uint8_t PIN_ENTROPY = 34;  // Floating ADC (input-only pin, no pullup)

// ── Frequency range perceived by moles (Hz) ─────────────────────────────────
static const uint16_t FREQ_MIN =  50;
static const uint16_t FREQ_MAX = 800;

// ── Cycle sleep window (ms) ──────────────────────────────────────────────────
static const uint32_t SLEEP_MIN_MS =  30000UL;  // 30 s
static const uint32_t SLEEP_MAX_MS = 180000UL;  // 3 min

// ─────────────────────────────────────────────────────────────────────────────
// XOR-shift 32-bit PRNG — fast, no division, full 2^32 period
// ─────────────────────────────────────────────────────────────────────────────
static uint32_t rng_state;

static void rng_seed(uint32_t seed) {
    rng_state = seed ? seed : 0xDEADBEEFUL;
}

// XOR-shift: 3 shifts produce a full-period sequence over all 2^32 non-zero values.
static uint32_t rng_next() {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

// Uniform random value in [lo, hi]
static uint32_t rng_range(uint32_t lo, uint32_t hi) {
    return lo + (rng_next() % (hi - lo + 1));
}

// ─────────────────────────────────────────────────────────────────────────────
// Relay helpers — 12V motor is on/off only (relay cannot PWM)
// ─────────────────────────────────────────────────────────────────────────────
static inline void motorOn()  { digitalWrite(PIN_RELAY, HIGH); }
static inline void motorOff() { digitalWrite(PIN_RELAY, LOW);  }

// ─────────────────────────────────────────────────────────────────────────────
// Pattern 1 — BURST: sharp on/off pulses at random frequencies
// ─────────────────────────────────────────────────────────────────────────────
static void patternBurst() {
    uint8_t count = rng_range(2, 8);       // R: 2–8 pulses — unpredictable burst length
    for (uint8_t i = 0; i < count; i++) {
        uint16_t freq = rng_range(FREQ_MIN, FREQ_MAX); // R: 50–800 Hz — different pitch every pulse
        uint16_t on   = rng_range(80, 400);            // R: pulse duration varies so rhythm is never fixed
        uint16_t off  = rng_range(50, 300);            // R: gap between pulses also varies
        motorOn();
        tone(PIN_SPEAKER, freq, on);
        delay(on);
        motorOff();
        noTone(PIN_SPEAKER);
        delay(off);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Pattern 2 — SWEEP: continuous glide up or down while motor runs
// ─────────────────────────────────────────────────────────────────────────────
static void patternSweep() {
    bool     asc    = rng_next() & 1;                    // R: direction up or down — coin flip each time
    uint16_t start  = asc ? rng_range(FREQ_MIN, 200)    // R: ascending sweep starts low (50–200 Hz)
                          : rng_range(400, FREQ_MAX);    //    descending sweep starts high (400–800 Hz)
    uint16_t end    = asc ? rng_range(400, FREQ_MAX)    // R: ascending ends high
                          : rng_range(FREQ_MIN, 200);    //    descending ends low
    uint8_t  steps  = rng_range(20, 60);                 // R: 20–60 steps — controls sweep resolution
    uint16_t stepMs = rng_range(20, 60);                 // R: 20–60 ms per step — controls sweep speed
    int16_t  delta  = ((int16_t)end - (int16_t)start) / (int16_t)steps;

    motorOn();
    for (uint8_t i = 0; i < steps; i++) {
        tone(PIN_SPEAKER, (uint16_t)((int16_t)start + (int16_t)i * delta));
        delay(stepMs);
    }
    noTone(PIN_SPEAKER);
    motorOff();
}

// ─────────────────────────────────────────────────────────────────────────────
// Pattern 3 — STACCATO: rapid short pulses with slight frequency jitter
// ─────────────────────────────────────────────────────────────────────────────
static void patternStaccato() {
    uint8_t  pulses   = rng_range(10, 40);    // R: 10–40 pulses — burst density unpredictable
    uint16_t baseFreq = rng_range(100, 600);  // R: base pitch shifts every staccato sequence
    for (uint8_t i = 0; i < pulses; i++) {
        uint16_t f = baseFreq + (uint16_t)rng_range(0, 80) - 40; // R: ±40 Hz jitter per pulse — never monotone
        motorOn();
        tone(PIN_SPEAKER, f);
        delay(rng_range(30, 120));             // R: on-time varies — irregular clicking rhythm
        motorOff();
        noTone(PIN_SPEAKER);
        delay(rng_range(20, 100));             // R: off-time varies independently of on-time
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Pattern 4 — RUMBLE: sustained low tone with irregular motor beats
// ─────────────────────────────────────────────────────────────────────────────
static void patternRumble() {
    uint16_t freq     = rng_range(FREQ_MIN, 150); // R: very low tone (50–150 Hz) — mimics underground activity
    uint32_t duration = rng_range(1000, 4000);    // R: rumble lasts 1–4 s — total length unpredictable
    uint32_t elapsed  = 0;
    tone(PIN_SPEAKER, freq);                       // sustained tone throughout
    while (elapsed < duration) {
        uint16_t on  = rng_range(100, 500);        // R: motor beat length varies within the rumble
        uint16_t off = rng_range(50, 250);         // R: motor rest length varies — irregular heartbeat feel
        motorOn();
        delay(on);
        motorOff();
        delay(off);
        elapsed += (uint32_t)on + off;
    }
    noTone(PIN_SPEAKER);
    motorOff();
}

// ─────────────────────────────────────────────────────────────────────────────
// Pattern 5 — COMBO: chains burst → sweep → staccato with gaps
// ─────────────────────────────────────────────────────────────────────────────
static void patternCombo() {
    patternBurst();
    delay(rng_range(100, 400));   // R: gap between sub-patterns varies
    patternSweep();
    delay(rng_range(100, 300));   // R: second gap independent of first
    patternStaccato();
    // Each sub-pattern carries its own internal randomisation (see above),
    // so combo produces the highest variety of any single pattern type.
}

// ─────────────────────────────────────────────────────────────────────────────
// Pattern dispatcher — pick 1–4 random patterns per wake cycle
// ─────────────────────────────────────────────────────────────────────────────
typedef void (*PatternFn)();
static const PatternFn PATTERNS[] = {
    patternBurst,
    patternSweep,
    patternStaccato,
    patternRumble,
    patternCombo,
};
static const uint8_t NUM_PATTERNS = sizeof(PATTERNS) / sizeof(PATTERNS[0]);

static void runCycle() {
    uint8_t count = rng_range(1, 4);                // R: 1–4 patterns fired per wake — cycle length varies
    for (uint8_t i = 0; i < count; i++) {
        PATTERNS[rng_next() % NUM_PATTERNS]();       // R: pattern type chosen independently each slot
        delay(rng_range(200, 800));                  // R: inter-pattern pause varies — no fixed cadence
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// ESP32 deep sleep — restarts from setup() on each wake
// ─────────────────────────────────────────────────────────────────────────────
static void deepSleepMs(uint32_t ms) {
    digitalWrite(PIN_RELAY, LOW);                              // ensure motor is off before sleep
    esp_sleep_enable_timer_wakeup((uint64_t)ms * 1000ULL);    // timer wakeup, µs units
    esp_deep_sleep_start();                                    // does not return
}

// ─────────────────────────────────────────────────────────────────────────────
// Arduino entry points
// ─────────────────────────────────────────────────────────────────────────────
void setup() {
    pinMode(PIN_RELAY,   OUTPUT);
    pinMode(PIN_SPEAKER, OUTPUT);
    pinMode(PIN_LED,     OUTPUT);
    digitalWrite(PIN_RELAY, LOW);   // relay off at boot — motor must not fire until pattern starts

    // Seed RNG from ADC noise: floating pin produces random LSBs each read.
    // Re-seeded on every wake so each sleep cycle starts a unique sequence.
    uint32_t seed = 0;
    for (uint8_t i = 0; i < 32; i++) {
        seed = (seed << 1) | (analogRead(PIN_ENTROPY) & 1);
        delay(1);
    }
    rng_seed(seed);

    // Quick startup blink to confirm the device is alive after each wake
    for (uint8_t i = 0; i < 3; i++) {
        digitalWrite(PIN_LED, HIGH); delay(100);
        digitalWrite(PIN_LED, LOW);  delay(100);
    }
}

void loop() {
    digitalWrite(PIN_LED, HIGH);
    runCycle();
    digitalWrite(PIN_LED, LOW);

    deepSleepMs(rng_range(SLEEP_MIN_MS, SLEEP_MAX_MS)); // R: 30–180 s silence — moles can't learn the interval
    // deepSleepMs does not return — ESP32 wakes and restarts from setup()
}
