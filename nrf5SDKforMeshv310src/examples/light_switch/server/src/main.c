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
#include "proxy.h"

/* Provisioning and configuration */
#include "mesh_provisionee.h"
#include "mesh_app_utils.h"

/* Models */
#include "generic_onoff_server.h"
#include "generic_onoff_client.h"
/* Logging and RTT */
#include "log.h"
#include "rtt_input.h"

/* Example specific includes */
#include "app_config.h"
#include "example_common.h"
#include "nrf_mesh_config_examples.h"
#include "light_switch_example_common.h"
#include "app_onoff.h"
#include "ble_softdevice_support.h"
#include "nrfx_gpiote.h"
#include "nrf_drv_gpiote.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "tqt_uart.h"

//tqt edit
#include "app_scheduler.h"
#include <string.h>
#define NODE_PROVISIONED              (LED_3)
#define REQUEST_FROM_CLOUD            (LED_4)
//tqt edit
static int16_t tqt_count_package = 1;
#define STATE_OF_SERVER_LOCK          (LED_1)
static bool current_state = false;
static bool tqt_last_state = false;
#define APP_SCHED_EVENT_SIZE                16                                                    /**< Maximum size of scheduler events. */
#define APP_SCHED_QUEUE_SIZE                192  
                                                 /**< Maximum number of events in the scheduler queue. */
static bool m_device_provisioned;
static bool message_from_remote = false;
static bool get_new_message_from_remote = false;
static bool message_from_cloud = false;
static bool get_new_message_from_cloud = false;
#define TQT_NEW_MODEL_1          (1)
#define TQT_NEW_MODEL_2          (2)
#define TQT_CONFIG_SERVER_NODE_RELAY          (1)
#define TQT_CONFIG_SERVER_NODE_LOCK           (2)
#define TQT_CONFIG_SERVER_ROLE                (TQT_CONFIG_SERVER_NODE_LOCK)

//tqt edit 12032019 start      
#define TQT_SERVER_MODEL_LISTEN_MESH_STATUS_INDEX   (3)
static void app_onoff_server_set_cb(const app_onoff_server_t * p_server, bool onoff);
static void app_onoff_server_get_cb(const app_onoff_server_t * p_server, bool * p_present_onoff);

APP_ONOFF_SERVER_DEF(server_model_listen_cloud_mesh_stt,
                     APP_CONFIG_FORCE_SEGMENTATION,
                     APP_CONFIG_MIC_SIZE,
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


static void app_onoff_server0_set_cb(const app_onoff_server_t * p_server, bool onoff);                 
static void app_onoff_server0_get_cb(const app_onoff_server_t * p_server, bool * p_present_onoff);     
/* Generic OnOff server structure definition and initialization */
APP_ONOFF_SERVER_DEF(m_onoff_server_0,                        
                     APP_CONFIG_FORCE_SEGMENTATION,
                     APP_CONFIG_MIC_SIZE,
                     app_onoff_server0_set_cb,
                     app_onoff_server0_get_cb)

void tqt_send_state_to_provisioner(bool state) {
            //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "STATE TO PROVISIONER: %d\n",state);
            uint32_t status = NRF_SUCCESS;
            generic_onoff_set_params_t set_params;
            model_transition_t transition_params;
            static uint8_t tid = 0;
            set_params.on_off = state;
            set_params.tid = tid++;
            transition_params.delay_ms = TQT_APP_CONFIG_ONOFF_DELAY_MS;
            transition_params.transition_time_ms = TQT_APP_CONFIG_ONOFF_TRANSITION_TIME_MS;
            
            (void)access_model_reliable_cancel(tqt_m_client.model_handle);
            status = generic_onoff_client_set(&tqt_m_client, &set_params, &transition_params); 
            switch (status)
            {
                case NRF_SUCCESS:
                    break;

                case NRF_ERROR_NO_MEM:
                case NRF_ERROR_BUSY:
                case NRF_ERROR_INVALID_STATE:
                    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Client cannot send\n");
                    break;

                case NRF_ERROR_INVALID_PARAM:
                    /* Publication not enabled for this client. One (or more) of the following is wrong:
                     * - An application key is missing, or there is no application key bound to the model
                     * - The client does not have its publication state set
                     *
                     * It is the provisioner that adds an application key, binds it to the model and sets
                     * the model's publication state.
                     */
                    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Publication not configured for client \n");
                    break;

                default:
                    ERROR_CHECK(status);
                    break;
            }
}

static void app_onoff_server0_set_cb(const app_onoff_server_t * p_server, bool onoff)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Request From Remote: %d\n", onoff);
    app_onoff_status_publish(&m_onoff_server_0);
#if TQT_CONFIG_SERVER_ROLE == TQT_CONFIG_SERVER_NODE_LOCK
    message_from_remote = true;
    get_new_message_from_remote = onoff;
    //nrf_gpio_pin_set(LOCK_STATUS);
//    tqt_send_state_to_provisioner(onoff);
    if(onoff) {
      nrf_gpio_pin_set(LOCK_STATUS);
    } else {
      nrf_gpio_pin_clear(LOCK_STATUS);
    }
#elif TQT_CONFIG_SERVER_ROLE == TQT_CONFIG_SERVER_NODE_RELAY
    app_onoff_status_publish(&m_onoff_server_0);
    if(onoff) {
      nrf_gpio_pin_set(LOCK_STATUS);
    } else {
      nrf_gpio_pin_clear(LOCK_STATUS);
    }
#endif
}

