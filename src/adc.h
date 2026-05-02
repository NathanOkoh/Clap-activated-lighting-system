#ifndef ADC_H
#define ADC_H
#include "stm32f10x.h"

void     adc_init(void);
uint16_t adc_read(void);

#endif
