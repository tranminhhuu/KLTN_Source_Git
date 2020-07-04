#ifndef TQT_UART_H_
#define TQT_UART_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "log.h"
#include "nrf_delay.h"
#include "app_uart.h"
#include "app_error.h"
#include "nrf.h"
#include "app_onoff.h"
#include "custom_board.h"
#include "generic_onoff_client.h"
#include "mesh_app_utils.h"
#if defined (UART_PRESENT)
#include "nrf_uart.h"
#endif
#if defined (UARTE_PRESENT)
#include "nrf_uarte.h"
#endif
#define MAX_TEST_DATA_BYTES     (15U)                /**< max number of test bytes to be used for tx and rx. */
#define UART_TX_BUF_SIZE 256                         /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE 256                         /**< UART RX buffer size. */

void tqt_init_uart();
void tqt_send_char(char );
void tqt_send_string(char*);
extern bool CLOUD_REQUEST_ON;
extern bool CLOUD_REQUEST_OFF;
extern bool CLOUD_REQUEST_NETWORK_STT;
extern bool CLOUD_REQUEST_LOCK_STT;
extern bool INIT_CNT_REQUEST_UPDATE_MEST_STT;
#endif	//TQT_UART_H_
