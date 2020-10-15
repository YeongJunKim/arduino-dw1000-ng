/*! ----------------------------------------------------------------------------
 *  @file    main.c
 *  @brief   Simple RX example code
 *
 * @attention
 *
 * Copyright 2015 (c) Decawave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 * @author Decawave
 */
#ifdef EX_02A_DEF
#include "deca_device_api.h"

#include "deca_regs.h"
#include "stdio.h"
#include "deca_spi.h"
#include "port.h"

#include <stdio.h>
#include <string.h>

#define stdio_write printf
/* Example application name and version to display. */
#define APP_NAME "SIMPLE RX v1.2"

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

/* Buffer to store received frame. See NOTE 1 below. */
#define FRAME_LEN_MAX 127
#define TX_TO_RX_DELAY_UUS 60

/* Receive response timeout, expressed in UWB microseconds. See NOTE 3 below. */
#define RX_RESP_TO_UUS 5000
static uint8 rx_buffer[FRAME_LEN_MAX];
static uint8 tx_msg[] = {0xC5, 0, 'D', 'E', 'C', 'A', 'W', 'A', 'V', 'E', 1, 3};

/* Hold copy of status register state here for reference so that it can be examined at a debug breakpoint. */
static uint32 status_reg = 0;

char dist_str[20] = {0};
/* Hold copy of frame length of frame received (if good) so that it can be examined at a debug breakpoint. */
static uint16 frame_len = 0;

static void rx_ok_cb(const dwt_cb_data_t *cb_data);
static void rx_to_cb(const dwt_cb_data_t *cb_data);
static void rx_err_cb(const dwt_cb_data_t *cb_data);
static void tx_conf_cb(const dwt_cb_data_t *cb_data);
uint32_t nTick = 0;
uint32_t pTick = 0;



uint32_t cbTxDoneCllabckCnt = 0;
uint32_t cbRxOkCallbackCnt = 0;
uint32_t cbRxToCallbackCnt = 0;
uint32_t cbRxErrCallbackCnt = 0;

uint32_t cbRxToCallbackFlag = 0;
/**
 * Application entry point.
 */
