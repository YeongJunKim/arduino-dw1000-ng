/*
 * rng_typedef.h
 *
 *  Created on: Oct 14, 2020
 *      Author: colson
 */

#ifndef SRC_TYPEDEFS_RNG_TYPEDEF_H_
#define SRC_TYPEDEFS_RNG_TYPEDEF_H_

#include "main.h"


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
	byte data[LEN_DATA];
} dw_dev;

typedef struct _dw_dev_config{
	uint16_t id;
} dw_deg_config;


boolean rng_check_dev(uint16_t id);
boolean rng_find_empty_index_dev(void);
boolean rng_add_dev(uint16_t id, uint16_t index);
boolean rng_del_dev(uint16_t id);
boolean rng_init_dev(uint16_t id);

void rng_machine_dev(uint8_t *data, uint16_t length);

dw_dev rng_dev[DW_DEV_MAX];

#endif /* SRC_TYPEDEFS_RNG_TYPEDEF_H_ */
