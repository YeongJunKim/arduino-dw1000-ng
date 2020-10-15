/*! ----------------------------------------------------------------------------
 *  @file    main.c
 *  @brief   TX then wait for response using interrupts example code
 *
 *           This is a variation of example 3a "TX then wait for a response" where events generated by DW1000 are handled through interrupts instead
 *           of polling the status register.
 *
 * @attention
 *
 * Copyright 2016 (c) Decawave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 * @author Decawave
 */
#ifdef EX_03D_DEF
#include "deca_device_api.h"
#include "deca_regs.h"
#include "stdio.h"
#include "deca_spi.h"
#include "port.h"

#include <stdio.h>
#include <string.h>

#include <rng_typedef.h>

#define stdio_write printf

#define APP_NAME "TX W4R IRQ v1.0"

/* Default communication configuration. We use here EVK1000's default mode (mode 3). */
static dwt_config_t config = {
    2,               /* Channel number. */
    DWT_PRF_64M,     /* Pulse repetition frequency. */
    DWT_PLEN_1024,   /* Preamble length. Used in TX only. */
    DWT_PAC32,       /* Preamble acquisition chunk size. Used in RX only. */
    3,               /* TX preamble code. Used in TX only. */
    3,               /* RX preamble code. Used in RX only. */
    1,               /* 0 to use standard SFD, 1 to use non-standard SFD. */
    DWT_BR_850K,     /* Data rate. */
    DWT_PHRMODE_STD, /* PHY header mode. */
    (1025 + 64 - 32) /* SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only. */
};
static uint8 tx_msg[] = {0xC5, 0, 'D', 'E', 'C', 'A', 'W', 'A', 'V', 'E', 0x43, 0x02, 0, 0};
/* Index to access the sequence number of the blink frame in the tx_msg array. */
#define BLINK_FRAME_SN_IDX 1
/* Delay from end of transmission to activation of reception, expressed in UWB microseconds (1 uus is 512/499.2 microseconds). See NOTE 2 below. */
#define TX_TO_RX_DELAY_UUS 60
/* Receive response timeout, expressed in UWB microseconds. See NOTE 3 below. */
#define RX_RESP_TO_UUS 50000
/* Default inter-frame delay period, in milliseconds. */
#define DFLT_TX_DELAY_MS 1000
/* Inter-frame delay period in case of RX timeout, in milliseconds.
 * In case of RX timeout, assume the receiver is not present and lower the rate of blink transmission. */
#define RX_TO_TX_DELAY_MS 3000
/* Inter-frame delay period in case of RX error, in milliseconds.
 * In case of RX error, assume the receiver is present but its response has not been received for any reason and retry blink transmission immediately. */
#define RX_ERR_TX_DELAY_MS 0
/* Current inter-frame delay period.
 * This global static variable is also used as the mechanism to signal events to the background main loop from the interrupt handler callbacks,
 * which set it to positive delay values. */
volatile static int32 tx_delay_ms = -1;

/* Buffer to store received frame. See NOTE 4 below. */
uint8_t rx_buffer[FRAME_LEN_MAX];

/* Declaration of static functions. */
static void rx_ok_cb(const dwt_cb_data_t *cb_data_);
static void rx_to_cb(const dwt_cb_data_t *cb_data_);
static void rx_err_cb(const dwt_cb_data_t *cb_data_);
static void tx_conf_cb(const dwt_cb_data_t *cb_data_);



volatile uint16_t rx_ok_flag = 0;
volatile uint16_t rx_to_flag = 0;
volatile uint16_t rx_err_flag = 0;
volatile uint16_t tx_conf_flag = 0;


dw_dev device[DW_DEV_MAX];
dwt_cb_data_t cb_data;

byte uid[8] = {0xFF, 0xFF, 0xFF, 0XFF, 1, 2, 3, 4};
#define PANID 0x0A

