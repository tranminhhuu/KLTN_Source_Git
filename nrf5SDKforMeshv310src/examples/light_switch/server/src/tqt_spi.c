#include "tqt_spi.h"
#include<stdio.h>
#include<stdlib.h>
#include "nrf_delay.h"
//#include "sdk_config.h"
/*Define  Role for SPI module
	0: Role Master
	1: Role Slave
*/
//#define TEST_STRING "Nordic"
//static uint8_t       m_tx_buf[] = TEST_STRING;           
//static uint8_t       m_rx_buf[sizeof(TEST_STRING) + 1];    
//static const uint8_t m_length = sizeof(m_tx_buf);  


#if SPI_ROLE == 0			 				//Master
#define SPI_INSTANCE  0
static const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(SPI_INSTANCE); 
static volatile bool spi_xfer_done; 
uint8_t tqt_data_tx[1];
uint8_t tqt_get;
void spi_master_event_handler(nrf_drv_spi_evt_t const * p_event,
                       void *                    p_context)
{
    spi_xfer_done = true;
    if (m_rx_buf[0] != 0)
    {
        tqt_get = m_rx_buf[0];
        __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "I: %d \n",  m_rx_buf[0]);
        //MFRC522_CS_HIGH;
    }
}

void tqt_init_spi_master() {
        nrf_gpio_cfg_output(SPI_SS_PIN);
    nrf_gpio_pin_set(SPI_SS_PIN);
    nrf_drv_spi_config_t spi_config = NRF_DRV_SPI_DEFAULT_CONFIG;
    spi_config.ss_pin   = SPI_SS_PIN;
    spi_config.miso_pin = SPI_MISO_PIN;
    spi_config.mosi_pin = SPI_MOSI_PIN;
    spi_config.sck_pin  = SPI_SCK_PIN;
    spi_config.frequency = SPI_FREQUENCY_FREQUENCY_K125;
    APP_ERROR_CHECK(nrf_drv_spi_init(&spi, &spi_config, spi_master_event_handler, NULL));
//    NRF_SPIM0->TXD.MAXCNT = 0;
}

uint8_t tqt_run_spi_master(const char* data_from_rfid) {
          spi_xfer_done = false;
         tqt_data_tx[0] = data_from_rfid[0];
         //MFRC522_CS_LOW;
          APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, tqt_data_tx, 1, m_rx_buf, 1));
          while (!spi_xfer_done)
          {
          __WFE();
          }
          NRF_LOG_FLUSH();
          return m_rx_buf[0];
}

#elif SPI_ROLE == 1	
											//Slave
#define SPIS_INSTANCE 0 /**< SPIS instance index. */
static const nrf_drv_spis_t spis = NRF_DRV_SPIS_INSTANCE(SPIS_INSTANCE);/**< SPIS instance. */
static volatile bool spis_xfer_done; /**< Flag used to indicate that SPIS instance completed the transfer. */

void spi_slave_event_handler(nrf_drv_spis_event_t event)
{
    if (event.evt_type == NRF_DRV_SPIS_XFER_DONE)
    {
        spis_xfer_done = true;
        __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, " Transfer completed. Received: %s\n",(uint32_t)m_rx_buf);
        NRF_LOG_INFO(" Transfer completed. Received: %s",(uint32_t)m_rx_buf);
       // nrf_delay_ms(2000);
    }
}
void tqt_init_spi_slave() {
    nrf_drv_spis_config_t spis_config = NRF_DRV_SPIS_DEFAULT_CONFIG;
    spis_config.csn_pin               = APP_SPIS_CS_PIN;
    spis_config.miso_pin              = APP_SPIS_MISO_PIN;
    spis_config.mosi_pin              = APP_SPIS_MOSI_PIN;
    spis_config.sck_pin               = APP_SPIS_SCK_PIN;
    APP_ERROR_CHECK(nrf_drv_spis_init(&spis, &spis_config, spi_slave_event_handler));  
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "SPI SLAVE Init OK.\n");
}

void tqt_run_spi_slave() {
        memset(m_rx_buf, 0, 1);
        spis_xfer_done = false;

        APP_ERROR_CHECK(nrf_drv_spis_buffers_set(&spis, NULL, NULL, m_rx_buf, 1));

        while (!spis_xfer_done)
        {
            __WFE();
        }

        NRF_LOG_FLUSH();
}
#endif 