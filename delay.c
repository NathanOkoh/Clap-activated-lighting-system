#include "stm32f10x.h"
#include "delay.h"

void delay_us(uint16_t delay_time_us)
{
	TIM2->CNT = 0;
	while (TIM2->CNT <= delay_time_us)
	{}
	
}

void delay_ms(uint16_t delay_time_ms)
{
	for (uint16_t i=0; i<delay_time_ms; i++)
	{
		delay_us (100); // delay of 1 ms
	}
}