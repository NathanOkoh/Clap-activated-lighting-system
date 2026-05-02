/* ================================================================
 *  main.c  —  Clap Detection Lighting System
 *  ENEL 351 Design Project 2026  |  STM32F103RB (NUCLEO)
 *
 *  HOW THE SYSTEM WORKS (big picture):
 *  -------------------------------------------------------------
 *  1. Press the button ? enter "Listening" mode (LCD updates).
 *  2. Press again      ? exit  "Listening" mode.
 *  3. While listening, the ADC samples the microphone 500 times
 *     per second through the op-amp preamp on PA0.
 *  4. Two loud sounds (ADC spike > CLAP_THRESHOLD) detected
 *     within CLAP_WINDOW_MS milliseconds = valid double-clap.
 *  5. Valid clap ? toggle LED on PA5, buzz buzzer on PA6 200ms,
 *     update LCD row 1 with "Light: ON" or "Light: OFF".
 *
 *  PIN ASSIGNMENTS:
 *  -------------------------------------------------------------
 *  PA0  Analog in    Mic preamp output ? ADC1 channel 0
 *  PA1  Input PU     Pushbutton (active LOW, internal pull-up)
 *  PA5  Output PP    White LED via 220O to GND
 *  PA6  Output PP    2N3904 Base via 1kO ? buzzer collector
 *  PB6  AF OD        I2C1 SCL ? LCD backpack (4.7k to 3.3V)
 *  PB7  AF OD        I2C1 SDA ? LCD backpack (4.7k to 3.3V)
 *
 *  FILES NEEDED IN KEIL PROJECT:
 *  -------------------------------------------------------------
 *  main.c   gpio.c   adc.c   timer.c   delay.c
 *  I2C.c    lcd.c    (all .h headers too)
 * ================================================================ */

#include "stm32f10x.h"
#include "timer.h"       /* timer_init() — TIM2 at 1us tick              */
#include "delay.h"       /* delay_ms(), delay_us()                        */
#include "gpio.h"        /* gpio_init() — LED, buzzer, button pins        */
#include "adc.h"         /* adc_init(), adc_read() — mic ADC on PA0      */
#include "I2C.h"         /* i2c_init(), i2c_enable() — I2C1 PB6/PB7     */
#include "lcd.h"         /* LCD_init(), LCD_setCursor(), LCD_sendString() */

/* ================================================================
 *  TUNABLE CLAP DETECTION PARAMETERS
 *  -------------------------------------------------------------
 *  After running test_stage4_adc.c, note your resting ADC value.
 *  Set CLAP_THRESHOLD = resting value + ~600.
 *  Example: resting = 2050, threshold = 2650.
 * ================================================================ */
#define CLAP_THRESHOLD   2500   /* ADC counts above this = clap event     */
#define CLAP_MIN_GAP_MS    60   /* min ms between two claps (anti-bounce) */
#define CLAP_WINDOW_MS    1000   /* both claps must arrive within this      */
#define BUZZER_MS         1500   /* how long the confirmation beep lasts    */
#define BTN_DEBOUNCE_MS    50   /* ms to wait before confirming button     */

/* ================================================================
 *  MILLISECOND TICK COUNTER  (driven by SysTick interrupt)
 *
 *  SysTick is a 24-bit countdown timer built into the Cortex-M3.
 *  We configure it to fire every 1ms (72MHz / 72000 = 1000Hz).
 *  Each interrupt increments ms_tick.
 *  get_ms() returns the current tick count for timing clap gaps.
 * ================================================================ */
static volatile uint32_t ms_tick = 0;

/* SysTick_Handler — called automatically every 1ms by hardware */
void SysTick_Handler(void)
{
    ms_tick++;   /* increment the global millisecond counter */
}

static uint32_t get_ms(void)
{
    return ms_tick;
}

/* ================================================================
 *  BUTTON READ  —  debounced, edge-detected
 *
 *  PA1 is HIGH at rest (internal pull-up).
 *  Pressing the button pulls PA1 LOW.
 *  We detect the HIGH?LOW transition (falling edge) and wait
 *  BTN_DEBOUNCE_MS to confirm it is not contact bounce.
 *  Returns 1 only once per physical press.
 * ================================================================ */
