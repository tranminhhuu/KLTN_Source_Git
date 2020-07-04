/* Copyright (c) 2010 - 2018, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <string.h>

/* HAL */
#include "boards.h"
#include "simple_hal.h"
#include "app_timer.h"

/* Core */
#include "nrf_mesh_config_core.h"
#include "nrf_mesh_gatt.h"
#include "nrf_mesh_configure.h"
#include "nrf_mesh.h"
#include "mesh_stack.h"
#include "device_state_manager.h"
#include "access_config.h"

/* Provisioning and configuration */
#include "mesh_provisionee.h"
#include "mesh_app_utils.h"

/* Models */
#include "generic_onoff_client.h"

/* Logging and RTT */
#include "log.h"
#include "rtt_input.h"

/* Example specific includes */
#include "app_config.h"
#include "nrf_mesh_config_examples.h"
#include "light_switch_example_common.h"
#include "example_common.h"
#include "ble_softdevice_support.h"

#include "nrfx_gpiote.h"
#include "nrf_drv_gpiote.h"
#include "nrf_delay.h"
#include "app_onoff.h"
//#define APP_STATE_OFF                (0)
//#define APP_STATE_ON                 (1)
//tqt edit
typedef enum
{
    APP_STATE_OFF,
    APP_STATE_ON
} tqt_button_state;
tqt_button_state tqt_state = APP_STATE_OFF;
static bool      last_state = APP_STATE_OFF;
static tqt_button_state   last_press = APP_STATE_ON;
static bool      is_message_from_current_node = false;
static bool      current_state_of_server_node = false;
static bool      last_state_of_server_node = false;
static bool      is_send_message_before = false;
static bool      first_blood = false;
//tqt edit
#define APP_UNACK_MSG_REPEAT_COUNT   (2)
#define NODE_PROVISIONED              (LED_3)
#define SERVER_NODE_SEND_RES          (LED_1)

//tqt edit 12032019 start
#define TQT_APP_CONFIG_FORCE_SEGMENTATION  (false)
#define TQT_APP_CONFIG_MIC_SIZE            (0)
#define TQT_SERVER_MODEL_LISTEN_MESH_STATUS_INDEX   (3)
static void app_onoff_server_set_cb(const app_onoff_server_t * p_server, bool onoff);
static void app_onoff_server_get_cb(const app_onoff_server_t * p_server, bool * p_present_onoff);

APP_ONOFF_SERVER_DEF(server_model_listen_cloud_mesh_stt,
                     TQT_APP_CONFIG_FORCE_SEGMENTATION,
                     TQT_APP_CONFIG_MIC_SIZE,
                     app_onoff_server_set_cb,
                     app_onoff_server_get_cb)
static void app_onoff_server_set_cb(const app_onoff_server_t * p_server, bool onoff)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Receive Mesh NetWork Request");
    app_onoff_status_publish(&server_model_listen_cloud_mesh_stt);
}

static void app_onoff_server_get_cb(const app_onoff_server_t * p_server, bool * p_present_onoff)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Get cb");
    //*p_present_onoff = hal_led_pin_get(LOCK_STATUS);
}

//tqt edit 12032019 end
static generic_onoff_client_t m_clients[CLIENT_MODEL_INSTANCE_COUNT];
static bool                   m_device_provisioned;
//static bool                   last_press;
/* Forward declaration */
static void app_gen_onoff_client_publish_interval_cb(access_model_handle_t handle, void * p_self);
static void app_generic_onoff_client_status_cb(const generic_onoff_client_t * p_self,
                                               const access_message_rx_meta_t * p_meta,
                                               const generic_onoff_status_params_t * p_in);
static void app_gen_onoff_client_transaction_status_cb(access_model_handle_t model_handle,
                                                       void * p_args,
                                                       access_reliable_status_t status);

const generic_onoff_client_callbacks_t client_cbs =
{
    .onoff_status_cb = app_generic_onoff_client_status_cb,
    .ack_transaction_status_cb = app_gen_onoff_client_transaction_status_cb,
    .periodic_publish_cb = app_gen_onoff_client_publish_interval_cb
};