int dw_main(void)
{
    stdio_write(APP_NAME);
    port_set_deca_isr(dwt_isr);
    reset_DW1000(); /* Target specific drive of RSTn line into DW1000 low for a period. */
    port_set_dw1000_slowrate();
    if (dwt_initialise(DWT_LOADNONE) == DWT_ERROR)
    {
        stdio_write("INIT FAILED  \r\n");
        while (1)
        { };
    }
    port_set_dw1000_fastrate();

    dwt_seteui(uid);
    dwt_setpanid((uint16_t)PANID);


    dwt_configure(&config);
    dwt_setcallbacks(&tx_conf_cb, &rx_ok_cb, &rx_to_cb, &rx_err_cb);
    dwt_setinterrupt(DWT_INT_TFRS | DWT_INT_RFCG | DWT_INT_RFTO | DWT_INT_RXPTO | DWT_INT_RPHE | DWT_INT_RFCE | DWT_INT_RFSL | DWT_INT_SFDT, 1);
    dwt_setrxaftertxdelay(TX_TO_RX_DELAY_UUS);
    dwt_setrxtimeout(RX_RESP_TO_UUS);
    dwt_setrxantennadelay(RX_ANT_DLY);
    dwt_settxantennadelay(TX_ANT_DLY);


   	dwt_rxenable(DWT_START_RX_IMMEDIATE);


   	rng_dev[0].id = 0x1234;
   	rng_dev[0].role = ROLE_AS_TAG;
   	rng_dev[0].ActivityResetPeriod = 500;
    while (1)
    {

    	rng_machine_dev(rx_buffer, cb_data.datalength, 0);

//        if(rx_ok_flag)
//        {
//        	rng_machine_dev(rx_buffer, cb_data.datalength, 0);
//
//           	dwt_rxenable(DWT_START_RX_IMMEDIATE);
//        	rx_ok_flag = 0;
//        }

//        if(tx_conf_flag)
//        {
//        	extern uint8_t sendData[LEN_DATA];
//        	rng_machine_sent_dev(sendData, LEN_DATA);
//        	tx_conf_flag = 0;
//        }



    }
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn rx_ok_cb()
 * @brief Callback to process RX good frame events
 * @param  cb_data  callback data
 * @return  none
 */
static void rx_ok_cb(const dwt_cb_data_t *cb_data_)
{
    int i;

    /* Clear local RX buffer to avoid having leftovers from previous receptions. This is not necessary but is included here to aid reading the RX
     * buffer. */
    for (i = 0 ; i < FRAME_LEN_MAX; i++ )
    {
        rx_buffer[i] = 0;
    }

    /* A frame has been received, copy it to our local buffer. */
    if (cb_data_->datalength <= FRAME_LEN_MAX)
    {
        dwt_readrxdata(rx_buffer, cb_data_->datalength, 0);
//        stdio_write("Received data : ");
//		for (int i = 0; i < cb_data_->datalength; i++)
//		{
//			printf("%02X,", rx_buffer[i]);
//		}
//		stdio_write("\r\n");
    }
    cb_data = *cb_data_;

    rx_ok_flag = 1;
    /* TESTING BREAKPOINT LOCATION #1 */
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn rx_to_cb()
 * @brief Callback to process RX timeout events
 * @param  cb_data  callback data
 * @return  none
 */
static void rx_to_cb(const dwt_cb_data_t *cb_data_)
{
    /* Set corresponding inter-frame delay. */
    tx_delay_ms = RX_TO_TX_DELAY_MS;

    printf("received timeout \r\n");
    rx_to_flag = 1;
    /* TESTING BREAKPOINT LOCATION #2 */
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn rx_err_cb()
 * @brief Callback to process RX error events
 * @param  cb_data  callback data
 * @return  none
 */
static void rx_err_cb(const dwt_cb_data_t *cb_data_)
{
    /* Set corresponding inter-frame delay. */
    tx_delay_ms = RX_ERR_TX_DELAY_MS;

    printf("received error  \r\n");
    rx_err_flag = 1;
    /* TESTING BREAKPOINT LOCATION #3 */
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn tx_conf_cb()
 *
 * @brief Callback to process TX confirmation events
 *
 * @param  cb_data  callback data
 *
 * @return  none
 */
static void tx_conf_cb(const dwt_cb_data_t *cb_data_)
{
    /* This callback has been defined so that a breakpoint can be put here to check it is correctly called but there is actually nothing specific to
     * do on transmission confirmation in this example. Typically, we could activate reception for the response here but this is automatically handled
     * by DW1000 using DWT_RESPONSE_EXPECTED parameter when calling dwt_starttx().
     * An actual application that would not need this callback could simply not define it and set the corresponding field to NULL when calling
     * dwt_setcallbacks(). The ISR will not call it which will allow to save some interrupt processing time. */
//	extern uint8_t sendData[26];
//
//	for(int i = 0; i < 26; i ++)
//	{
//		printf("%02X,", sendData[i]);
//	}

//    printf("tx done \r\n");
    tx_conf_flag = 1;
    /* TESTING BREAKPOINT LOCATION #4 */
}
#endif
/*****************************************************************************************************************************************************
 * NOTES:
 *
 * 1. The device ID is a hard coded constant in the blink to keep the example simple but for a real product every device should have a unique ID.
 *    For development purposes it is possible to generate a DW1000 unique ID by combining the Lot ID & Part Number values programmed into the
 *    DW1000 during its manufacture. However there is no guarantee this will not conflict with someone else�s implementation. We recommended that
 *    customers buy a block of addresses from the IEEE Registration Authority for their production items. See "EUI" in the DW1000 User Manual.
 * 2. TX to RX delay can be set to 0 to activate reception immediately after transmission. But, on the responder side, it takes time to process the
 *    received frame and generate the response (this has been measured experimentally to be around 70 �s). Using an RX to TX delay slightly less than
 *    this minimum turn-around time allows the application to make the communication efficient while reducing power consumption by adjusting the time
 *    spent with the receiver activated.
 * 3. This timeout is for complete reception of a frame, i.e. timeout duration must take into account the length of the expected frame. Here the value
 *    is arbitrary but chosen large enough to make sure that there is enough time to receive a complete frame sent by the "RX then send a response"
 *    example at the 110k data rate used (around 3 ms).
 * 4. In this example, maximum frame length is set to 127 bytes which is 802.15.4 UWB standard maximum frame length. DW1000 supports an extended frame
 *    length (up to 1023 bytes long) mode which is not used in this example.
 * 5. In this example, LDE microcode is not loaded upon calling dwt_initialise(). This will prevent the IC from generating an RX timestamp. If
 *    time-stamping is required, DWT_LOADUCODE parameter should be used. See two-way ranging examples (e.g. examples 5a/5b).
 * 6. In a real application, for optimum performance within regulatory limits, it may be necessary to set TX pulse bandwidth and TX power, (using
 *    the dwt_configuretxrf API call) to per device calibrated values saved in the target system or the DW1000 OTP memory.
 * 7. dwt_writetxdata() takes the full size of tx_msg as a parameter but only copies (size - 2) bytes as the check-sum at the end of the frame is
 *    automatically appended by the DW1000. This means that our tx_msg could be two bytes shorter without losing any data (but the sizeof would not
 *    work anymore then as we would still have to indicate the full length of the frame to dwt_writetxdata()).
 * 8. The user is referred to DecaRanging ARM application (distributed with EVK1000 product) for additional practical example of usage, and to the
 *    DW1000 API Guide for more details on the DW1000 driver functions.
 ****************************************************************************************************************************************************/