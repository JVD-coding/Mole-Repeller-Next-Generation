# Design Issues with Traditional Mole Repellers

## Why Most Commercial Mole Repellers Fail

---

## 1. Habituation — The Core Problem

Habituation is the single biggest reason commercial mole repellers stop working within days
to weeks of installation. It is a fundamental neurological process: when an animal is
repeatedly exposed to a stimulus that produces no real consequence (no predator, no pain,
no actual threat), the nervous system learns to ignore it. Moles are burrowing mammals with
highly developed sensory systems tuned precisely to detect ground vibrations — they are
**experts** at filtering out background noise from meaningful signals.

### Why habituation is so fast in moles

- Moles live underground where constant low-level vibration is normal (rain, traffic, footsteps).
  Their brains are already pre-adapted to ignore repetitive, non-threatening vibration.
- A fixed-pattern repeller presents the same stimulus in the same way, at the same interval,
  at the same frequency, every time. Within a few exposures the mole's brain flags it as
  "irrelevant background" and stops routing it to the threat-response pathway.
- Field observations and pest control reports consistently show commercial stake repellers
  losing effectiveness within **3–14 days**.

---

## 2. Fixed Frequency — Single Tone, Easy to Tune Out

Most commercial repellers generate a single fixed frequency (commonly 400–600 Hz or a
fixed ultrasonic tone). Problems:

- A mole exposed to the same 500 Hz tone repeatedly habituates to **that specific frequency**.
  Adjacent frequencies may still trigger a response, but the fixed one no longer does.
- The nervous system effectively builds a "notch filter" centred on the repeated frequency.
- Changing frequency unpredictably prevents the brain from forming that filter.

**What this design does instead:** `rng_range(FREQ_MIN, FREQ_MAX)` selects a different
frequency for every pulse — 50 to 800 Hz — so no single frequency is repeated enough
to habituate to.

---

## 3. Fixed Timing Intervals — Predictability Breeds Confidence

Many stake repellers operate on a simple timer: active for N seconds, silent for M seconds,
repeat forever. Examples seen in commercial products:

- 5 s on / 25 s off (30 s cycle)
- 1 s buzz every 60 s

**The problem:** Moles are capable of learning temporal patterns. After a few days they
begin moving during the silent windows. The repeller becomes a clock that tells the mole
exactly when it is safe to tunnel. Fixed timing is arguably worse than no repeller at all
because it creates a reliable safety signal.

**What this design does instead:** The sleep interval between cycles is randomised between
30 s and 180 s. The number of patterns per cycle (1–4) and the gaps between them are also
randomised. There is no detectable period for the mole to synchronise to.

---

## 4. Single Stimulus Channel — Only Vibration OR Only Sound

The majority of commercial repellers produce either:
- **Vibration only** — a buzzing motor in a plastic stake
- **Sound only** — an audible or ultrasonic emitter above ground

Single-channel stimulation is inherently easier to habituate to than multi-channel. When
only one sense is being stimulated, the brain can suppress that channel while leaving the
others intact. Multi-modal stimulation (vibration **and** sound simultaneously) engages
more of the threat-detection system at once, requires suppression across multiple channels,
and is more cognitively demanding to habituate to.

**What this design does instead:** Both the vibration motor and the speaker fire together,
often at different intensities and timings within the same pattern. The Rumble pattern
deliberately runs a sustained speaker tone while applying irregular motor beats — two
independent stimulus streams that are not perfectly correlated.

---

## 5. Wrong Frequency Range — Ultrasonic in Soil Does Not Work

A significant number of products are marketed as "ultrasonic mole repellers." This is
physically problematic:

- Ultrasonic frequencies (>20 kHz) have very short wavelengths. In soil, these wavelengths
  are smaller than the grain-size of typical garden soil, causing rapid scattering and
  attenuation within centimetres.
- Moles primarily perceive the world through **seismic (infrasonic to low-audio)** vibration
  detected by mechanoreceptors in their snouts and limbs, not through an ear adapted for
  airborne ultrasound.
