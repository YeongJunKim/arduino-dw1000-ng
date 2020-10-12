
#pragma once

#ifdef STM32
#include "main.h"

typedef uint8_t byte;
typedef uint32_t GPIO_PinMode;


void delay(uint32_t val);

void delayMicroseconds(uint32_t val);
void bitSet(byte x, uint8_t bit_);
void bitClear(byte x, uint16_t bit_);
int bitRead(byte x, uint16_t bit_);

void gpioMode(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinMode mode);
void gpioWrite(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState);


#else
#include <Arduino.h>
#include <SPI.h>
#endif

