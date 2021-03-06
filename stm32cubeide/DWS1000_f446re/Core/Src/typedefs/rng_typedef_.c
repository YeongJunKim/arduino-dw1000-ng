/*
 * rng_typedef.c
 *
 *  Created on: 2020. 10. 15.
 *      Author: dud37
 */

#include "rng_typedef.h"


#define RNG_DEBUG printf

#define POLL 0
#define POLL_ACK 1
#define POLL_ACK_LEN 5
#define RANGE 2
#define RANGE_REPORT 3
#define BROADCASTING 4

#define RANGE_FAILED 255


#define LOCAL_ADDRESS 0x2222

float DISTANCE_OF_RADIO     = 0.0046917639786159f;
float TIME_RES_INV = 63897.6f;

uint8_t sendData[LEN_DATA] = {0,};


uint32_t nowCheckTick = 0;
uint32_t pastCheckTick = 0;

int rng_check_dev(uint16_t id)
{
	for(int i = 0; i < DW_DEV_MAX; i++) {
		if(rng_dev[i].id == id) {
			return i;
		}
	}
	return -1;
}
uint16_t rng_find_empty_index_dev(void)
{
	for(int i = 0; i < DW_DEV_MAX; i++)	{
		if(rng_dev[i].id == 0x0000) {
			return i;
		}
	}
	return -1;
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
	rng_dev[index].protocolFailed 		= 0;
	rng_dev[index].receivedAck 			= 0;
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

	rng_dev[index].lastActivity 		= 0;
	rng_dev[index].nowActivity			= 0;
	rng_dev[index].ActivityResetPeriod  = 600; //100ms


	return 1;
}

void rng_machine_sent_dev(uint8_t *data, uint16_t length)
{
	uint16_t id = 0x0000;

	int index = 0;
	RNG_DEBUG("MACHINE SENT RUN \r\n");

	id = data[18];
	id |= (data[19] << 8);

	if(data[0] == POLL_ACK)
	{
		index = rng_check_dev(id);
		if(index != -1)
		{
			rng_dev[index].timePollAckSent = get_tx_timestamp_u64();
		}
	}


}