static void app_onoff_server0_get_cb(const app_onoff_server_t * p_server, bool * p_present_onoff)
{
    *p_present_onoff = hal_led_pin_get(LOCK_STATUS);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                           Start:  ADD NEW MODEL: SERVER MODEL + CLIENT MODEL                                                                              //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if TQT_CONFIG_SERVER_ROLE == TQT_CONFIG_SERVER_NODE_RELAY
    static void app_onoff_server1_set_cb(const app_onoff_server_t * p_server, bool onoff);
    static void app_onoff_server1_get_cb(const app_onoff_server_t * p_server, bool * p_present_onoff);
// tqt edit: m_onoff_server_1 is the sever model of RELAY node to receive message from End Node (Lock) to send forward to Provisioner node
APP_ONOFF_SERVER_DEF(m_onoff_server_1,
                     APP_CONFIG_FORCE_SEGMENTATION,
                     APP_CONFIG_MIC_SIZE,
                     app_onoff_server1_set_cb,
                     app_onoff_server1_get_cb)
    static void app_onoff_server1_set_cb(const app_onoff_server_t * p_server, bool onoff)
{

    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Relay Message\n");
        if(onoff) {
      nrf_gpio_pin_set(STATE_OF_SERVER_LOCK);
    } else {
      nrf_gpio_pin_clear(STATE_OF_SERVER_LOCK);
    }
    app_onoff_status_publish(&m_onoff_server_1);
}

/* Callback for reading the hardware state */
static void app_onoff_server1_get_cb(const app_onoff_server_t * p_server, bool * p_present_onoff)
{
     __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "app_onoff_server1_get_cb RUN\n");
    //*p_present_onoff = hal_led_pin_get(LOCK_STATUS);
}

static void app_onoff_server2_set_cb(const app_onoff_server_t * p_server, bool onoff);
static void app_onoff_server2_get_cb(const app_onoff_server_t * p_server, bool * p_present_onoff);
// tqt edit: m_onoff_server_2 is the sever model of RELAY node to receive message from Provisioner Node to send forward to End node (Lock)
APP_ONOFF_SERVER_DEF(m_onoff_server_2,
                     APP_CONFIG_FORCE_SEGMENTATION,
                     APP_CONFIG_MIC_SIZE,
                     app_onoff_server2_set_cb,
                     app_onoff_server2_get_cb)
    static void app_onoff_server2_set_cb(const app_onoff_server_t * p_server, bool onoff)
{

    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Relay message from CLOUD number: %d\n",tqt_count_package);
        if(onoff) {
      nrf_gpio_pin_set(REQUEST_FROM_CLOUD);
    } else {
      nrf_gpio_pin_clear(REQUEST_FROM_CLOUD);
    }
    tqt_count_package++;
    app_onoff_status_publish(&m_onoff_server_2);
}

/* Callback for reading the hardware state */
static void app_onoff_server2_get_cb(const app_onoff_server_t * p_server, bool * p_present_onoff)
{
     __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "app_onoff_server_SP_get_cb RUN\n");
    //*p_present_onoff = hal_led_pin_get(LOCK_STATUS);
}

