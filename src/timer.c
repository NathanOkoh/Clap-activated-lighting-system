/*  timer.c  -  TIM2 free-running at 1 us per tick
 *
 *  APB1 clock = 36 MHz, but when APB1 prescaler != 1 the timer
 *  input clock is doubled to 72 MHz.
 *  PSC = 72-1  -->  counter ticks at 1 MHz (1 us per tick)
 *  ARR = 0xFFFF  -->  free-running, wraps at 65535 us
 *
 *  delay.c reads TIM2->CNT directly.
 */
#include "stm32f10x.h"
#include "timer.h"

void timer_init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;   /* enable TIM2 clock              */

    TIM2->CR1   = 0;                        /* stop while configuring         */
    TIM2->PSC   = 72 - 1;                   /* 72 MHz / 72 = 1 MHz = 1 us    */
    TIM2->ARR   = 0xFFFF;                   /* max count, free-running        */
    TIM2->CNT   = 0;
    TIM2->EGR  |= TIM_EGR_UG;              /* force update: load PSC & ARR   */
    TIM2->CR1  |= TIM_CR1_CEN;             /* start counter                  */
}
