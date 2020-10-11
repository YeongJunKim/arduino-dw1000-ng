
#pragma once

#ifdef ARDUINO
#include <Arduino.h>
#include <SPI.h>
#endif
#ifdef STM32
#include "main.h"
#ifdef G071RB

#endif


#include <string>

typedef uint8_t byte;
typedef int boolean;

constexpr uint32_t EspSPImaximumSpeed = 20000000; //20MHz
constexpr uint32_t ArduinoSPImaximumSpeed = 16000000; //16MHz
constexpr uint32_t SPIminimumSpeed = 2000000; //2MHz



#ifdef G071RB
#define G071RB_GPIO_RST 		GPIOD
#define G071RB_GPIO_RST_PIN 	7
#endif



#endif




