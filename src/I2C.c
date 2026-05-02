#include "stm32f10x.h"
#include "I2C.h"

void i2c_init() 
{
	RCC->APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN; 			// enable clocks for I2C related GPIOs
	RCC->APB1ENR |= RCC_APB1ENR_I2C1EN; // enable clock for I2C1
		
  GPIOB->CRL &= ~((0xF << 24) | (0xF << 28)); // Clear PB7 & PB6
	GPIOB->CRL |= GPIO_CRL_CNF7 | GPIO_CRL_CNF6 | GPIO_CRL_MODE7 | GPIO_CRL_MODE6 ;		// configure PB6 and PB7 as alt. func. open drain 

	I2C1->CR1 |= I2C_CR1_SWRST;   // software reset
  I2C1->CR1 &= ~I2C_CR1_SWRST;  // release reset
	
//  Setup for 36MHZ APB1 Clock @ 100kHz I2C clock speed
	I2C1->CR2 = 72;          // 72Mhz
	I2C1->CCR = 360; // 
	I2C1->TRISE = 73; // 
}


void i2c_periph_set_ack()
{
	I2C1->CR1 |= I2C_CR1_ACK ; // Enable Peripheral ACK
}
void i2c_periph_set_ownaddr()
{
	I2C1->OAR1 = (0x68<<1) ; // Enable Peripheral ACK
}

void i2c_enable()
{
	I2C1->CR1 |=  I2C_CR1_PE ;	// PE = 1 
}


void i2c_waitForReady()	
{
	while((I2C1->SR2 & I2C_SR2_BUSY) != 0)   // check bus busy
		{
			__NOP();
		}
}

void i2c_sendStart()
{
	
	I2C1->CR1 |= (I2C_CR1_START); // start 
	while((I2C1->SR1&(I2C_SR1_SB)) == 0) // wait for SB
	{
		__NOP();
	}
	(void)I2C1->SR2;
}

void i2c_sendStop()
{
	I2C1->CR1 |= (I2C_CR1_STOP); // stop 
	while((I2C1->SR2&(I2C_SR2_MSL)) != 0) // wait for becoming slave
  {
	  __NOP();
	}
}

uint8_t i2c_sendAddr(uint8_t addr)
{
	int stat;
  uint32_t timeout = 50000; // tweak as needed

  I2C1->DR = addr;

  do {
        stat = I2C1->SR1;

        if ((stat & I2C_SR1_ARLO) != 0) // arbitration lost
            return 1;

        if ((stat & I2C_SR1_ADDR) != 0) // ACK received
        {
            stat = I2C1->SR2; // clear ADDR flag
            return 0;         // success
        }

        if (timeout-- == 0)
            return 2; // timeout: slave did not ACK within timeout

      } while(1);
}

uint8_t i2c_sendAddrForRead(uint8_t addr)
{
	return i2c_sendAddr((addr<<1) + 1); // addr+Read(1) 
}

uint8_t i2c_sendAddrForWrite(uint8_t addr)
{	
	return i2c_sendAddr((addr<<1)); // addr+Write(0)
}

uint8_t i2c_sendData(uint8_t data)
{
	int stat1;
	I2C1->DR = data;
	
	do{
		stat1 = I2C1->SR1;
		if((stat1&(I2C_SR1_TXE)) != 0) // TxE = 1 
			return 0;
	}	while(1);
}

uint8_t i2c_readData(uint8_t ack)
{
	if(ack!= 0)
		I2C1->CR1 |=  I2C_CR1_ACK;//1<<10;
	else
		I2C1->CR1 &= 	~(I2C_CR1_ACK);//~(1<<10);
	while((I2C1->SR1&(I2C_SR1_RXNE)) == 0)		// waiting for RxNE 
	{
		__NOP();
	}
	return (I2C1->DR); 
}

