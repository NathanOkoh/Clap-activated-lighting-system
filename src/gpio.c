/*  gpio.c  -  GPIO init for LED, Buzzer, and Button
 *
 *  PA0  Analog input   - microphone ADC (configured in adc.c, left alone here)
 *  PA1  Input pull-up  - Pushbutton, active LOW
 *  PA5  Output 2MHz PP - White LED (via 220R to GND)
 *  PA6  Output 2MHz PP - 2N3904 base (via 1kR to transistor -> buzzer)
 *
 *  All bit manipulation uses STM32 CMSIS register macros only.
 */
#include "stm32f10x.h"
#include "gpio.h"

void gpio_init(void)
{
    /* -- Enable GPIOA clock ------------------------------------------- */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;

    /* -- PA1: Input with pull-up (Button) ----------------------------- */
    /* CRL bits [7:4] for PA1:
     *   MODE1[1:0] = 00  (input)
     *   CNF1 [1:0] = 10  (input with pull-up / pull-down)
     *   Set ODR bit 1 HIGH to select pull-up                           */
    GPIOA->CRL &= ~(GPIO_CRL_MODE1 | GPIO_CRL_CNF1);  /* clear PA1     */
    GPIOA->CRL |=   GPIO_CRL_CNF1_1;                  /* CNF=10 MODE=00 */
    GPIOA->ODR |=   GPIO_ODR_ODR1;                    /* pull-up ON     */

    /* -- PA5: Output push-pull 2MHz (LED) ---------------------------- */
    /* CRL bits [23:20] for PA5:
     *   MODE5[1:0] = 10  (output, max 2MHz)
     *   CNF5 [1:0] = 00  (general purpose push-pull)                  */
    GPIOA->CRL &= ~(GPIO_CRL_MODE5 | GPIO_CRL_CNF5);  /* clear PA5     */
    GPIOA->CRL |=   GPIO_CRL_MODE5_1;                 /* MODE=10 CNF=00 */
    GPIOA->BRR  =   GPIO_BRR_BR5;                     /* LED starts OFF */

    /* -- PA6: Output push-pull 2MHz (Buzzer transistor base) ---------- */
    /* CRL bits [27:24] for PA6:
     *   MODE6[1:0] = 10  (output, max 2MHz)
     *   CNF6 [1:0] = 00  (general purpose push-pull)                  */
    GPIOA->CRL &= ~(GPIO_CRL_MODE6 | GPIO_CRL_CNF6);  /* clear PA6     */
    GPIOA->CRL |=   GPIO_CRL_MODE6_1;                 /* MODE=10 CNF=00 */
    GPIOA->BRR  =   GPIO_BRR_BR6;                     /* Buzzer starts OFF */
}
