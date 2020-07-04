#include "tqt_uart.h"
#include "access_config.h"
#include "nrfx_gpiote.h"

bool CLOUD_REQUEST_ON = false;
bool CLOUD_REQUEST_OFF = false;
bool CLOUD_REQUEST_NETWORK_STT = false;
bool CLOUD_REQUEST_LOCK_STT = false;
bool CHECK_NEW_NODE_REQUEST = false;
bool INIT_CNT_REQUEST_UPDATE_MEST_STT = false;
static char data[30];
static uint8_t count = 0;
char REQUEST_ON[2] = "ON";
char REQUEST_OFF[3] = "OFF";
char REQUEST_MESH_STT[5] = "RENEW";
char LOAD_LOCK_STT[2] = "UP";
char INIT_MESH_STT_REQ[6] = "START\n";

void tqt_uart_interupt_handler(app_uart_evt_t * p_event)
{
    if(p_event->evt_type == APP_UART_DATA_READY) {
       char r_data;
      UNUSED_VARIABLE(app_uart_get(&r_data));
      data[count] = r_data;
      count++;
      if(strstr(data,REQUEST_ON)) {
          CLOUD_REQUEST_ON = true;
          count = 0;
          memset(data,'\0',sizeof(data));
          return;
      } else if(strstr(data,INIT_MESH_STT_REQ)) {
          INIT_CNT_REQUEST_UPDATE_MEST_STT = true;
          count = 0;
          memset(data,'\0',sizeof(data));
          return;
      } else if(strstr(data,LOAD_LOCK_STT)) {
          CLOUD_REQUEST_LOCK_STT = true;
          count = 0;
          memset(data,'\0',sizeof(data));
          return;
      } else if(strstr(data,REQUEST_OFF)) {
          CLOUD_REQUEST_OFF = true;
          count = 0;
          memset(data,'\0',sizeof(data));
          return;
      }  else if (strstr(data,REQUEST_MESH_STT)) {
          CLOUD_REQUEST_NETWORK_STT = true;
          count = 0;
          memset(data,'\0',sizeof(data));
          return;
      }  else {
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
          NRF_UART_BAUDRATE_115200
#else
          NRF_UARTE_BAUDRATE_115200
#endif
      };

    APP_UART_FIFO_INIT(&comm_params,
                         UART_RX_BUF_SIZE,
                         UART_TX_BUF_SIZE,
                         tqt_uart_interupt_handler,
                         NRF_MESH_IRQ_PRIORITY_THREAD,         // APP_IRQ_PRIORITY_LOWEST
                         err_code);
//                         __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "TQT RUNN\n");
    APP_ERROR_CHECK(err_code);
}

void tqt_send_char(char data) {
	while (app_uart_put(data) != NRF_SUCCESS);
//        __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "length: %c\n",data);
}

void tqt_send_string(char* w_data) {
  uint8_t length_of_data = strlen(w_data);
//  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "length: %d\n",length_of_data);
  for(int i = 0; i< length_of_data; i++)
  {
    while (app_uart_put(w_data[i]) != NRF_SUCCESS);
  }
}
