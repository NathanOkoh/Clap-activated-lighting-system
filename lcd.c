#include "stm32f10x.h"
#include "lcd.h"
#include "I2C.h"
#include "delay.h"

// --- Internal us Delay ----------------------------------------------------
// kept private — delay.h only provides ms, LCD init needs us precision
static void LCD_delay_us(uint32_t us)
{
    for (uint32_t i = 0; i < us * 36; i++)
    {
        __NOP();
    }
}

// --- Write raw byte to PCF8574 over I2C -----------------------------------
static void LCD_writeExpander(uint8_t data)
{
    i2c_waitForReady();
    i2c_sendStart();
    i2c_sendAddrForWrite(LCD_ADDR);
    i2c_sendData(data);
    i2c_sendStop();
}

// --- Pulse the Enable pin to latch a nibble -------------------------------
static void LCD_pulseEnable(uint8_t data)
{
    LCD_writeExpander(data |  LCD_ENABLE);   // EN high
    LCD_delay_us(1);
    LCD_writeExpander(data & ~LCD_ENABLE);   // EN low
    LCD_delay_us(50);
}

// --- Send 4 bits (one nibble) ---------------------------------------------
static void LCD_sendNibble(uint8_t nibble, uint8_t rs)
{
    uint8_t data = (nibble & 0xF0) | LCD_BACKLIGHT | rs;
    LCD_pulseEnable(data);
}

// --- Send full byte as two nibbles (4-bit mode) ---------------------------
static void LCD_sendByte(uint8_t byte, uint8_t rs)
{
    LCD_sendNibble( byte & 0xF0,       rs);   // high nibble first
    LCD_sendNibble((byte << 4) & 0xF0, rs);   // low nibble second
}

// --- Send a command (RS = 0) ----------------------------------------------
static void LCD_sendCommand(uint8_t cmd)
{
    LCD_sendByte(cmd, 0);
    delay_ms(2);
}

// --- Init -----------------------------------------------------------------
void LCD_init(void)
{
    delay_ms(50);   // Wait for LCD power-on

    // HD44780 4-bit init sequence (3x 0x30 then switch to 4-bit)
    LCD_sendNibble(0x30, 0);  delay_ms(5);
    LCD_sendNibble(0x30, 0);  delay_ms(1);
    LCD_sendNibble(0x30, 0);  delay_ms(1);
    LCD_sendNibble(0x20, 0);  delay_ms(1);   // switch to 4-bit mode

    LCD_sendCommand(0x28);   // Function set: 4-bit, 2 lines, 5x8 font
    LCD_sendCommand(0x0C);   // Display ON, cursor OFF, blink OFF
    LCD_sendCommand(0x06);   // Entry mode: increment cursor, no shift
    LCD_sendCommand(0x01);   // Clear display
    delay_ms(2);
}

// --- Clear ----------------------------------------------------------------
void LCD_clear(void)
{
    LCD_sendCommand(0x01);
    delay_ms(2);
}

// --- Set Cursor Position --------------------------------------------------
void LCD_setCursor(uint8_t col, uint8_t row)
{
    uint8_t addr;

    if ((row & 0x01) != 0)
        addr = LCD_ROW1 + col;   // row 1
    else
        addr = LCD_ROW0 + col;   // row 0

    LCD_sendCommand(addr);
}

// --- Send Single Character ------------------------------------------------
void LCD_sendChar(char c)
{
    LCD_sendByte((uint8_t)c, LCD_RS);   // RS = 1 for data
}

// --- Send String ----------------------------------------------------------
void LCD_sendString(char *str)
{
    while (*str)
    {
        LCD_sendChar(*str++);
    }
}

// --- Send Integer ---------------------------------------------------------
void LCD_sendInt(int value)
{
    char buf[6];
    int i = 0;

    if (value == 0)
    {
        LCD_sendChar('0');
        return;
    }

    if (value < 0)
    {
        LCD_sendChar('-');
        value = -value;
    }

    while (value > 0)
    {
        buf[i++] = '0' + (value % 10);
        value /= 10;
    }

    while (i > 0)
    {
        LCD_sendChar(buf[--i]);
    }
}