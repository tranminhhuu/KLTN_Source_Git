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
#define LOCK_STATUS               (LED_2)
#define MAX_TEST_DATA_BYTES     (15U)                /**< max number of test bytes to be used for tx and rx. */
#define UART_TX_BUF_SIZE 256                         /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE 1024                         /**< UART RX buffer size. */
#define READ_ID    "AT+ID\r\n"
#define VALID_ID_1 "ID:494AAE8E"
#define VALID_ID_2 "ID:50E7891E"

#define INIT_LOCK_PIN       nrf_gpio_cfg(UNLOCK_DOOR,\
                             NRF_GPIO_PIN_DIR_OUTPUT,\
                             NRF_GPIO_PIN_INPUT_CONNECT,\
                             NRF_GPIO_PIN_NOPULL,\
                             NRF_GPIO_PIN_S0S1,\
                             NRF_GPIO_PIN_NOSENSE);
#define TQT_APP_CONFIG_ONOFF_DELAY_MS           (50)              // tqt edit 30 -> 50 

/** Transition time value used by the OnOff client for sending OnOff Set messages. */
#define TQT_APP_CONFIG_ONOFF_TRANSITION_TIME_MS (100)            // tqt edit 50 -> 100 
//enum Check_ID {
//  SUCCESS,
//  CONTINUE,
//  STOP
//};
void tqt_init_uart();
void tqt_send_char(char );
void tqt_send_string(char*);
void read_RFID_data();
void init_unlock_pin();
//void active_unlock_pin();
//void deactive_unlock_pin();
void door_unlock();
void door_lock();
void tqt_send_state_to_provisioner(bool state);
void check_ID();
//extern bool last_lock_state;
//extern bool current_lock_state;
extern generic_onoff_client_t tqt_m_client;

extern bool READ_RFID;
extern bool RIGHT_PASSWORD;
extern bool WRONG_PASSWORD;
extern bool RECEIVE_RFID_DATA_OK;
extern char RFID_number[30];
extern char ID_of_card[30];
#endif	//TQT_UART_H_
