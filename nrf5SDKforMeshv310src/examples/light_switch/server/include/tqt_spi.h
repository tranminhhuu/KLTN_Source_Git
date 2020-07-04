#ifndef TQT_SPI_H_
#define TQT_SPI_H_

#include "nrf_drv_spi.h"
#include "nrf_drv_spis.h"
#include "log.h"
#include "rtt_input.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_delay.h"
#include "nrfx_spim.h"
#include "tqt_common_func.h"
#include <string.h>
/*Define  Role for SPI module
	0: Role Master
	1: Role Slave
*/

#define SPI_ROLE 0
#define TEST_STRING "Nordic"
#define MFRC522_CS_LOW	nrf_gpio_pin_clear(SPI_SS_PIN);
#define MFRC522_CS_HIGH	nrf_gpio_pin_set(SPI_SS_PIN);
// uint32_t tqt_get_receive_buff = 0; 

static char* tqt_tx= "HELLO_TQT";
     
static uint8_t m_rx_buf[1];    
//static const uint8_t m_length = sizeof(m_tx_buf);  
void tqt_init_spi_master();
//uint8_t tqt_run_spi_master(uint8_t );
uint8_t tqt_run_spi_master(const char*);
void tqt_init_spi_slave();
void tqt_run_spi_slave();

#endif	//TQT_SPI_H_