void rng_machine_dev(uint8_t *data, uint16_t length, uint8_t temp)
{
	uint16_t id = 0x0000;
	uint16_t targetid = 0x0000;
	int index = 0;
	extern volatile uint16_t rx_ok_flag;
	extern volatile uint16_t rx_to_flag;
	extern volatile uint16_t rx_err_flag;
	extern volatile uint16_t tx_conf_flag;



	// check activity
	if(rx_ok_flag == 0 && tx_conf_flag == 0)
	{
		for(int i = 0; i < DW_DEV_MAX; i++)
		{
			uint32_t nTick = HAL_GetTick();
			if(  rng_dev[i].id != 0x0000 )
			if(nTick - rng_dev[i].lastActivity > rng_dev[i].ActivityResetPeriod)
			{
				RNG_DEBUG("ACTIVITY FAILED, RESART DEV[%d] \r\n", i);

				if(rng_dev[i].role == ROLE_AS_TAG)
				{
					transmitPoll(rng_dev[i].id);
				   	dwt_rxenable(DWT_START_RX_IMMEDIATE);
				}
				else if(rng_dev[i].role == ROLE_AS_AHCHOR)
				{
					rng_dev[i].expectedMsgId = POLL;
				}

				rng_dev[i].lastActivity = nTick;
			}

		}
	}

	if(tx_conf_flag == 1)
	{
		RNG_DEBUG("TX COMPLETE \r\n");

		tx_conf_flag = 0;
	}

	if(rx_ok_flag == 1)
	{
		RNG_DEBUG("rxdata[%d]", length);
		for(int i = 0; i < length; i++)
		{
			RNG_DEBUG("%02X, ", data[i]);
		}
		RNG_DEBUG("\r\n");
       	id = data[16];
       		id |= (data[17]<<8);
       		targetid = data[18];
       		targetid |= (data[19]<<8);

       		if(targetid != LOCAL_ADDRESS) {
       			if(targetid == 0xFFFF)
       			{
       				RNG_DEBUG("INCOMMING ID %04X \r\n", id);
       				RNG_DEBUG("It is Broadcasting Poll message \r\n");
       			}
       			else
       			{
       				RNG_DEBUG("It is not correct address\r\n");
       			}
       		}


       		index = rng_check_dev(id);
       		if(index != -1)
       		{
       			//TODO there is exist data of specific id.
       			for(int i = 0; i <length; i++) {
       				rng_dev[index].data[i] = data[i];
       			}
       			if(rng_dev[index].data[0] != rng_dev[index].expectedMsgId)
       			{
       				RNG_DEBUG("PROTOCOL FAILED\r\n");
       				rng_dev[index].protocolFailed = 1;
       			}

       			switch(rng_dev[index].data[0])
       			{
       			case POLL:
       				RNG_DEBUG("[ANCHOR] POLL MESSAGE INCOME \r\n");
       				rng_dev[index].protocolFailed = 0;
       				rng_dev[index].timePollReceived = get_rx_timestamp_u64();
       				rng_dev[index].expectedMsgId = RANGE;
       				transmitPollAck(id);

       				break;
       			case POLL_ACK:
       				RNG_DEBUG("[TAG] POLL_ACK MESSAGE INCOME \r\n");
       				rng_dev[index].timePollSent = get_tx_timestamp_u64();
       				rng_dev[index].timePollAckReceived = get_rx_timestamp_u64();
       				rng_dev[index].expectedMsgId = RANGE_REPORT;
       				transmitRange(id);

       				break;
       			case RANGE:
       				RNG_DEBUG("[ANCHOR] RANGE MESSAGE INCOME \r\n");
       				rng_dev[index].timeRangeReceived = get_rx_timestamp_u64();
       				rng_dev[index].expectedMsgId = POLL;

       				if(rng_dev[index].protocolFailed == 0) {
       					RNG_DEBUG("PROTOCOL OK \r\n");
       					rng_dev[index].timePollSent 		= bytesAsValue(rng_dev[index].data + 1, 5);
       					rng_dev[index].timePollAckReceived 	= bytesAsValue(rng_dev[index].data + 6, 5);
       					rng_dev[index].timeRangeSent 		= bytesAsValue(rng_dev[index].data+11, 5);

       					double distance = computeRangeAsymmetric(rng_dev[index].timePollSent,
       							rng_dev[index].timePollReceived,
								rng_dev[index].timePollAckSent,
								rng_dev[index].timePollAckReceived,
								rng_dev[index].timeRangeSent,
								rng_dev[index].timeRangeReceived);
       					RNG_DEBUG("distance = %f \r\n",distance);

       					transmitRangeReport(distance, id);
       				}
       				else {
       					RNG_DEBUG("[ANCHOR] PROTOCOL FAILED \r\n");
       					transmitRangeFailed(id);
       				}

       				break;
       			case RANGE_REPORT:
       				RNG_DEBUG("[TAG] RANGE_REPORT MESSAGE INCOME \r\n");
       				transmitPoll(id);
       				break;
       			case RANGE_FAILED:
       				RNG_DEBUG("[TAG] RANGE_FAILED MESSAGE INCOME \r\n");
       				transmitPoll(id);
       				break;
       			}
       			rng_dev[index].lastActivity = HAL_GetTick();
       		}
       		else
       		{
       			RNG_DEBUG("There is no ID Information. -> Add process.. \r\n");
       			index = rng_find_empty_index_dev();
       			RNG_DEBUG("empty INDEX: %d \r\n", index);
       			if(index != -1) {
       				//ADD ID
       				rng_add_dev(id, index);
       				rng_init_dev(index);
       				RNG_DEBUG("ADD SEQUENCE DONE \r\n");
       			}
       		}
       		//clear message
       		for(int i = 0; i < LEN_DATA; i ++)
       			data[i] = 0;

           	dwt_rxenable(DWT_START_RX_IMMEDIATE);
       		rx_ok_flag = 0;
	}


}

void transmitPoll(uint16_t targetid) {
	RNG_DEBUG("transmit poll \r\n");
	sendData[0] = 0;
    sendData[16] = LOCAL_ADDRESS & 0xFF;
    sendData[17] = (LOCAL_ADDRESS  >> 8)& 0xFF;
	sendData[18] = targetid & 0xFF;
	sendData[19] = (targetid >> 8) & 0xFF;
	dwt_writetxdata(sizeof(sendData), sendData, 0);
	dwt_writetxfctrl(sizeof(sendData), 0, 0);
	dwt_starttx(DWT_START_TX_IMMEDIATE);

}
void transmitRange(uint16_t targetid) {
	sendData[0] = RANGE;

//    /* Calculation of future time */

    int index = rng_check_dev(targetid);


    rng_dev[index].timeRangeSent = get_sys_timestamp_u64();
    rng_dev[index].timeRangeSent = microsecondsToUWBTime(3000);
    dwt_setdelayedtrxtime(rng_dev[index].timeRangeSent);
    rng_dev[index].timeRangeSent += TX_ANT_DLY;

    writeValueToBytes(sendData+1, rng_dev[index].timePollSent, 5);
    writeValueToBytes(sendData + 6, rng_dev[index].timePollAckReceived, 5);
    writeValueToBytes(sendData+11, rng_dev[index].timeRangeSent, 5);

    sendData[16] = LOCAL_ADDRESS & 0xFF;
    sendData[17] = (LOCAL_ADDRESS  >> 8)& 0xFF;
	sendData[18] = targetid & 0xFF;
	sendData[19] = (targetid >> 8) & 0xFF;

	dwt_writetxdata(sizeof(sendData), sendData, 0);
	dwt_writetxfctrl(sizeof(sendData), 0, 0);
	dwt_starttx(DWT_START_TX_IMMEDIATE);
}

