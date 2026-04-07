#ifndef I2C_H
#define I2C_H

#include "stm32f10x.h"

void i2c_init();
void i2c_periph_set_ack();
void i2c_periph_set_ownaddr();
void i2c_enable();
void i2c_waitForReady();
void i2c_sendStart();
void i2c_sendStop();
uint8_t i2c_sendAddr(uint8_t addr);
uint8_t i2c_sendAddrForRead(uint8_t addr);
uint8_t i2c_sendAddrForWrite(uint8_t addr);
uint8_t i2c_sendData(uint8_t data);
uint8_t i2c_readData(uint8_t ack);
int8_t TC74_readTemp(void);

#endif