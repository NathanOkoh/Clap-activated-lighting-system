#ifndef LCD_H
#define LCD_H

#include "stm32f10x.h"
#include <stdint.h>

// I2C address — default (A0/A1/A2 all HIGH)
#define LCD_ADDR             0x27

// PCF8574 backpack pin mapping
#define LCD_BACKLIGHT        0x08   // P3
#define LCD_ENABLE           0x04   // P2
#define LCD_RW               0x02   // P1 — always LOW (write only)
#define LCD_RS               0x01   // P0 — 0=command, 1=data

// DDRAM row start addresses
#define LCD_ROW0             0x80
#define LCD_ROW1             0xC0

void LCD_init(void);
void LCD_clear(void);
void LCD_setCursor(uint8_t col, uint8_t row);
void LCD_sendChar(char c);
void LCD_sendString(char *str);
void LCD_sendInt(int value);

#endif