#elif TQT_CONFIG_SERVER_ROLE == TQT_CONFIG_SERVER_NODE_LOCK
// tqt edit tqt_m_client: is the client model to send request from End Node (Lock) to Provisioner
generic_onoff_client_t tqt_m_client;
static bool      first_blood = false;
static bool      last_state_of_server_node = false;
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
/* This callback is called periodically if model is configured for periodic publishing */
static void app_gen_onoff_client_publish_interval_cb(access_model_handle_t handle, void * p_self)
{
     __LOG(LOG_SRC_APP, LOG_LEVEL_WARN, "Publish desired message here.\n");
}

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
static void app_generic_onoff_client_status_cb(const generic_onoff_client_t * p_self,
                                               const access_message_rx_meta_t * p_meta,
                                               const generic_onoff_status_params_t * p_in)
{

        __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "State Feedback\n");

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
//              __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "State Feedback: %d\n",p_in->target_on_off);
//          }
//      }
}


// tqt edit: add server model to subscribe to provisioner receive request from cloud
  static void app_onoff_server1_set_cb(const app_onoff_server_t * p_server, bool onoff);
  static void app_onoff_server1_get_cb(const app_onoff_server_t * p_server, bool * p_present_onoff);
// tqt edit: m_onoff_server_1 is the sever model of END NODE to receive message from Provisioner Node (Cloud)
APP_ONOFF_SERVER_DEF(m_onoff_server_1,
                     APP_CONFIG_FORCE_SEGMENTATION,
                     APP_CONFIG_MIC_SIZE,
                     app_onoff_server1_set_cb,
                     app_onoff_server1_get_cb)
static void app_onoff_server1_set_cb(const app_onoff_server_t * p_server, bool onoff)
{
    message_from_cloud = true;
    get_new_message_from_cloud = onoff;
//    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "New Request From Cloud To Door: %d ---- Count Package: %d\n", onoff, tqt_count_package);
    app_onoff_status_publish(&m_onoff_server_1);
//    if(onoff) {
//      nrf_gpio_pin_set(REQUEST_FROM_CLOUD);
//      tqt_send_state_to_provisioner(true);
//      door_unlock();
//      tqt_count_package++;
//    } else {
//      tqt_send_state_to_provisioner(false);
//      nrf_gpio_pin_clear(REQUEST_FROM_CLOUD);
//      door_lock();
//      tqt_count_package++;
//    }
}

static void app_onoff_server1_get_cb(const app_onoff_server_t * p_server, bool * p_present_onoff)
{
     __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "app_onoff_server1_get_cb RUN\n");
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                           End:  ADD NEW MODEL: SERVER MODEL + CLIENT MODEL                                                                              //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void app_model_init(void)
{
    /* Instantiate onoff server on element index APP_ONOFF_ELEMENT_INDEX */
//tqt edit
    ERROR_CHECK(app_onoff_init(&m_onoff_server_0, 0));                  
    //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "App OnOff Model Handle: %d\n", m_onoff_server_0.server.model_handle);
    #if TQT_CONFIG_SERVER_ROLE == TQT_CONFIG_SERVER_NODE_RELAY
//tqt edit: This model use for relay server node
     ERROR_CHECK(app_onoff_init(&m_onoff_server_1, 1));                 
     ERROR_CHECK(app_onoff_init(&m_onoff_server_2, 2));
#elif TQT_CONFIG_SERVER_ROLE == TQT_CONFIG_SERVER_NODE_LOCK
//tqt edit: add new client model send request to provisioner
    tqt_m_client.settings.p_callbacks = &client_cbs;
    tqt_m_client.settings.timeout = 0;
    tqt_m_client.settings.force_segmented = APP_CONFIG_FORCE_SEGMENTATION;
    tqt_m_client.settings.transmic_size = APP_CONFIG_MIC_SIZE;
    ERROR_CHECK(generic_onoff_client_init(&tqt_m_client, 1));
//tqt edit: add new server model receive request from provisioner (Cloud)
    ERROR_CHECK(app_onoff_init(&m_onoff_server_1, 2));
#endif
    ERROR_CHECK(app_onoff_init(&server_model_listen_cloud_mesh_stt, TQT_SERVER_MODEL_LISTEN_MESH_STATUS_INDEX));
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Add Mesh NetWork status model.\n");
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

static void models_init_cb(void)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Initializing and adding server models\n");
    app_model_init();
}
void delay_timer( int timeout)
{
int TimerStart=0;
TimerStart= app_timer_cnt_get();
while(app_timer_cnt_get()-TimerStart <=timeout);
TimerStart=0;
}
static void mesh_init(void)
{
#if TQT_CONFIG_SERVER_ROLE == TQT_CONFIG_SERVER_NODE_LOCK
    mesh_stack_init_params_t init_params =
    {
        .core.irq_priority       = NRF_MESH_IRQ_PRIORITY_THREAD,        
        .core.lfclksrc           = DEV_BOARD_LF_CLK_CFG,
        .core.p_uuid             = NULL,
        .models.models_init_cb   = models_init_cb,
        .models.config_server_cb = config_server_evt_cb
    };
    ERROR_CHECK(mesh_stack_init(&init_params, &m_device_provisioned));
#elif TQT_CONFIG_SERVER_ROLE == TQT_CONFIG_SERVER_NODE_RELAY
    mesh_stack_init_params_t init_params =
    {
        .core.irq_priority       = NRF_MESH_IRQ_PRIORITY_LOWEST, 
        .core.lfclksrc           = DEV_BOARD_LF_CLK_CFG,
        .core.p_uuid             = NULL,
        .models.models_init_cb   = models_init_cb,
        .models.config_server_cb = config_server_evt_cb
    };
    ERROR_CHECK(mesh_stack_init(&init_params, &m_device_provisioned));
#endif
}