void transmitRangeReport(float curRange, uint16_t targetid) {
	sendData[0] = RANGE_REPORT;
    // write final ranging result

    sendData[16] = LOCAL_ADDRESS & 0xFF;
    sendData[17] = (LOCAL_ADDRESS  >> 8)& 0xFF;
	sendData[18] = targetid & 0xFF;
	sendData[19] = (targetid >> 8) & 0xFF;

	dwt_writetxdata(sizeof(sendData), sendData, 0);
	dwt_writetxfctrl(sizeof(sendData), 0, 0);
	dwt_starttx(DWT_START_TX_IMMEDIATE);
}

void transmitRangeFailed(uint16_t targetid) {
	sendData[0] = RANGE_FAILED;
	sendData[16] = LOCAL_ADDRESS & 0xFF;
	sendData[17] = (LOCAL_ADDRESS  >> 8)& 0xFF;
	sendData[18] = targetid & 0xFF;
	sendData[19] = (targetid >> 8) & 0xFF;


	dwt_writetxdata(sizeof(sendData), sendData, 0);
	dwt_writetxfctrl(sizeof(sendData), 0, 0);
	dwt_starttx(DWT_START_TX_IMMEDIATE);
}

void transmitPollAck(uint16_t targetid)
{
	RNG_DEBUG("TRANSMIT_POLL_ACK\r\n");
	sendData[0] = POLL_ACK;
	sendData[16] = LOCAL_ADDRESS & 0xFF;
	sendData[17] = (LOCAL_ADDRESS >> 8);
	sendData[18] = targetid & 0xFF;
	sendData[19] = (targetid >> 8) & 0xFF;

	dwt_writetxdata(sizeof(sendData), sendData, 0);
	dwt_writetxfctrl(sizeof(sendData), 0, 0);
	dwt_starttx(DWT_START_TX_IMMEDIATE);
}


uint64_t microsecondsToUWBTime(uint64_t microSeconds)
{
	return (uint64_t)(microSeconds * TIME_RES_INV);
}

uint64 get_sys_timestamp_u64(void){
	uint8_t ts_tab[5];
	uint64_t ts = 0;
	int i;
	dwt_readsystime(ts_tab);
	for (i = 4; i >=0; i--)
	{
		ts <<= 8;
		ts |= ts_tab[i];
	}
	return ts;
}
uint64 get_tx_timestamp_u64(void)
{
    uint8 ts_tab[5];
    uint64 ts = 0;
    int i;
    dwt_readtxtimestamp(ts_tab);
    for (i = 4; i >= 0; i--)
    {
        ts <<= 8;
        ts |= ts_tab[i];
    }
    return ts;
}
uint64 get_rx_timestamp_u64(void)
{
    uint8 ts_tab[5];
    uint64 ts = 0;
    int i;
    dwt_readrxtimestamp(ts_tab);
    for (i = 4; i >= 0; i--)
    {
        ts <<= 8;
        ts |= ts_tab[i];
    }
    return ts;
}

void writeValueToBytes(byte data[], uint64_t val, uint8_t n) {
	for(int i = 0; i < n; i++) {
		data[i] = ((val >> (i*8)) & 0xFF);
	}
}
uint64_t bytesAsValue(byte data[], uint8_t n) {
		uint64_t value = 0;
		for(int i = 0; i < n; i++) {
			value |= ((uint64_t)data[i] << (i*8));
		}
		return value;
	}

double computeRangeAsymmetric(uint64_t timePollSent,
             	 	 	 	  uint64_t timePollReceived,
							  uint64_t timePollAckSent,
							  uint64_t timePollAckReceived,
							  uint64_t timeRangeSent,
							  uint64_t timeRangeReceived)
{
	uint32_t timePollSent_32 		= (timePollSent);
	uint32_t timePollReceived_32 	= (timePollReceived);
	uint32_t timePollAckSent_32 	= (timePollAckSent);
	uint32_t timePollAckReceived_32 = (timePollAckReceived);
	uint32_t timeRangeSent_32 		= (timeRangeSent);
	uint32_t timeRangeReceived_32 	= (timeRangeReceived);

	double round1 = (double)(timePollAckReceived_32 - timePollSent_32);
	double reply1 = (double)(timePollAckSent_32 - timePollReceived_32);
	double round2 = (double)(timeRangeReceived_32 - timePollAckSent_32);
	double reply2 = (double)(timeRangeSent_32 - timePollAckReceived_32);

	int64_t tof_uwb = (int64_t)((round1 * round2 - reply1 * reply2) / (round1 + round2 + reply1 + reply2));
	double distance = tof_uwb * DISTANCE_OF_RADIO;

	return distance;
}
//