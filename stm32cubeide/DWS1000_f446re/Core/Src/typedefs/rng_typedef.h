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


#endif /* SRC_TYPEDEFS_RNG_TYPEDEF_H_ */