/**
*
*   @brief: Button event handler
*/
void in_pin_handler(nrfx_gpiote_pin_t button_number, nrf_gpiote_polarity_t action)
{
    
    uint32_t status = NRF_SUCCESS;
    generic_onoff_set_params_t set_params;
    model_transition_t transition_params;
    static uint8_t tid = 0;

    switch (button_number)
    {
        case BUTTON_3:
        {
            
            nrf_gpio_pin_clear(LED_3);
            delay_timer(5000);
            nrf_gpio_pin_set(LED_3);
            
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "BUTTON 3\n");
            break;
        }
      case BUTTON_4:
        {
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "BUTTON 4\n");
            break;
        }
        case BUTTON_1:
        {
            nrf_gpio_pin_toggle(LED_1);
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "BUTTON 1\n");
            break;
        }
        /* Initiate node reset */
        case BUTTON_2:
        {
        bsp_board_init(LED_2);
        __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "BUTTON 2\n");
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

        default:
            break;
    }
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

void tqt_init_led()
{
    nrf_gpio_pin_clear(LED_1);
    nrf_gpio_pin_clear(LED_2);
    nrf_gpio_pin_clear(LED_3);
    nrf_gpio_pin_clear(LED_4);
}

static void initialize(void)
{
    __LOG_INIT(LOG_SRC_APP | LOG_SRC_ACCESS | LOG_SRC_BEARER, LOG_LEVEL_INFO, LOG_CALLBACK_DEFAULT);
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "----- BLE Mesh Light Switch Server Demo -----\n");

    ERROR_CHECK(app_timer_init());

#if BUTTON_BOARD
    ERROR_CHECK(hal_buttons_init(button_event_handler));
#endif
    bsp_board_init(BSP_INIT_LEDS);
    tqt_init_led();
    tqt_gpiote_button_ext_init(BUTTON_1);
    tqt_gpiote_button_ext_init(BUTTON_3);
    tqt_gpiote_button_ext_init(BUTTON_4);
    tqt_gpiote_button_ext_init(BUTTON_2);

    ble_stack_init();
    #if MESH_FEATURE_GATT_ENABLED
    gap_params_init();
    conn_params_init();
    #endif
    /* Mesh Init */
    mesh_init();
}