static void device_identification_start_cb(uint8_t attention_duration_s)
{
    hal_led_mask_set(LEDS_MASK, false);
    hal_led_blink_ms(BSP_LED_2_MASK  | BSP_LED_3_MASK,
                     LED_BLINK_ATTENTION_INTERVAL_MS,
                     LED_BLINK_ATTENTION_COUNT(attention_duration_s));
}

static void provisioning_aborted_cb(void)
{
    hal_led_blink_stop();
}

static void provisioning_complete_cb(void)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Successfully provisioned\n");
    nrf_gpio_pin_set(NODE_PROVISIONED);
#if MESH_FEATURE_GATT_ENABLED
    /* Restores the application parameters after switching from the Provisioning
     * service to the Proxy  */
    gap_params_init();
    conn_params_init();
#endif

    dsm_local_unicast_address_t node_address;
    dsm_local_unicast_addresses_get(&node_address);
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Node Address: 0x%04x \n", node_address.address_start);

    hal_led_blink_stop();
    hal_led_mask_set(LEDS_MASK, LED_MASK_STATE_OFF);
    hal_led_blink_ms(LEDS_MASK, LED_BLINK_INTERVAL_MS, LED_BLINK_CNT_PROV);
}

/* This callback is called periodically if model is configured for periodic publishing */
static void app_gen_onoff_client_publish_interval_cb(access_model_handle_t handle, void * p_self)
{
     __LOG(LOG_SRC_APP, LOG_LEVEL_WARN, "Publish desired message here.\n");
}

/* Acknowledged transaction status callback, if acknowledged transfer fails, application can
* determine suitable course of action (e.g. re-initiate previous transaction) by using this
* callback.
*/
static void app_gen_onoff_client_transaction_status_cb(access_model_handle_t model_handle,
                                                       void * p_args,
                                                       access_reliable_status_t status)
{
    switch(status)
    {
        case ACCESS_RELIABLE_TRANSFER_SUCCESS:
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Acknowledged transfer success.\n");
            break;

        case ACCESS_RELIABLE_TRANSFER_TIMEOUT:
            hal_led_blink_ms(LEDS_MASK, LED_BLINK_SHORT_INTERVAL_MS, LED_BLINK_CNT_NO_REPLY);
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Acknowledged transfer timeout.\n");
            break;

        case ACCESS_RELIABLE_TRANSFER_CANCELLED:
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Acknowledged transfer cancelled.\n");
            break;

        default:
            ERROR_CHECK(NRF_ERROR_INTERNAL);
            break;
    }
}

/* Generic OnOff client model interface: Process the received status message in this callback */
static void app_generic_onoff_client_status_cb(const generic_onoff_client_t * p_self,
                                               const access_message_rx_meta_t * p_meta,
                                               const generic_onoff_status_params_t * p_in)
{


//      if (first_blood == false) {
//          last_state_of_server_node = p_in->present_on_off;
//          __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "first blood: %d\n",p_in->target_on_off);
//          first_blood = true;
//      } else {
//          if (last_state_of_server_node == p_in->present_on_off) {
//              __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Throw\n");
//              return; 
//          } else {
//              last_state_of_server_node = p_in->present_on_off;  
              __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "State Feedback: %d\n",p_in->target_on_off);
//          }
//      }
}

static void node_reset(void)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "----- Node reset  -----\n");
    hal_led_blink_ms(LEDS_MASK, LED_BLINK_INTERVAL_MS, LED_BLINK_CNT_RESET);
    /* This function may return if there are ongoing flash operations. */
    mesh_stack_device_reset();
}

static void config_server_evt_cb(const config_server_evt_t * p_evt)
{
    if (p_evt->type == CONFIG_SERVER_EVT_NODE_RESET)
    {
        node_reset();
    }
}

