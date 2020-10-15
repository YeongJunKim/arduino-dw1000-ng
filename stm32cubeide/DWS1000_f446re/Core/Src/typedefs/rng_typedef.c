/*
 * rng_typedef.c
 *
 *  Created on: 2020. 10. 15.
 *      Author: dud37
 */

#include "rng_typedef.h"

#define INFO printf




boolean rng_check_dev(uint16_t id)
{
	for(int i = 0; i < DW_DEV_MAX; i++) {
		if(rng_dev[i].id == id) {
			return 1;
		}
	}
	return 0;
}
uint16_t rng_find_empty_index_dev(void)
{
	for(int i = 0; i < DW_DEV_MAX; i++)	{
		if(rng_dev[i].id == 0x0000) {
			return i;
		}
	}
	return 0;
}
boolean rng_add_dev(uint16_t id, uint16_t index)
{
	if(rng_dev[index].id != 0x0000)
	{
		return 0;
	}
	rng_dev[index].id = id;
	return 1;
}
boolean rng_del_dev(uint16_t id)
{
	for(int i = 0; i < DW_DEV_MAX; i++)
	{
		if(rng_dev[i].id == id)
		{
			rng_dev[i].id = 0x0000;
			return 1;
		}
	}
	return 0;
}
boolean rng_init_dev(uint16_t index)
{

	for(int i = 0; i < LEN_DATA;i++)
	{
		rng_dev[index].data[i] = 0x00;
	}
	rng_dev[index].expectedMsgId 		= 0;
	rng_dev[index].protocolFailed 		= 0;
	rng_dev[index].receivedAck 			= 0;
	rng_dev[index].expectedMsgId 		= 0;
	rng_dev[index].sentAck 				= 0;
	rng_dev[index].receivedAck 			= 0;
	rng_dev[index].protocolFailed 		= 0;

	rng_dev[index].timePollSent 		= 0;
	rng_dev[index].timePollReceived 	= 0;
	rng_dev[index].timePollAckSent 		= 0;
	rng_dev[index].timePollAckReceived 	= 0;
	rng_dev[index].timeRangeSent 		= 0;
	rng_dev[index].timeRangeReceived 	= 0;
	rng_dev[index].timeComputedRange 	= 0;

	return 1;
}
void rng_machine_dev(uint8_t *data, uint16_t length)
{
	uint16_t id = 0x0000;

	id = data[16];
	id |= (data[17]<<8);
	//TODO check dev call rng_check_dev
	if(rng_check_dev(id))
	{

	}
	else
	{
		//Check available id
		uint16_t index = 0;
		index = rng_find_empty_index_dev();
		if(index) {
			//ADD ID
			rng_add_dev(id, index);
			rng_init_dev(index);
		}
	}
}