static void start(void)
{
    //rtt_input_enable(app_rtt_input_handler, RTT_INPUT_POLL_PERIOD_MS);

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
            .p_device_uri = EX_URI_LS_SERVER
        };
        ERROR_CHECK(mesh_provisionee_prov_start(&prov_start_params));
    }

    mesh_app_uuid_print(nrf_mesh_configure_device_uuid_get());

    ERROR_CHECK(mesh_stack_start());

    hal_led_mask_set(LEDS_MASK, LED_MASK_STATE_OFF);
    hal_led_blink_ms(LEDS_MASK, LED_BLINK_INTERVAL_MS, LED_BLINK_CNT_START);
}

void check_ID() {
    if((strstr(ID_of_card,"ID:494AAE8E")) || (strstr(ID_of_card,"ID:50E7891E"))) {
        door_unlock();
        tqt_send_state_to_provisioner(true);
        memset(ID_of_card,'\0',sizeof(ID_of_card));
  } else {
        door_lock();
        tqt_send_state_to_provisioner(false);
        memset(ID_of_card,'\0',sizeof(ID_of_card));
  }
}

int main(void)
{ 
    initialize();
    start();
#if TQT_CONFIG_SERVER_ROLE == TQT_CONFIG_SERVER_NODE_LOCK
    APP_SCHED_INIT(APP_SCHED_EVENT_SIZE, APP_SCHED_QUEUE_SIZE);
    tqt_init_uart();
    init_unlock_pin();
#endif
    for (;;)
    { 
#if TQT_CONFIG_SERVER_ROLE == TQT_CONFIG_SERVER_NODE_LOCK
      app_sched_execute();
      bool done = nrf_mesh_process();
      if (done){
          if(RECEIVE_RFID_DATA_OK == true) {
            if(strstr(ID_of_card,"+TAG:1")) {
                  tqt_send_string(READ_ID);
                  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "AT COMMAND\n");
                  memset(ID_of_card,'\0',sizeof(ID_of_card));
                  RECEIVE_RFID_DATA_OK = false;
                  continue;
            } else if (strstr(ID_of_card,"+TAG:0")) {
                  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "TAG 0\n");
                  memset(ID_of_card,'\0',sizeof(ID_of_card));
                  RECEIVE_RFID_DATA_OK = false;
                  continue;
            } else if((strstr(ID_of_card,"ID:494AAE8E")) || (strstr(ID_of_card,"ID:50E7891E"))) {
                  door_unlock();
                  tqt_send_state_to_provisioner(true);
                  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "OPEN\n");
                  memset(ID_of_card,'\0',sizeof(ID_of_card));
                  RECEIVE_RFID_DATA_OK = false;
                  continue;
            } else {
                  door_lock();
                  tqt_send_state_to_provisioner(false);
                  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "CLOSE\n");
                  memset(ID_of_card,'\0',sizeof(ID_of_card));
                  RECEIVE_RFID_DATA_OK = false;
                  continue;
            }
          } else if (message_from_cloud == true) {
              __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "New Request From Cloud To Door: %d ---- Count Package: %d\n", get_new_message_from_cloud, tqt_count_package);
              if(get_new_message_from_cloud) {
                //nrf_gpio_pin_set(REQUEST_FROM_CLOUD);
                nrf_gpio_pin_set(UNLOCK_DOOR);
                nrf_gpio_pin_set(LOCK_STATUS);
              } else {
                //nrf_gpio_pin_clear(REQUEST_FROM_CLOUD);
                nrf_gpio_pin_clear(UNLOCK_DOOR);
                nrf_gpio_pin_clear(LOCK_STATUS);
              }
              tqt_send_state_to_provisioner(get_new_message_from_cloud);
              tqt_count_package++;
              message_from_cloud = false;
              continue;
          } else if (message_from_remote == true) {
          __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "New Request From Remote\n");
              tqt_send_state_to_provisioner(get_new_message_from_remote);
              if(get_new_message_from_remote == true) {
                nrf_gpio_pin_set(UNLOCK_DOOR);
                nrf_gpio_pin_set(LOCK_STATUS);
              } else {
                nrf_gpio_pin_clear(UNLOCK_DOOR);
                nrf_gpio_pin_clear(LOCK_STATUS);
              }
              message_from_remote = false;
              continue;
          } else {
              continue;
          }
        (void)sd_app_evt_wait();
     }
#elif TQT_CONFIG_SERVER_ROLE == TQT_CONFIG_SERVER_NODE_RELAY
     (void)sd_app_evt_wait();
#endif
   }
}