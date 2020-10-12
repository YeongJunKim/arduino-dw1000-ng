/*
 * MIT License
 * 
 * Copyright (c) 2018 Michele Biondi, Andrea Salvatori
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * @file SPIporting.hpp
 * Arduino porting for the SPI interface.
*/

#ifdef STM32
#include "typedef.hpp"
#include "utils.hpp"
#else
 #include <Arduino.h>
#include <SPI.h>
#endif
#include "SPIporting.hpp"
#include "DW1000NgConstants.hpp"
#include "DW1000NgRegisters.hpp"

#ifdef STM32
namespace SPIporting {
namespace {

		constexpr uint32_t EspSPImaximumSpeed = 20000000; //20MHz
		constexpr uint32_t ArduinoSPImaximumSpeed = 16000000; //16MHz
		constexpr uint32_t SPIminimumSpeed = 2000000; //2MHz

		/* SPI relative variables */
		#if defined(ESP32) || defined(ESP8266)
			const SPISettings _fastSPI = SPISettings(EspSPImaximumSpeed, MSBFIRST, SPI_MODE0);
		#else
//			TODO
//			SPISettings _fastSPI = SPISettings(ArduinoSPImaximumSpeed, MSBFIRST, SPI_MODE0);
		#endif
//		TODO
//		const SPISettings _slowSPI = SPISettings(SPIminimumSpeed, MSBFIRST, SPI_MODE0);
//		const SPISettings* _currentSPI = &_fastSPI;

		void _openSPI(GPIO_TypeDef *GPIOx, uint16_t ss) {
//			TODO
			gpioWrite(GPIOx, ss, GPIO_PIN_RESET);
//			SPI.beginTransaction(*_currentSPI);
//			digitalWrite(slaveSelectPIN, LOW);
		}

    	void _closeSPI(GPIO_TypeDef *GPIOx, uint16_t ss) {
//    		TODO
			gpioWrite(GPIOx, ss, GPIO_PIN_SET);
//			digitalWrite(slaveSelectPIN, HIGH);
//			SPI.endTransaction();
		}
	}
    void SPIinit()
    {

    }
	void SPIend()
    {

    }
	void SPIselect(GPIO_TypeDef *GPIOx, uint8_t ss, uint8_t irq)
    {
		gpioMode(GPIOx, ss, GPIO_MODE_OUTPUT_PP);
		gpioWrite(GPIOx, ss, GPIO_PIN_SET);
    }
    void writeToSPI(uint8_t slaveSelectPIN, uint8_t headerLen, byte header[], uint16_t dataLen, byte data[])
    {
    	_openSPI(SPI1_DW_SS_GPIO_Port, SPI1_DW_SS_Pin);

    	_closeSPI(SPI1_DW_SS_GPIO_Port, SPI1_DW_SS_Pin);
    }
    void readFromSPI(uint8_t slaveSelectPIN, uint8_t headerLen, byte header[], uint16_t dataLen, byte data[])
    {
    	_openSPI(SPI1_DW_SS_GPIO_Port, SPI1_DW_SS_Pin);

    	_closeSPI(SPI1_DW_SS_GPIO_Port, SPI1_DW_SS_Pin);
    }
    void setSPIspeed(SPIClock speed)
    {

    }
}
#else
namespace SPIporting {
	
	namespace {

		constexpr uint32_t EspSPImaximumSpeed = 20000000; //20MHz
		constexpr uint32_t ArduinoSPImaximumSpeed = 16000000; //16MHz
		constexpr uint32_t SPIminimumSpeed = 2000000; //2MHz

		/* SPI relative variables */
		#if defined(ESP32) || defined(ESP8266)
			const SPISettings _fastSPI = SPISettings(EspSPImaximumSpeed, MSBFIRST, SPI_MODE0);
		#else
			const SPISettings _fastSPI = SPISettings(ArduinoSPImaximumSpeed, MSBFIRST, SPI_MODE0);
		#endif
		const SPISettings _slowSPI = SPISettings(SPIminimumSpeed, MSBFIRST, SPI_MODE0);
		const SPISettings* _currentSPI = &_fastSPI;

		void _openSPI(uint8_t slaveSelectPIN) {
			SPI.beginTransaction(*_currentSPI);
			digitalWrite(slaveSelectPIN, LOW);
		}

    	void _closeSPI(uint8_t slaveSelectPIN) {
			digitalWrite(slaveSelectPIN, HIGH);
			SPI.endTransaction();
		}
	}

	void SPIinit() {
		SPI.begin();
	}

	void SPIend() {
		SPI.end();
	}

	void SPIselect(uint8_t slaveSelectPIN, uint8_t irq) {
		#if !defined(ESP32) && !defined(ESP8266)
			if(irq != 0xff)
				SPI.usingInterrupt(digitalPinToInterrupt(irq));
		#endif
		pinMode(slaveSelectPIN, OUTPUT);
		digitalWrite(slaveSelectPIN, HIGH);
	}

	void writeToSPI(uint8_t slaveSelectPIN, uint8_t headerLen, byte header[], uint16_t dataLen, byte data[]) {
		_openSPI(slaveSelectPIN);
		for(auto i = 0; i < headerLen; i++) {
			SPI.transfer(header[i]); // send header
		}
		for(auto i = 0; i < dataLen; i++) {
			SPI.transfer(data[i]); // write values
		}
		delayMicroseconds(5);
		_closeSPI(slaveSelectPIN);
	}

    void readFromSPI(uint8_t slaveSelectPIN, uint8_t headerLen, byte header[], uint16_t dataLen, byte data[]){
		_openSPI(slaveSelectPIN);
		for(auto i = 0; i < headerLen; i++) {
			SPI.transfer(header[i]); // send header
		}
		for(auto i = 0; i < dataLen; i++) {
			data[i] = SPI.transfer(0x00); // read values
		}
		delayMicroseconds(5);
		_closeSPI(slaveSelectPIN);
	}

	void setSPIspeed(SPIClock speed) {
		if(speed == SPIClock::FAST) {
			_currentSPI = &_fastSPI;
		 } else if(speed == SPIClock::SLOW) {
			_currentSPI = &_slowSPI;
		 }
	}

}
#endif