static void models_init_cb(void)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Initializing and adding models\n");

    for (uint32_t i = 0; i < CLIENT_MODEL_INSTANCE_COUNT; ++i)
    {
        m_clients[i].settings.p_callbacks = &client_cbs;
        m_clients[i].settings.timeout = 0;
        m_clients[i].settings.force_segmented = APP_CONFIG_FORCE_SEGMENTATION;
        m_clients[i].settings.transmic_size = APP_CONFIG_MIC_SIZE;

        ERROR_CHECK(generic_onoff_client_init(&m_clients[i], i + 1));
    }

//tqt edit 12032019 start
      ERROR_CHECK(app_onoff_init(&server_model_listen_cloud_mesh_stt, TQT_SERVER_MODEL_LISTEN_MESH_STATUS_INDEX)); 
      __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Add new server model.\n");
//tqt edit 12032019 end

}

static void mesh_init(void)
{
    mesh_stack_init_params_t init_params =
    {
        .core.irq_priority       = NRF_MESH_IRQ_PRIORITY_LOWEST,
        .core.lfclksrc           = DEV_BOARD_LF_CLK_CFG,
        .core.p_uuid             = NULL,
        .models.models_init_cb   = models_init_cb,
        .models.config_server_cb = config_server_evt_cb
    };
    ERROR_CHECK(mesh_stack_init(&init_params, &m_device_provisioned));
}

static void start(void)
{

    if (!m_device_provisioned)
    {
        static const uint8_t static_auth_data[NRF_MESH_KEY_SIZE] = STATIC_AUTH_DATA;
        mesh_provisionee_start_params_t prov_start_params =
        {
            .p_static_data    = static_auth_data,
            .prov_complete_cb = provisioning_complete_cb,
            .prov_device_identification_start_cb = device_identification_start_cb,
            .prov_device_identification_stop_cb = NULL,
            .prov_abort_cb = provisioning_aborted_cb,
            .p_device_uri = EX_URI_LS_CLIENT
        };
        ERROR_CHECK(mesh_provisionee_prov_start(&prov_start_params));
    }

    mesh_app_uuid_print(nrf_mesh_configure_device_uuid_get());

    ERROR_CHECK(mesh_stack_start());

    hal_led_mask_set(LEDS_MASK, LED_MASK_STATE_OFF);
    hal_led_blink_ms(LEDS_MASK, LED_BLINK_INTERVAL_MS, LED_BLINK_CNT_START);
}

void in_pin_handler(nrfx_gpiote_pin_t button_number, nrf_gpiote_polarity_t action)
{
    uint32_t status = NRF_SUCCESS;
    generic_onoff_set_params_t set_params;
    model_transition_t transition_params;
    static uint8_t tid = 0;
    /* Button 1: On, Button 2: Off, Client[0]
     * Button 2: On, Button 3: Off, Client[1]
     */
    switch(button_number)
    {
     
        case BUTTON_1: {
////        if(last_press == APP_STATE_OFF) {
////            break;
////        } else {
////                __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "ON \n");
                set_params.on_off = APP_STATE_ON;
////                last_press = APP_STATE_OFF;
////                tqt_state = !tqt_state;
////            }

            Pressbutton(LED_1,LED_BLINK_SHORT_INTERVAL_MS);

            set_params.tid = tid++;
            transition_params.delay_ms = APP_CONFIG_ONOFF_DELAY_MS;
            transition_params.transition_time_ms = APP_CONFIG_ONOFF_TRANSITION_TIME_MS;
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Model Handle: %d\n",m_clients[0].model_handle);
            (void)access_model_reliable_cancel(m_clients[0].model_handle);
            status = generic_onoff_client_set(&m_clients[0], &set_params, &transition_params);
                  
            break;
            }
        case BUTTON_2: {
       
//        if(last_press == APP_STATE_OFF) {
//                __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "OFF \n");
                set_params.on_off = APP_STATE_OFF;
//                last_press = APP_STATE_ON;
//        } else {
//              break;
//              }
            set_params.tid = tid++;
            transition_params.delay_ms = APP_CONFIG_ONOFF_DELAY_MS;
            transition_params.transition_time_ms = APP_CONFIG_ONOFF_TRANSITION_TIME_MS;
            (void)access_model_reliable_cancel(m_clients[0].model_handle);
            status = generic_onoff_client_set(&m_clients[0], &set_params, &transition_params);
            //is_message_from_current_node = true;
            break;
            }
       case BUTTON_3: {
                        /* Clear all the states to reset the node. */
            if (mesh_stack_is_device_provisioned())
            {
#if MESH_FEATURE_GATT_PROXY_ENABLED
                (void) proxy_stop();
#endif
                mesh_stack_config_clear();
                node_reset();
            }
            else
            {
                __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "The device is unprovisioned. Resetting has no effect.\n");
            }
            break;
            }
    }
    switch (status)
    {
        case NRF_SUCCESS:
            break;

        case NRF_ERROR_NO_MEM:
        case NRF_ERROR_BUSY:
        case NRF_ERROR_INVALID_STATE:
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Client %u cannot send\n", button_number);
            hal_led_blink_ms(LEDS_MASK, LED_BLINK_SHORT_INTERVAL_MS, LED_BLINK_CNT_NO_REPLY);
            break;

        case NRF_ERROR_INVALID_PARAM:
            /* Publication not enabled for this client. One (or more) of the following is wrong:
             * - An application key is missing, or there is no application key bound to the model
             * - The client does not have its publication state set
             *
             * It is the provisioner that adds an application key, binds it to the model and sets
             * the model's publication state.
             */
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Publication not configured for client %u\n", button_number);
            break;

        default:
            ERROR_CHECK(status);
            break;
    }
    nrf_delay_ms(500);
}


