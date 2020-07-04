#include "tqt_uart.h"
#include "access_config.h"
#include "nrfx_gpiote.h"
char tqt_last_card[11] = "TQTTQTTQTTQ";
//bool last_lock_state = false;
//bool current_lock_state = false;
char* VALID_ID_1_ = VALID_ID_1;
char* VALID_ID_2_ = VALID_ID_2;
char tag1[6] = "+TAG:1";
char tag0[6] = "+TAG:0";
char garbage[2] = "\r\n";
char valid_id_1[11] = "ID:494AAE8E";
char valid_id_2[11] = "ID:50E7891E";
char RFID_number[30];
char r_data;
char ID_of_card[30];
static uint8_t count = 0;
static bool tqt_send_to_provisioner = 0;
static bool tqt_check;
static uint8_t check_end = 0;
bool READ_RFID = false;
bool RECEIVE_RFID_DATA_OK = false;

void tqt_uart_interupt_handler(app_uart_evt_t * p_event)
{
    if(p_event->evt_type == APP_UART_DATA_READY) {
      UNUSED_VARIABLE(app_uart_get(&r_data));
      RFID_number[count] = r_data;
        if((r_data == '\n') && (RFID_number[count-1] == '\r')) {
            if(check_end == 1) {
            count = 0;
            check_end = 0;
            strcpy(ID_of_card,RFID_number);
            memset(RFID_number,'\0',sizeof(RFID_number));
            RECEIVE_RFID_DATA_OK = true;
            return;
            } else {
              check_end++;
              return;
            }
        } else {
            count++;
            return;
        }
        }else if (p_event->evt_type == APP_UART_COMMUNICATION_ERROR) {
        APP_ERROR_HANDLER(p_event->data.error_communication);
    } else if (p_event->evt_type == APP_UART_FIFO_ERROR)  {
        APP_ERROR_HANDLER(p_event->data.error_code);
    }
}


void tqt_init_uart() {
    uint32_t err_code;
    const app_uart_comm_params_t comm_params =
      {
          RX_PIN_NUMBER,
          TX_PIN_NUMBER,
          RTS_PIN_NUMBER,
          CTS_PIN_NUMBER,
          APP_UART_FLOW_CONTROL_DISABLED,
          false,
#if defined (UART_PRESENT)
          NRF_UART_BAUDRATE_9600
#else
          NRF_UARTE_BAUDRATE_115200
#endif
      };

    APP_UART_FIFO_INIT(&comm_params,
                         UART_RX_BUF_SIZE,
                         UART_TX_BUF_SIZE,
                         tqt_uart_interupt_handler,
                         APP_IRQ_PRIORITY_THREAD,         // APP_IRQ_PRIORITY_LOWEST
                         err_code);
    APP_ERROR_CHECK(err_code);
}

void tqt_send_char(char data) {
	while (app_uart_put(data) != NRF_SUCCESS);
        __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "length: %c\n",data);
}

void tqt_send_string(char* w_data) {
  uint8_t length_of_data = strlen(w_data);
  for(int i = 0; i< 7; i++)
  {
    while (app_uart_put(w_data[i]) != NRF_SUCCESS);
  }
}

//void check_ID() {
//    if((strstr(ID_of_card,valid_id_1)) || (strstr(ID_of_card,valid_id_2))) {
//        door_unlock();
//        tqt_send_state_to_provisioner(true);
//        memset(ID_of_card,'\0',sizeof(RFID_number));
//  } else {
//        door_lock();
//        tqt_send_state_to_provisioner(false);
//        memset(ID_of_card,'\0',sizeof(RFID_number));
//  }
//}

void init_unlock_pin() {
      nrf_gpio_cfg(
        UNLOCK_DOOR,
        NRF_GPIO_PIN_DIR_OUTPUT,
        NRF_GPIO_PIN_INPUT_DISCONNECT,
        NRF_GPIO_PIN_NOPULL,
        NRF_GPIO_PIN_S0S1,
        NRF_GPIO_PIN_NOSENSE);
      nrf_gpio_pin_clear(UNLOCK_DOOR);
}

void door_unlock() {
    nrf_gpio_pin_set(UNLOCK_DOOR);
    nrf_gpio_pin_set(LOCK_STATUS);
}

void door_lock() {
  nrf_gpio_pin_clear(UNLOCK_DOOR);
  nrf_gpio_pin_clear(LOCK_STATUS);
}

//void active_unlock_pin() {
//  nrf_gpio_cfg_output(UNLOCK_DOOR);
//  nrf_gpio_pin_clear(UNLOCK_DOOR);
//}
//
//void deactive_unlock_pin() {
//  nrf_gpio_cfg_output(UNLOCK_DOOR);
//  nrf_gpio_pin_set(UNLOCK_DOOR);
//}