/*  adc.c  -  ADC1 single-conversion on PA0 (Channel 0)
 *            Microphone op-amp output feeds this pin.
 *
 *  Hardware:
 *    PA0 = Analog Input (no pull-up, no pull-down)
 *    ADC1 Channel 0
 *    12-bit result (0 to 4095)
 *    ADC clock = APB2 / 8 = 72 MHz / 8 = 9 MHz  (limit is 14 MHz)
 *    Sample time = 55.5 cycles on CH0  (~6.2 us at 9 MHz - good for mic)
 *
 *  At rest the op-amp output sits at ~1.65V (bias midpoint).
 *  1.65V / 3.3V * 4095 = ~2047 ADC counts.
 *  A clap produces a spike well above that.
 *
 *  All bit manipulation uses STM32 CMSIS register macros.
 */
#include "stm32f10x.h"
#include "adc.h"

void adc_init(void)
{
    /* -- 1. Enable clocks ---------------------------------------------- */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;    /* GPIOA clock                */
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;    /* ADC1 clock                 */

    /* -- 2. PA0 = analog input ----------------------------------------- */
    /* CRL bits [3:0] for PA0 = 0000 = analog input. That is reset state.*/
    GPIOA->CRL &= ~(GPIO_CRL_MODE0 | GPIO_CRL_CNF0); /* 0000 = analog   */

    /* -- 3. ADC prescaler = /8  (72 MHz / 8 = 9 MHz) ----------------- */
    RCC->CFGR &= ~RCC_CFGR_ADCPRE;          /* clear prescaler bits      */
    RCC->CFGR |=  RCC_CFGR_ADCPRE_DIV8;     /* set /8                    */

    /* -- 4. ADC basic config ------------------------------------------- */
    ADC1->CR1  = 0;                          /* independent, no scan      */
    ADC1->CR2  = 0;                          /* single conversion, SW trig*/

    /* -- 5. Sample time: CH0 = 55.5 cycles (bits [2:0] of SMPR2 = 101) */
    ADC1->SMPR2 &= ~ADC_SMPR2_SMP0;         /* clear CH0 sample bits     */
    ADC1->SMPR2 |=  ADC_SMPR2_SMP0_2;       /* set bit2 only => 101 = 55.5 cycles */

    /* -- 6. Regular sequence: 1 conversion, channel 0 ----------------- */
    ADC1->SQR1  = 0;                         /* L[3:0]=0000: 1 conversion */
    ADC1->SQR3  = 0;                         /* SQ1 = channel 0           */

    /* -- 7. Power on ADC (first write just powers it up) -------------- */
    ADC1->CR2 |= ADC_CR2_ADON;

    /* -- 8. Calibration (required on STM32F1 after power-on) ---------  */
    ADC1->CR2 |= ADC_CR2_RSTCAL;            /* reset calibration         */
    while (ADC1->CR2 & ADC_CR2_RSTCAL) { __NOP(); }

    ADC1->CR2 |= ADC_CR2_CAL;              /* start calibration          */
    while (ADC1->CR2 & ADC_CR2_CAL)    { __NOP(); }
}

uint16_t adc_read(void)
{
    /* Writing ADON while already ON starts a conversion on STM32F1 */
    ADC1->CR2 |= ADC_CR2_ADON;

    /* Wait for End Of Conversion flag */
    while (!(ADC1->SR & ADC_SR_EOC)) { __NOP(); }

    /* Read 12-bit result (reading DR also clears EOC) */
    return (uint16_t)(ADC1->DR & 0x0FFF);
}
