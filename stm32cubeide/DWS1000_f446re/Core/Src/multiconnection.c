/*
 * multiconnection.c
 *
 *  Created on: Oct 14, 2020
 *      Author: colson
 */
#include "deca_device_api.h"
#include "deca_regs.h"
#include "stdio.h"
#include "deca_spi.h"
#include "port.h"

#include <stdio.h>
#include <string.h>

#include "rng_typedef.h"
#include "main.h"

#define POLL_INTERVAL_INDEX 0
#define UART_TRANSMIT_INDEX 1
#define POLL_INTERVAL_TIME  50
#define UART_TRANSMIT_TIME  100

uint32_t nowTick[4] 	= {0,};
uint32_t pastTick[4] 	= {0,};

dw_dev dev[DW_DEV_MAX];

void dw1000_init(void)
{

}

void dw1000_main(void)
{
	//send poll message
	nowTick[POLL_INTERVAL_INDEX] = HAL_GetTick();
	if(nowTick[POLL_INTERVAL_INDEX] - pastTick[POLL_INTERVAL_INDEX] > POLL_INTERVAL_TIME)
	{
		//send poll message to all node
	}

	//read data
	//if there are rx data
	//check step (poll -> pollack -> range)
	//

	//save range

	//send to UART about result
	nowTick[UART_TRANSMIT_INDEX] = HAL_GetTick();
	if(nowTick[UART_TRANSMIT_INDEX] - pastTick[UART_TRANSMIT_INDEX] > UART_TRANSMIT_TIME)
	{
		//pos of neighbor
		//distance of neighbor
		//
	}
}