- Research on mole sensory biology points to sensitivity in the **50–1000 Hz** range for
  ground-borne vibration — the same range used by predators (foxes, badgers) digging.

**What this design does instead:** All frequencies are capped at 800 Hz. The Rumble pattern
specifically targets 50–150 Hz to mimic the low-frequency signature of a digging predator.

---

## 6. Poor Ground Coupling — Energy Never Reaches the Mole

Even a well-designed stimulus is useless if it does not reach the mole through the soil.
Typical commercial plastic stakes have poor acoustic impedance matching to soil:

- Plastic has a very different acoustic impedance from moist soil, so a large fraction of
  vibration energy is reflected at the stake–soil interface rather than transmitted.
- Thin-walled hollow plastic stakes act as resonators at specific frequencies, filtering
  out the rest of the spectrum.
- Speakers mounted above ground waste most of their energy into air; only a tiny fraction
  couples into the ground through the base.

**Improvements in this design:**
- The vibration motor should be mechanically fixed to a **metal stake** driven deeply (30–50 cm)
  into soil. Metal has better impedance matching to soil than plastic.
- The speaker should be mounted face-down on a flat plate in contact with the soil surface,
  or coupled to the same metal stake, to maximise ground transmission.

---

## 7. No Adaptation to Soil or Environmental Conditions

Soil conditions dramatically affect vibration propagation:

| Condition | Effect on propagation |
|---|---|
| Dry, loose soil | High attenuation — vibration dies within 1–2 m |
| Moist, compacted soil | Low attenuation — vibration carries 5–10 m |
| Frozen soil | Extremely high transmission — carries tens of metres |
| Clay-heavy soil | Good low-frequency transmission, poor high-frequency |
| Sandy soil | Poor transmission at all frequencies |

Commercial repellers apply the same fixed output regardless of these conditions. A device
perfectly sized for clay soil in summer will be virtually silent underground in dry sandy
soil in August.

**Future improvement:** A soil moisture sensor (e.g. capacitive sensor on the stake) could
be used to scale motor and speaker power up when soil is dry and transmission is poor.

---

## 8. Insufficient Coverage Area

Commercial repeller packaging commonly claims 200–800 m² coverage. In practice:

- Vibration from a 3V coin motor in a plastic stake may only meaningfully penetrate
  1–3 m radius in average garden soil — covering 3–28 m², not 500 m².
- Coverage is also highly anisotropic: a mole 2 m away in a direction with a root mass
  or a layer boundary between it and the stake may receive near-zero stimulus.
- Multiple units are usually needed but rarely used.

---

## 9. No Battery / Fault Indication

Most commercial repellers give no indication when the battery is dead or when the device
has failed. A dead repeller provides zero stimulus while the owner assumes it is working.
Moles re-colonise the area while the owner thinks they are protected.

**What this design does instead:** The LED blinks 3× at power-on, then lights solid during
every active cycle. A silent, dark device tells the user immediately that something is wrong.

---

## 10. Summary: Failure Mode Map

| Failure mode | Traditional repeller | This design |
|---|---|---|
| Habituation to fixed frequency | Yes — single fixed tone | No — 50–800 Hz random every pulse |
| Habituation to fixed timing | Yes — rigid on/off timer | No — 30–180 s random sleep |
| Habituation to fixed pattern | Yes — identical burst every cycle | No — 5 pattern types, 1–4 per cycle, random order |
| Single stimulus channel | Yes — vibration only or sound only | No — motor + speaker simultaneously |
| Wrong frequency range | Often — ultrasonic products | No — 50–800 Hz (seismic range) |
| Poor ground coupling | Yes — plastic stake, surface speaker | Mitigated — metal stake + face-down speaker recommended |
| No fault indication | Yes | No — LED cycle indicator |
| Predictable seed / repeating sequence | N/A (analogue timers) | No — ADC noise seed, unique on every power-on |

---
