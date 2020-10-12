
#include "utils.hpp"

#ifdef STM32
void delay(uint32_t val)
{

}

void delayMicroseconds(uint32_t val)
{

}

void bitSet(byte x, uint8_t bit_)
{

}

void bitClear(byte x, uint16_t bit_)
{

}
int bitRead(byte x, uint16_t bit_)
{

	return 0;
}




void gpioMode(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinMode mode)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = GPIO_Pin;
	GPIO_InitStruct.Mode = mode;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}
void gpioWrite(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState)
{
	HAL_GPIO_WritePin(GPIOx, GPIO_Pin, PinState);
}
#endif
