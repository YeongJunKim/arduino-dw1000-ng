/*
 * rng_typedef.h
 *
 *  Created on: Oct 14, 2020
 *      Author: colson
 */

#ifndef SRC_TYPEDEFS_RNG_TYPEDEF_H_
#define SRC_TYPEDEFS_RNG_TYPEDEF_H_



#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "stdio.h"

#include "deca_device_api.h"
#include "deca_regs.h"
#include "stdio.h"
#include "deca_spi.h"
#include "port.h"



#ifndef boolean
	typedef int boolean;
#endif

#ifndef byte
	typedef uint8_t byte;
#endif

#define LEN_DATA 127
#define DW_DEV_MAX 5


typedef struct _dw_dev
{
	uint16_t id;
	volatile byte expectedMsgId;
	volatile boolean sentAck;
	volatile boolean receivedAck;
	boolean protocolFailed;

	uint64_t timePollSent;
	uint64_t timePollReceived;
	uint64_t timePollAckSent;
	uint64_t timePollAckReceived;
	uint64_t timeRangeSent;
	uint64_t timeRangeReceived;
	uint64_t timeComputedRange;

	uint32_t lastActivity;
	uint32_t nowActivity;

	uint32_t ActivityResetPeriod;

	byte data[LEN_DATA];
} dw_dev;

typedef struct _dw_dev_config{
	uint16_t id;
} dw_deg_config;

dw_dev rng_dev[DW_DEV_MAX];

int rng_check_dev(uint16_t id);
uint16_t rng_find_empty_index_dev(void);
boolean rng_add_dev(uint16_t id, uint16_t index);
boolean rng_del_dev(uint16_t id);
boolean rng_init_dev(uint16_t index);

void rng_machine_sent_dev(uint8_t *data, uint16_t length);
void rng_machine_dev(uint8_t *data, uint16_t length);

void transmitRangeFailed(uint16_t targetid);
void transmitPollAck(uint16_t targetid);

static uint64 get_tx_timestamp_u64(void);
static uint64 get_rx_timestamp_u64(void);

uint64_t bytesAsValue(byte data[], uint8_t n);
double computeRangeAsymmetric(uint64_t timePollSent,
             	 	 	 	 uint64_t timePollReceived,
							 uint64_t timePollAckSent,
							 uint64_t timePollAckReceived,
							 uint64_t timeRangeSent,
							 uint64_t timeRangeReceived);
    //TODO Symmetric

    /**
    Removes bias from the target range

    returns the unbiased range
    */
    double correctRange(double range);

#ifdef __cplusplus
}
#endif
#endif /* SRC_TYPEDEFS_RNG_TYPEDEF_H_ */