void tqt_init_led()
{
   // nrf_gpio_pin_clear(LED_1);
    //nrf_gpio_pin_clear(LED_2);
    //nrf_gpio_pin_clear(LED_3);
    hal_led_blink_stop();
    hal_led_mask_set(LEDS_MASK, LED_MASK_STATE_OFF);
    //hal_led_blink_ms(LEDS_MASK, LED_BLINK_INTERVAL_MS, LED_BLINK_CNT_PROV);
  //  nrf_gpio_pin_clear(LED_4);
}

static void tqt_gpiote_button_ext_init(nrfx_gpiote_pin_t pin_cfg)
{

    nrfx_gpiote_init();
    nrfx_gpiote_in_config_t in_config = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(true);                   //NRFX_GPIOTE_CONFIG_IN_SENSE_TOGGLE ====> NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO
    in_config.pull = NRF_GPIO_PIN_PULLUP;

    nrfx_gpiote_in_init(pin_cfg, &in_config, in_pin_handler);
    nrfx_gpiote_in_event_enable(pin_cfg, false);
    nrfx_gpiote_in_event_disable(pin_cfg);
    nrfx_gpiote_in_event_enable(pin_cfg, true);
}

static void initialize(void)
{
    __LOG_INIT(LOG_SRC_APP | LOG_SRC_ACCESS, LOG_LEVEL_INFO, LOG_CALLBACK_DEFAULT);
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "----- BLE Mesh Light Switch Client Demo -----\n");

    ERROR_CHECK(app_timer_init());
   // hal_leds_init();

//#if BUTTON_BOARD
//    ERROR_CHECK(hal_buttons_init(button_event_handler));
//#endif
    bsp_board_init(BSP_INIT_LEDS);
    tqt_init_led();
    tqt_gpiote_button_ext_init(BUTTON_1);
    tqt_gpiote_button_ext_init(BUTTON_3);
//    tqt_gpiote_button_ext_init(BUTTON_4);
    tqt_gpiote_button_ext_init(BUTTON_2);

    ble_stack_init();
    #if MESH_FEATURE_GATT_ENABLED
    gap_params_init();
    conn_params_init();
    #endif
    /* Mesh Init */
    mesh_init();
}


int main(void)
{

sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE);
    initialize();
    start();
    for (;;)
    {
        (void)sd_app_evt_wait();
    }
}
