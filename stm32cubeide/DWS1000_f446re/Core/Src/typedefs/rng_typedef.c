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
boolean rng_find_empty_index_dev(void)
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
boolean rng_init_dev(uint16_t id)
{

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
		//check available id
	}

}
