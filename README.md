# Clap-activated-lighting-system

A embedded system built on the STM32F103RB microcontroller that detects a double clap audio input and toggles an LED based on a valid input. Written entirely in register-level C against the STM32 CMSIS headers.

Press a button to arm the system. Clap twice within one second. The LED toggles, the buzzer confirms, and the LCD updates. Press again to disarm.

---

## How it works

Sound is captured by an electret condenser microphone (CMA-4544PF-W). Because the raw mic signal is only a few millivolts, a TL084CN op-amp preamp amplifies it roughly 100× before it reaches the STM32 ADC. The ADC samples at ~500Hz inside the main loop. A software state machine watches for two ADC threshold crossings (rising edges) within a configurable time window — if both arrive, it's a valid double-clap.

```
Mic → 4.7kΩ bias → 10µF AC cap → op-amp (gain ≈100×) → 100Ω → PA0 ADC
                                                                     ↓
                                              clap state machine in main loop
                                                                     ↓
                                              PA5 LED  +  PA6 buzzer  +  I2C LCD
```

---

## Technical stack

| Layer | Detail |
|---|---|
| MCU | STM32F103RB — ARM Cortex-M3, 72MHz |
| Language | C  |
| IDE | Keil uVision |
| Analog front-end | TL084CN non-inverting amplifier, gain ~100× |
| ADC | 12-bit, single conversion, software triggered, ~500 samples/sec |
| Timing | SysTick at 1ms for clap window timing; TIM2 at 1µs for delays |
| Display | HD44780 16×2 LCD via PCF8574 I2C backpack at 0x27 |
| I2C | I2C1 at 100kHz, PB6/PB7, open-drain with 4.7kΩ pull-ups |
| GPIO | Push-pull outputs for LED and buzzer transistor; pull-up input for button |
| Output driver | 2N3904 NPN transistor switch for buzzer load current |

---

## Clap detection algorithm

The detector runs on every main loop iteration with no blocking delays, giving it maximum ADC throughput.

```c
// Rising-edge detection: only the onset of each clap is counted
if (above_threshold && !was_above_threshold_last_sample) {
    if (first_clap_not_yet_seen) {
        record_timestamp();         // start the window
    } else {
        gap = now - first_clap_timestamp;
        if (gap >= MIN_GAP && gap <= WINDOW) {
            return VALID_DOUBLE_CLAP;   // triggers LED + buzzer + LCD
        }
    }
}
```

The minimum gap (80ms) rejects echoes and surface reflections. The window (1000ms) requires intentional timing. Both are compile-time constants that can be tuned without touching the logic.

---

## Hardware

| Component | Part | Role |
|---|---|---|
| Microcontroller | STM32F103RB (NUCLEO) | Processing |
| Microphone | CMA-4544PF-W | Sound input |
| Op-amp | TL084CN | Signal amplification |
| Transistor | 2N3904 NPN | Buzzer current driver |
| Buzzer | PK-27N25WQ | Audible confirmation |
| Display | CN0295D + PCF8574 | I2C LCD state display |
| LED | White 5mm | Visual output |

Full schematic, wiring table, and component datasheets are in `/docs`.

---

## Project structure

```
stm32-clap-activated-lighting/
├── src/
│   ├── main.c          # Clap detection state machine, main loop
│   ├── adc.c / .h      # ADC1 single conversion on PA0
│   ├── gpio.c / .h     # LED, buzzer, button GPIO config
│   ├── timer.c / .h    # TIM2 free-running at 1µs
│   ├── delay.c / .h    # delay_ms() / delay_us()
│   ├── I2C.c / .h      # I2C1 master driver
│   └── lcd.c / .h      # HD44780 over PCF8574 backpack
├── docs/
│   ├── schematic.pdf           # Full system schematic
│   ├── wiring_table.md         # Pin-by-pin connection reference
│   └── engineering_handover.docx  # Full project report
└── README.md
```

---

## Build and flash

1. Open Keil uVision 5 and create a new project targeting `STM32F103RB`
2. Add all `.c` files from `/src` to the project
3. Build — zero errors, zero warnings expected
4. Flash via the onboard ST-Link over USB
5. Press RESET on the NUCLEO to start

No external libraries or package managers. All peripheral configuration is done directly against CMSIS registers.

---

## Configuration

All tunable parameters are `#define` constants at the top of `main.c`:

```c
#define CLAP_THRESHOLD    2600   // ADC counts — raise to reduce sensitivity
#define CLAP_MIN_GAP_MS     80   // Minimum ms between claps (echo rejection)
#define CLAP_WINDOW_MS    1000   // Maximum ms between claps (timing window)
#define BUZZER_MS         1500   // Confirmation beep duration
```

If the resting ADC value changes due to a different mic or gain setting, run the ADC debug mode (flash `test_stage4_adc.c`) to read the baseline, then set `CLAP_THRESHOLD` to roughly 200 counts above it.

---

## What I learned

- Analog signal conditioning for embedded ADC inputs — biasing, AC coupling, gain selection
- Why op-amps need a minimum supply voltage and how to work around 3.3V system constraints
- I2C peripheral configuration at the register level including open-drain requirements and pull-up sizing
- Debugging hardware with a multimeter alongside software with a USART terminal
- The value of staged integration testing — each component was verified independently before being combined

---