static uint8_t btn_prev = 1;   /* 1 = not pressed, updated each call */

static uint8_t button_pressed(void)
{
    /* Read PA1: IDR_IDR1 is set when pin is HIGH (not pressed) */
    uint8_t raw = (GPIOA->IDR & GPIO_IDR_IDR1) ? 1 : 0;

    if (raw == 0 && btn_prev == 1)          /* falling edge — button just pressed  */
    {
        delay_ms(BTN_DEBOUNCE_MS);          /* wait for mechanical bounce to settle */

        /* Re-read: if still LOW after debounce, it is a real press */
        if ((GPIOA->IDR & GPIO_IDR_IDR1) == 0)
        {
            btn_prev = 0;
            return 1;   /* confirmed press — return 1 exactly once */
        }
    }

    if (raw == 1)
        btn_prev = 1;   /* button released — arm for next press detection */

    return 0;
}

/* ================================================================
 *  CLAP DETECTOR  —  call in the main loop as fast as possible
 *
 *  State machine with two states:
 *    clap_count = 0 ? waiting for first clap
 *    clap_count = 1 ? got first clap, waiting for second
 *
 *  On every call:
 *    1. Read ADC.
 *    2. Detect RISING edge (signal crosses threshold going up).
 *       Only rising edge counts — this is one clap onset.
 *    3. If first clap:  record timestamp, set clap_count = 1.
 *    4. If second clap: check gap is within window.
 *       If YES ? reset and return 1 (valid double-clap!).
 *       If NO  ? this slow clap becomes the new first clap.
 *    5. If window expires between claps ? reset to idle.
 *
 *  Returns 1 when a valid double-clap is detected, else 0.
 * ================================================================ */
static uint8_t detect_clap(void)
{
    /* Static vars persist between calls (keep state across loop iterations) */
    static uint8_t  above_prev    = 0;   /* was ADC above threshold last call? */
    static uint8_t  clap_count    = 0;   /* 0=waiting first, 1=waiting second  */
    static uint32_t first_clap_ms = 0;   /* timestamp of first clap            */

    uint16_t val  = adc_read();          /* read mic ADC, 0-4095               */
    uint32_t now  = get_ms();            /* current time in ms                 */

    /* Is the signal currently above the clap threshold? */
    uint8_t above = (val > CLAP_THRESHOLD) ? 1 : 0;

    /* Detect RISING EDGE: was below, now above ? new clap onset */
    if (above && !above_prev)
    {
        if (clap_count == 0)
        {
            /* -- First clap detected -- */
            clap_count    = 1;
            first_clap_ms = now;   /* start the timing window */
        }
        else   /* clap_count == 1: second clap has arrived */
        {
            uint32_t gap = now - first_clap_ms;

            if (gap >= CLAP_MIN_GAP_MS && gap <= CLAP_WINDOW_MS)
            {
                /* -- Valid double-clap! --
                 * Gap is long enough to not be an echo (>80ms)
                 * but short enough to be intentional (<600ms)  */
                clap_count    = 0;
                first_clap_ms = 0;
                above_prev    = above;
                return 1;   /* signal the caller */
            }
            else if (gap > CLAP_WINDOW_MS)
            {
                /* Too slow — treat this new clap as a fresh first clap */
                clap_count    = 1;
                first_clap_ms = now;
            }
            /* If gap < CLAP_MIN_GAP_MS: too fast — likely echo, ignore */
        }
    }

    /* Reset window if it has timed out without a second clap arriving */
    if (clap_count == 1 && (now - first_clap_ms) > CLAP_WINDOW_MS)
    {
        clap_count    = 0;
        first_clap_ms = 0;
    }

    above_prev = above;   /* remember this sample for next call */
    return 0;
}

/* ================================================================
 *  MAIN
 * ================================================================ */