int dw_main(void)
{
    /* Display application name. */
    stdio_write(APP_NAME);
//    port_set_deca_isr(dwt_isr);

    /* Reset and initialise DW1000. See NOTE 2 below.
     * For initialisation, DW1000 clocks must be temporarily set to crystal speed. After initialisation SPI rate can be increased for optimum
     * performance. */
    reset_DW1000(); /* Target specific drive of RSTn line into DW1000 low for a period. */
    port_set_dw1000_slowrate();
    if (dwt_initialise(DWT_LOADNONE) == DWT_ERROR)
    {
        stdio_write("INIT FAILED");
        while (1)
        { };
    }
    port_set_dw1000_fastrate();

//#define DWT_INT_TFRS            0x00000080          // frame sent
//#define DWT_INT_LDED            0x00000400          // micro-code has finished execution
//#define DWT_INT_RFCG            0x00004000          // frame received with good CRC
//#define DWT_INT_RPHE            0x00001000          // receiver PHY header error
//#define DWT_INT_RFCE            0x00008000          // receiver CRC error
//#define DWT_INT_RFSL            0x00010000          // receiver sync loss error
//#define DWT_INT_RFTO            0x00020000          // frame wait timeout
//#define DWT_INT_RXOVRR          0x00100000          // receiver overrun
//#define DWT_INT_RXPTO           0x00200000          // preamble detect timeout
//#define DWT_INT_GPIO            0x00400000          // GPIO interrupt
//#define DWT_INT_SFDT            0x04000000          // SFD timeout
//#define DWT_INT_ARFE            0x20000000          // frame rejected (due to frame filtering configuration)

    /* Configure DW1000. */
    dwt_configure(&config);
//    dwt_setcallbacks(&tx_conf_cb, &rx_ok_cb, &rx_to_cb, &rx_err_cb);
//    dwt_setinterrupt(DWT_INT_TFRS | DWT_INT_RFCG | DWT_INT_RFTO | DWT_INT_RXPTO | DWT_INT_RPHE | DWT_INT_RFCE | DWT_INT_RFSL | DWT_INT_SFDT, 1);
//
//    dwt_setrxaftertxdelay(20);
//    dwt_setrxtimeout(500);


    uint8_t setID[8] = {1,2,3,4,5,6,7,8};

    dwt_seteui(setID);
    uint8_t getID[8] = {0,};
    dwt_geteui(getID);

    uint32_t lotid = 0;
    uint32_t partid = 0;
    lotid = dwt_getlotid();
    partid =  dwt_getpartid();
    /* Loop forever receiving frames. */
    while (1)
    {
        int i;

        /* TESTING BREAKPOINT LOCATION #1 */

        /* Clear local RX buffer to avoid having leftovers from previous receptions  This is not necessary but is included here to aid reading
         * the RX buffer.
         * This is a good place to put a breakpoint. Here (after first time through the loop) the local status register will be set for last event
         * and if a good receive has happened the data buffer will have the data in it, and frame_len will be set to the length of the RX frame. */
        for (i = 0 ; i < FRAME_LEN_MAX; i++ )
        {
            rx_buffer[i] = 0;
        }

        /* Activate reception immediately. See NOTE 3 below. */
        dwt_rxenable(DWT_START_RX_IMMEDIATE);

        /* Poll until a frame is properly received or an error/timeout occurs. See NOTE 4 below.
         * STATUS register is 5 bytes long but, as the event we are looking at is in the first byte of the register, we can use this simplest API
         * function to access it. */
        while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG | SYS_STATUS_ALL_RX_ERR )))
        { };

        if (status_reg & SYS_STATUS_RXFCG)
        {
            stdio_write("Frame received!\r\n");
            /* A frame has been received, copy it to our local buffer. */
            if (frame_len <= FRAME_LEN_MAX)
            frame_len = dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXFL_MASK_1023;

                dwt_readrxdata(rx_buffer, frame_len, 0);
                stdio_write("Data : ");
                for (int i = 0; i < frame_len; i++)
				{
                	printf("%02X,", rx_buffer[i]);
				}
                stdio_write("\r\n");


            /* Clear good RX frame event in the DW1000 status register. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG);
        }
        else
        {
            /* Clear RX error events in the DW1000 status register. */
        	printf("error occured in rx seqeunce \r\n");
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_ERR);
        }

        nTick = HAL_GetTick();
        if(nTick - pTick > 100)
        {
        	dwt_writetxdata(sizeof(tx_msg), tx_msg, 0);
        	dwt_writetxfctrl(sizeof(tx_msg), 0, 0);
        	dwt_starttx(DWT_START_TX_IMMEDIATE);
        	while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS)) {};

        	dwt_write32bitreg(SYS_STATE_ID, SYS_STATUS_TXFRS);
        	tx_msg[1]++;
        	pTick = nTick;
        }
    }
}


//typedef void (*dwt_cb_t)(const dwt_cb_data_t *);


static void tx_conf_cb(const dwt_cb_data_t *cb_data)
{
	cbTxDoneCllabckCnt++;
}
static void rx_ok_cb(const dwt_cb_data_t *cb_data)
{
	cbRxOkCallbackCnt++;
}
static void rx_to_cb(const dwt_cb_data_t *cb_data)
{
	cbRxToCallbackCnt++;
	cbRxToCallbackFlag = 1;
}
static void rx_err_cb(const dwt_cb_data_t *cb_data)
{
	cbRxErrCallbackCnt++;
}

#endif
/*****************************************************************************************************************************************************
 * NOTES:
 *
 * 1. In this example, maximum frame length is set to 127 bytes which is 802.15.4 UWB standard maximum frame length. DW1000 supports an extended
 *    frame length (up to 1023 bytes long) mode which is not used in this example.
 * 2. In this example, LDE microcode is not loaded upon calling dwt_initialise(). This will prevent the IC from generating an RX timestamp. If
 *    time-stamping is required, DWT_LOADUCODE parameter should be used. See two-way ranging examples (e.g. examples 5a/5b).
 * 3. Manual reception activation is performed here but DW1000 offers several features that can be used to handle more complex scenarios or to
 *    optimise system's overall performance (e.g. timeout after a given time, automatic re-enabling of reception in case of errors, etc.).
 * 4. We use polled mode of operation here to keep the example as simple as possible but RXFCG and error/timeout status events can be used to generate
 *    interrupts. Please refer to DW1000 User Manual for more details on "interrupts".
 * 5. The user is referred to DecaRanging ARM application (distributed with EVK1000 product) for additional practical example of usage, and to the
 *    DW1000 API Guide for more details on the DW1000 driver functions.
 ****************************************************************************************************************************************************/