int main(void)
{
    /* -- SysTick: generate 1ms interrupt (72MHz core / 72000 = 1kHz) -- */
    SysTick_Config(72000);

    /* -- Peripheral initialisation ----------------------------------- */
    timer_init();   /* TIM2 free-running at 1us, required by delay.c     */
    gpio_init();    /* PA1 pull-up input, PA5/PA6 push-pull outputs       */
    adc_init();     /* ADC1 CH0 on PA0, 12-bit, calibrated                */

    /* I2C and LCD — order matters:
     *   i2c_init()   sets up PB6/PB7 and I2C1 peripheral registers
     *   i2c_enable() sets the PE (peripheral enable) bit in CR1
     *   delay_ms(200) gives the HD44780 LCD controller time to finish
     *                 its internal power-on reset before we talk to it.
     *                 Skipping this delay causes random characters.     */
    i2c_init();
    i2c_enable();
    delay_ms(200);
    LCD_init();
    LCD_clear();

    /* -- State variables ---------------------------------------------- */
    uint8_t listening = 0;   /* 0 = idle, 1 = actively detecting claps   */
    uint8_t led_on    = 0;   /* tracks current LED state for toggling     */

    /* -- Initial LCD display ------------------------------------------ */
    /* LCD_setCursor(col, row) — col first, row second                   */
    LCD_setCursor(0, 0);
    LCD_sendString("Not Listening");
    LCD_setCursor(0, 1);
    LCD_sendString("Press button");
		
		
    /* ================================================================
     *  MAIN LOOP
     *  The loop runs as fast as possible (~500 ADC samples/sec).
     *  button_pressed() and detect_clap() are called every iteration.
     * ================================================================ */
    while (1)
    {
        /* -- Check button: toggle listen mode ------------------------- */
        if (button_pressed())
        {
            listening = !listening;   /* flip the listening flag */
            LCD_clear();

            if (listening)
            {
                /* Entered listening mode */
                LCD_setCursor(0, 0);
                LCD_sendString("Listening...");
                LCD_setCursor(0, 1);
                LCD_sendString("Clap twice!");
            }
            else
            {
                /* Exited listening mode */
                LCD_setCursor(0, 0);
                LCD_sendString("Not Listening");
                LCD_setCursor(0, 1);
                LCD_sendString("Press button");

                /* Wait for the user to physically release the button
                 * before continuing — prevents immediately re-triggering
                 * on the next loop iteration while finger is still down. */
                while ((GPIOA->IDR & GPIO_IDR_IDR1) == 0)
                {
                    __NOP();
                }
            }
        }

        /* -- Clap detection (only runs while listening) --------------- */
        if (listening)
        {
            if (detect_clap())   /* returns 1 on valid double-clap */
            {
                /* -- Toggle LED -- */
                led_on = !led_on;

                if (led_on)
                {
                    /* BSRR bits [15:0]  = set  (write 1 ? pin HIGH) */
                    GPIOA->BSRR = GPIO_BSRR_BS5;   /* PA5 HIGH = LED ON  */
                }
                else
                {
                    /* BSRR bits [31:16] = reset (write 1 ? pin LOW)  */
                    GPIOA->BSRR = GPIO_BSRR_BR5;   /* PA5 LOW  = LED OFF */
                }

                /* -- Buzzer confirmation beep --
                 * PA6 HIGH turns on the 2N3904 transistor.
                 * The transistor then allows current to flow through
                 * the buzzer from 5V ? buzzer+ ? buzzer- ? collector
                 * ? emitter ? GND, making the buzzer sound.           */
                GPIOA->BSRR = GPIO_BSRR_BS6;       /* PA6 HIGH = buzz ON  */
                delay_ms(BUZZER_MS);                /* hold for 200ms      */
                GPIOA->BSRR = GPIO_BSRR_BR6;       /* PA6 LOW  = buzz OFF */

                /* -- Update LCD row 1 -- */
                LCD_setCursor(0, 1);
                if (led_on)
                    LCD_sendString("Light: ON   ");  /* trailing spaces clear old chars */
                else
                    LCD_sendString("Light: OFF  ");
            }
        }
        /* No delay here — we want maximum ADC sampling rate (~500Hz) */
    }
}
