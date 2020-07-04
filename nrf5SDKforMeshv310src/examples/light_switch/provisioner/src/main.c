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
#include "nrf_delay.h"
#include "simple_hal.h"
#include "app_timer.h"

/* Core */
#include "nrf_mesh.h"
#include "nrf_mesh_events.h"
#include "nrf_mesh_assert.h"
#include "access_config.h"
#include "device_state_manager.h"
#include "flash_manager.h"
#include "mesh_stack.h"
#include "net_state.h"

/* Provisioning and configuration */
#include "provisioner_helper.h"
#include "node_setup.h"
#include "mesh_app_utils.h"

/* Models */
#include "config_client.h"
#include "config_server.h"
#include "health_client.h"

/* Logging and RTT */
#include "rtt_input.h"
#include "log.h"

/* Example specific includes */
#include "light_switch_example_common.h"
#include "example_network_config.h"
#include "nrf_mesh_config_examples.h"
#include "ble_softdevice_support.h"
#include "example_common.h"
#include "nrfx_gpiote.h"
#include "nrf_drv_gpiote.h"
#include "generic_onoff_client.h"

//tqt edit
#include "app_onoff.h"
#include "tqt_uart.h"
#include "app_scheduler.h"
#define APP_SCHED_EVENT_SIZE                16                                                    /**< Maximum size of scheduler events. */
#define APP_SCHED_QUEUE_SIZE                192                                                   /**< Maximum number of events in the scheduler queue. */
//tqt edit
#define APP_NETWORK_STATE_ENTRY_HANDLE (0x0001)
#define APP_FLASH_PAGE_COUNT           (1)
#define APP_PROVISIONING_LED            BSP_LED_0
#define APP_CONFIGURATION_LED           BSP_LED_1
#define TQT_APP_CONFIG_FORCE_SEGMENTATION  (false)
#define TQT_APP_CONFIG_MIC_SIZE            (0)
#define TQT_APP_CONFIG_ONOFF_DELAY_MS           (50)
#define TQT_APP_CONFIG_ONOFF_TRANSITION_TIME_MS (100)
#define MESH_NETWORK_REQUEST            (LED_4)
//#define NUMBER_OF_DEVICES_IN_MESH            (2)
static bool FINISHED_GET_NODE_ALIVE_SIGNAL = false;
static bool PROVISIONER_REQUEST = false;
/* Required for the provisioner helper module */
static network_dsm_handles_data_volatile_t m_dev_handles;
static network_stats_data_stored_t m_nw_state;        //tqt edit: change to extern in node_setup.h
static bool m_node_prov_setup_started;
static bool last_door_state_to_server = false;
//static bool save_door_status = false;
static bool door_state_to_server = false;
static bool tqt_state_network_update_ok = false;
bool tqt_not_use = false;
static uint8_t counter_request_from_cloud = 0;
/* Forward declarations */
static void app_health_event_cb(const health_client_t * p_client, const health_client_evt_t * p_event);
static void app_config_successful_cb(void);
static void app_config_failed_cb(void);
static void app_mesh_core_event_cb (const nrf_mesh_evt_t * p_evt);

static void app_start(void);

static nrf_mesh_evt_handler_t m_mesh_core_event_handler = { .evt_cb = app_mesh_core_event_cb };

/*****************************************************************************/
/**** Flash handling ****/
#if PERSISTENT_STORAGE

static flash_manager_t m_flash_manager;

static void app_flash_manager_add(void);

static void flash_write_complete(const flash_manager_t * p_manager, const fm_entry_t * p_entry, fm_result_t result)
{
     __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Flash write complete\n");

    /* If we get an AREA_FULL then our calculations for flash space required are buggy. */
    NRF_MESH_ASSERT(result != FM_RESULT_ERROR_AREA_FULL);

    /* We do not invalidate in this module, so a NOT_FOUND should not be received. */
    NRF_MESH_ASSERT(result != FM_RESULT_ERROR_NOT_FOUND);
    if (result == FM_RESULT_ERROR_FLASH_MALFUNCTION)
    {
        ERROR_CHECK(NRF_ERROR_NO_MEM);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//tqt Edit Start: add client model to provisioner to send request from Cloud to Server End Node
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define TQT_CLIENT_MODEL_ELEMENT       (2)
generic_onoff_client_t tqt_pro_m_client;
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
        
        __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "State Feedback: %d  src_address: 0x%04X dst_address: 0x%04X\n",p_in->target_on_off, p_meta->src.value, p_meta->dst.value);
}

void tqt_run_config_client_model() {
    static access_model_id_t model_id;
    model_id.model_id = GENERIC_ONOFF_CLIENT_MODEL_ID;
    model_id.company_id = ACCESS_COMPANY_ID_NONE;
    access_model_handle_t model_handle;
    access_handle_get(TQT_CLIENT_MODEL_ELEMENT,model_id,&model_handle);        
    dsm_handle_t publish_address_handle;
    dsm_address_publish_add(GROUP_ADDRESS_TQT_CLOUD, &publish_address_handle);
    
    access_model_publish_address_set(model_handle, publish_address_handle);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//tqt Edit End: add client model to provisioner to send request from Cloud to Server End Node
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//tqt edit 12032019 start

#define TQT_CLIENT_MODEL_MESH_STATUS_INDEX       (3)
generic_onoff_client_t tqt_mesh_network_status;
static void tqt_client_public_interval_cb(access_model_handle_t handle, void * p_self);
static void tqt_mesh_node_send_feedback_cb(const generic_onoff_client_t * p_self,
                                               const access_message_rx_meta_t * p_meta,
                                               const generic_onoff_status_params_t * p_in);
static void tqt_client_transaction_status_cb(access_model_handle_t model_handle,
                                                       void * p_args,
                                                       access_reliable_status_t status);

const generic_onoff_client_callbacks_t tqt_mesh_network_status_client_cbs =
{
    .onoff_status_cb = tqt_mesh_node_send_feedback_cb,
    .ack_transaction_status_cb = tqt_client_transaction_status_cb,
    .periodic_publish_cb = tqt_client_public_interval_cb
};
static void tqt_client_public_interval_cb(access_model_handle_t handle, void * p_self)
{
     __LOG(LOG_SRC_APP, LOG_LEVEL_WARN, "Publish desired message here.\n");
}
static void tqt_client_transaction_status_cb(access_model_handle_t model_handle,
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
                FINISHED_GET_NODE_ALIVE_SIGNAL = true;
                counter_request_from_cloud = 0;
            break;

        default:
            ERROR_CHECK(NRF_ERROR_INTERNAL);
            break;
    }
}
static void tqt_mesh_node_send_feedback_cb(const generic_onoff_client_t * p_self,
                                               const access_message_rx_meta_t * p_meta,
                                               const generic_onoff_status_params_t * p_in)
{
        
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Request Mesh Network Status:src_address: 0x%04X TQT_counter: %d\n", p_meta->src.value, counter_request_from_cloud);
    switch (counter_request_from_cloud) {
        case 0:
            break;
        case 1: case 2: case 3:
            addr_of_node_model_alive[counter_node_signal_index] = p_meta->src.value;
            counter_node_signal_index++;
            break;
        case 4:
            FINISHED_GET_NODE_ALIVE_SIGNAL = true;
            counter_request_from_cloud = 0;
            break;
        default:
            break;
    }
}

void tqt_run_config_mesh_network_stt() {
    static access_model_id_t model_id;
    model_id.model_id = GENERIC_ONOFF_CLIENT_MODEL_ID;
    model_id.company_id = ACCESS_COMPANY_ID_NONE;
    access_model_handle_t model_handle;
    access_handle_get(TQT_CLIENT_MODEL_MESH_STATUS_INDEX,model_id,&model_handle);        
    dsm_handle_t publish_address_handle;
    dsm_address_publish_add(GROUP_ADDRESS_TQT_MESH_STATUS, &publish_address_handle);
    
    access_model_publish_address_set(model_handle, publish_address_handle);
}


//tqt edit 12032019 end

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//tqt Edit Start: add server model to provisioner to receive message from Server End Node
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define TQT_SERVER_MODEL_ELEMENT       (1)
uint8_t*  tqt_appkey;
static uint16_t m_appkey_idx = 0;
#define APP_ONOFF_ELEMENT_INDEX     (0)
#define LOCK_STATUS               (LED_2)
static void app_onoff_server_set_cb(const app_onoff_server_t * p_server, bool onoff);
static void app_onoff_server_get_cb(const app_onoff_server_t * p_server, bool * p_present_onoff);

APP_ONOFF_SERVER_DEF(server_in_prov_node,
                     TQT_APP_CONFIG_FORCE_SEGMENTATION,
                     TQT_APP_CONFIG_MIC_SIZE,
                     app_onoff_server_set_cb,
                     app_onoff_server_get_cb)
static void app_onoff_server_set_cb(const app_onoff_server_t * p_server, bool onoff)
{
    if(onoff) {
      nrf_gpio_pin_set(LOCK_STATUS);
      //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "DOOR ON\n");
      door_state_to_server = true;
      //tqt_send_state_to_end_node(onoff);
    } else {
      nrf_gpio_pin_clear(LOCK_STATUS);
      door_state_to_server = false;
      //tqt_send_state_to_end_node(onoff);
    }
    app_onoff_status_publish(&server_in_prov_node);
}

static void app_onoff_server_get_cb(const app_onoff_server_t * p_server, bool * p_present_onoff)
{

    *p_present_onoff = hal_led_pin_get(LOCK_STATUS);
}

void tqt_run_config_server_model() {
    static access_model_id_t model_id;
    model_id.model_id = GENERIC_ONOFF_SERVER_MODEL_ID;
    model_id.company_id = ACCESS_COMPANY_ID_NONE;
    access_model_handle_t model_handle;
    access_handle_get(TQT_SERVER_MODEL_ELEMENT,model_id,&model_handle);        //

    dsm_handle_t subscription_address_handle;
    dsm_address_subscription_add(GROUP_ADDRESS_TQT, &subscription_address_handle);
    
    access_model_subscription_add(model_handle, subscription_address_handle);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//tqt Edit End: add server model to provisioner to receive message from Server End Node
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void flash_invalidate_complete(const flash_manager_t * p_manager, fm_handle_t handle, fm_result_t result)
{
    /* This application does not expect invalidate complete calls. */
    ERROR_CHECK(NRF_ERROR_INTERNAL);
}

typedef void (*flash_op_func_t) (void);
static void flash_manager_mem_available(void * p_args)
{
    ((flash_op_func_t) p_args)(); /*lint !e611 Suspicious cast */
}


static void flash_remove_complete(const flash_manager_t * p_manager)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Flash remove complete\n");
}

static void app_flash_manager_add(void)
{

    static fm_mem_listener_t flash_add_mem_available_struct = {
        .callback = flash_manager_mem_available,
        .p_args = app_flash_manager_add
    };

    const uint32_t * start_address;
    uint32_t allocated_area_size;
    ERROR_CHECK(mesh_stack_persistence_flash_usage(&start_address, &allocated_area_size));

    flash_manager_config_t manager_config;
    manager_config.write_complete_cb = flash_write_complete;
    manager_config.invalidate_complete_cb = flash_invalidate_complete;
    manager_config.remove_complete_cb = flash_remove_complete;
    manager_config.min_available_space = WORD_SIZE;
    manager_config.p_area = (const flash_manager_page_t *)((uint32_t)start_address - PAGE_SIZE * APP_FLASH_PAGE_COUNT);
    manager_config.page_count = APP_FLASH_PAGE_COUNT;
    uint32_t status = flash_manager_add(&m_flash_manager, &manager_config);
    if (NRF_SUCCESS != status)
    {
        flash_manager_mem_listener_register(&flash_add_mem_available_struct);
        __LOG(LOG_SRC_APP, LOG_LEVEL_ERROR, "Unable to add flash manager for app data\n");
    }
}

static bool load_app_data(void)
{
    uint32_t length = sizeof(m_nw_state);
    uint32_t status = flash_manager_entry_read(&m_flash_manager,
                                               APP_NETWORK_STATE_ENTRY_HANDLE,
                                               &m_nw_state,
                                               &length);
    if (status != NRF_SUCCESS)
    {
        memset(&m_nw_state, 0x00, sizeof(m_nw_state));
    }

    return (status == NRF_SUCCESS);
}

static uint32_t store_app_data(void)
{
    fm_entry_t * p_entry = flash_manager_entry_alloc(&m_flash_manager, APP_NETWORK_STATE_ENTRY_HANDLE, sizeof(m_nw_state));
    static fm_mem_listener_t flash_add_mem_available_struct = {
        .callback = flash_manager_mem_available,
        .p_args = store_app_data
    };

    if (p_entry == NULL)
    {
        flash_manager_mem_listener_register(&flash_add_mem_available_struct);
    }
    else
    {
        network_stats_data_stored_t * p_nw_state = (network_stats_data_stored_t *) p_entry->data;
        memcpy(p_nw_state, &m_nw_state, sizeof(m_nw_state));
        flash_manager_entry_commit(p_entry);
    }

    return NRF_SUCCESS;
}

static void clear_app_data(void)
{
    memset(&m_nw_state, 0x00, sizeof(m_nw_state));

    if (flash_manager_remove(&m_flash_manager) != NRF_SUCCESS)
    {
        /* Register the listener and wait for some memory to be freed up before we retry. */
        static fm_mem_listener_t mem_listener = {.callback = flash_manager_mem_available,
                                                 .p_args = clear_app_data};
        flash_manager_mem_listener_register(&mem_listener);
    }
}

#else

static void clear_app_data(void)
{
    return;
}

bool load_app_data(void)
{
    return false;
}
static uint32_t store_app_data(void)
{
    return NRF_SUCCESS;
}

#endif


static void app_data_store_cb(void)
{
    ERROR_CHECK(store_app_data());
}

static const char * server_uri_index_select(const char * p_client_uri)
{
    if (strcmp(p_client_uri, EX_URI_LS_CLIENT) == 0 || strcmp(p_client_uri, EX_URI_ENOCEAN) == 0)
    {
        return EX_URI_LS_SERVER;
    }
    else if (strcmp(p_client_uri, EX_URI_DM_CLIENT) == 0)
    {
        return EX_URI_DM_SERVER;
    }

    __LOG(LOG_SRC_APP, LOG_LEVEL_ASSERT, "Invalid client URI index.\n");
    NRF_MESH_ASSERT(false);

    return NULL;
}

/*****************************************************************************/
/**** Configuration process related callbacks ****/

static void app_config_successful_cb(void)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Configuration of device %u successful address: 0x%04X\n", m_nw_state.configured_devices, m_nw_state.last_device_address);
        index_online++;
        tqt_online_node_state[index_online].addr = m_nw_state.last_device_address;
        tqt_online_node_state[index_online].node_model_addr = m_nw_state.last_device_address + 3;
        tqt_online_node_state[index_online].active_state = ONLINE;
        number_of_node_current = m_nw_state.configured_devices;
    if(m_nw_state.configured_devices == NUMBER_OF_DEFAULT_DEVICES_IN_MESH) {                     //tqt test provisione all node
        tqt_send_string("FINISH\n");
        nrf_delay_ms(100);
        tqt_state_network_update = tqt_update_add_uart_state();
        //tqt_reset_nodes_state();
        tqt_run_config_server_model();
        tqt_run_config_client_model();
        tqt_run_config_mesh_network_stt();
        __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "----- PROVISIONER SUB  -----\n");
    }
    hal_led_pin_set(APP_CONFIGURATION_LED, 0);

    m_nw_state.configured_devices++;
    access_flash_config_store();
    ERROR_CHECK(store_app_data());

    if (m_nw_state.configured_devices < (SERVER_NODE_COUNT + CLIENT_NODE_COUNT))
    {
        static const char * uri_list[1];
        uri_list[0] = server_uri_index_select(m_nw_state.p_client_uri);
        prov_helper_provision_next_device(PROVISIONER_RETRY_COUNT, m_nw_state.next_device_address,
                                           uri_list, ARRAY_SIZE(uri_list));
        prov_helper_scan_start();

        hal_led_pin_set(APP_PROVISIONING_LED, 1);
    }
    else
    {
        __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "All servers provisioned\n");

        hal_led_blink_ms(LEDS_MASK, LED_BLINK_INTERVAL_MS, LED_BLINK_CNT_PROV);
    }
}

static void app_config_failed_cb(void)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Configuration of device %u failed. Press Button 1 to retry.\n", m_nw_state.configured_devices);
    m_node_prov_setup_started = false;
    hal_led_pin_set(APP_CONFIGURATION_LED, 0);
}

static void app_prov_success_cb(void)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Provisioning successful\n");

    hal_led_pin_set(APP_PROVISIONING_LED, 0);
    hal_led_pin_set(APP_CONFIGURATION_LED, 1);

    ERROR_CHECK(store_app_data());
}

static void app_prov_failed_cb(void)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Provisioning failed. Press Button 1 to retry.\n");
    m_node_prov_setup_started = false;

    hal_led_pin_set(APP_PROVISIONING_LED, 0);
}


/*****************************************************************************/
/**** Model related callbacks ****/
static void app_health_event_cb(const health_client_t * p_client, const health_client_evt_t * p_event)
{
    switch (p_event->type)
    {
        case HEALTH_CLIENT_EVT_TYPE_CURRENT_STATUS_RECEIVED:
//                __LOG(LOG_SRC_APP,
//                  LOG_LEVEL_INFO,
//                  "Node 0x%04x alive with %u active fault(s), RSSI: %d\n",
//                  p_event->p_meta_data->src.value,
//                  p_event->data.fault_status.fault_array_length,
//                  ((p_event->p_meta_data->p_core_metadata->source == NRF_MESH_RX_SOURCE_SCANNER)
//                       ? p_event->p_meta_data->p_core_metadata->params.scanner.rssi
//                       : 0));
            break;
        default:
            break;
    }
}

static void app_config_server_event_cb(const config_server_evt_t * p_evt)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "config_server Event %d.\n", p_evt->type);

    if (p_evt->type == CONFIG_SERVER_EVT_NODE_RESET)
    {
        /* This should never return */
        hal_device_reset(0);
    }
}

static void app_config_client_event_cb(config_client_event_type_t event_type, const config_client_event_t * p_event, uint16_t length)
{
    /* USER_NOTE: Do additional processing of config client events here if required */
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Config client event\n");

    /* Pass events to the node setup helper module for further processing */
    node_setup_config_client_event_process(event_type, p_event, length);
}


/** Check if all devices have been provisioned. If not, provision remaining devices.
 *  Check if all devices have been configured. If not, start configuring them.
 */
static void check_network_state(void)
{
    if (!m_node_prov_setup_started)
    {
        /* If previously provisioned device is not configured, start node setup procedure. */
        if (m_nw_state.configured_devices < m_nw_state.provisioned_devices)
        {
            /* Execute configuration */
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Waiting for provisioned node to be configured ...\n");
                 node_setup_start(m_nw_state.last_device_address, PROVISIONER_RETRY_COUNT,
                            m_nw_state.appkey, APPKEY_INDEX, m_nw_state.p_client_uri);

            hal_led_pin_set(APP_CONFIGURATION_LED, 1);
        }
        else if (m_nw_state.provisioned_devices == 0)
        {
            /* Start provisioning - First provision the client with known URI hash */
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Waiting for Client node to be provisioned ...\n");

            static const char * uri_list[] = {EX_URI_LS_CLIENT, EX_URI_ENOCEAN, EX_URI_DM_CLIENT};
            prov_helper_provision_next_device(PROVISIONER_RETRY_COUNT, m_nw_state.next_device_address,
                                              uri_list, ARRAY_SIZE(uri_list));
            prov_helper_scan_start();

            hal_led_pin_set(APP_PROVISIONING_LED, 1);
        }
        else if (m_nw_state.provisioned_devices < (SERVER_NODE_COUNT + CLIENT_NODE_COUNT))
        {
            /* Start provisioning - rest of the devices, depending on the client that was provisioned. */
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Waiting for Server node to be provisioned ...\n");

            static const char * uri_list[1];
            uri_list[0] = server_uri_index_select(m_nw_state.p_client_uri);
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Server URI index: %d\n",  uri_list[0]);
            prov_helper_provision_next_device(PROVISIONER_RETRY_COUNT, m_nw_state.next_device_address,
                                               uri_list, ARRAY_SIZE(uri_list));
            prov_helper_scan_start();

            hal_led_pin_set(APP_PROVISIONING_LED, 1);
        }
        else
        {
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "All servers provisioned\n");
            return;
        }

        m_node_prov_setup_started = true;
    }
    else
    {
         __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Waiting for previous procedure to finish ...\n");
    }
}

static void app_mesh_core_event_cb (const nrf_mesh_evt_t * p_evt)
{
    /* USER_NOTE: User can insert mesh core event proceesing here */
    switch(p_evt->type)
    {
        /* Start user application specific functions only when flash is stable */
        case NRF_MESH_EVT_FLASH_STABLE:
            __LOG(LOG_SRC_APP, LOG_LEVEL_DBG1, "Mesh evt: FLASH_STABLE \n");
#if (PERSISTENT_STORAGE)
            {
                static bool s_app_started;
                if (!s_app_started)
                {
                    /* Flash operation initiated during initialization has been completed */
                    app_start();
                    s_app_started = true;
                }
            }
#endif
            break;

        default:
            break;
    }
}

/* Binds the local models correctly with the desired keys */
void app_default_models_bind_setup(void)
{
    /* Bind health client to App key, and configure publication key */
    ERROR_CHECK(access_model_application_bind(m_dev_handles.m_health_client_instance.model_handle, m_dev_handles.m_appkey_handle));
    ERROR_CHECK(access_model_publish_application_set(m_dev_handles.m_health_client_instance.model_handle, m_dev_handles.m_appkey_handle));

    /* Bind self-config server to the self device key */
    ERROR_CHECK(config_server_bind(m_dev_handles.m_self_devkey_handle));

    ERROR_CHECK(access_model_application_bind(server_in_prov_node.server.model_handle,m_dev_handles.m_appkey_handle));
    ERROR_CHECK(access_model_publish_application_set(server_in_prov_node.server.model_handle,m_dev_handles.m_appkey_handle));

    ERROR_CHECK(access_model_application_bind(tqt_pro_m_client.model_handle,m_dev_handles.m_appkey_handle));
    ERROR_CHECK(access_model_publish_application_set(tqt_pro_m_client.model_handle,m_dev_handles.m_appkey_handle));

    ERROR_CHECK(access_model_application_bind(tqt_mesh_network_status.model_handle,m_dev_handles.m_appkey_handle));
    ERROR_CHECK(access_model_publish_application_set(tqt_mesh_network_status.model_handle,m_dev_handles.m_appkey_handle));
}


static bool app_flash_config_load(void)
{
    bool app_load = false;
#if PERSISTENT_STORAGE
    app_flash_manager_add();
    app_load = load_app_data();
#endif
    if (!app_load)
    {
        m_nw_state.provisioned_devices = 0;
        m_nw_state.configured_devices = 0;
        m_nw_state.next_device_address = UNPROV_START_ADDRESS;
        ERROR_CHECK(store_app_data());
    }
    else
    {
        __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Restored: App data\n");
    }
    return app_load;
}
void models_init_cb(void)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Initializing and adding models\n");
    m_dev_handles.m_netkey_handle = DSM_HANDLE_INVALID;
    m_dev_handles.m_appkey_handle = DSM_HANDLE_INVALID;
    m_dev_handles.m_self_devkey_handle = DSM_HANDLE_INVALID;

    /* This app requires following models :
     * config client : To be able to configure other devices
     * health client : To be able to interact with other health servers */
    ERROR_CHECK(config_client_init(app_config_client_event_cb));
    ERROR_CHECK(health_client_init(&m_dev_handles.m_health_client_instance, 0, app_health_event_cb));
    //tqt edit add server model
    ERROR_CHECK(app_onoff_init(&server_in_prov_node, TQT_SERVER_MODEL_ELEMENT));
    //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "App OnOff Model Handle: %d\n", server_in_prov_node.server.model_handle);
    //tqt edit add server model
    //tqt edit add client model
    tqt_pro_m_client.settings.p_callbacks = &client_cbs;
    tqt_pro_m_client.settings.timeout = 0;
    tqt_pro_m_client.settings.force_segmented = TQT_APP_CONFIG_FORCE_SEGMENTATION;
    tqt_pro_m_client.settings.transmic_size = TQT_APP_CONFIG_MIC_SIZE;
    ERROR_CHECK(generic_onoff_client_init(&tqt_pro_m_client, TQT_CLIENT_MODEL_ELEMENT));
    //tqt edit add client model

    //tqt edit add client model send mesh network status start
    tqt_mesh_network_status.settings.p_callbacks = &tqt_mesh_network_status_client_cbs;
    tqt_mesh_network_status.settings.timeout = 0;
    tqt_mesh_network_status.settings.force_segmented = TQT_APP_CONFIG_FORCE_SEGMENTATION;
    tqt_mesh_network_status.settings.transmic_size = TQT_APP_CONFIG_MIC_SIZE;
    ERROR_CHECK(generic_onoff_client_init(&tqt_mesh_network_status, TQT_CLIENT_MODEL_MESH_STATUS_INDEX));
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Add mesh network status send request model\n");
    //tqt edit add client model send mesh network status end
}

static void mesh_init(void)
{
    bool device_provisioned;
    mesh_stack_init_params_t init_params =
    {
        .core.irq_priority       = NRF_MESH_IRQ_PRIORITY_THREAD,
        .core.lfclksrc           = DEV_BOARD_LF_CLK_CFG,
        .models.models_init_cb   = models_init_cb,
        .models.config_server_cb = app_config_server_event_cb
    };
    ERROR_CHECK(mesh_stack_init(&init_params, &device_provisioned));

    nrf_mesh_evt_handler_add(&m_mesh_core_event_handler);

    /* Load application configuration, if available */
    m_dev_handles.flash_load_success = app_flash_config_load();

    /* Initialize the provisioner */
    mesh_provisioner_init_params_t m_prov_helper_init_info =
    {
        .p_dev_data = &m_dev_handles,
        .p_nw_data = &m_nw_state,
        .netkey_idx = NETKEY_INDEX,
        .attention_duration_s = ATTENTION_DURATION_S,
        .p_data_store_cb  = app_data_store_cb,
        .p_prov_success_cb = app_prov_success_cb,
        .p_prov_failed_cb = app_prov_failed_cb
    };
    prov_helper_init(&m_prov_helper_init_info);

    if (!device_provisioned)
    {
        __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Setup defaults: Adding keys, addresses, and bindings \n");
        prov_helper_provision_self();
        app_default_models_bind_setup();
        access_flash_config_store();
        app_data_store_cb();
    }
    else
    {
        __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Restored: Handles \n");
        prov_helper_device_handles_load();
    }

    node_setup_cb_set(app_config_successful_cb, app_config_failed_cb);
}

/**
*
*   @brief: Button event handler
*/

void tqt_send_state_to_end_node(bool state) {
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "STATE TO END NODE: %d\n",state);
            uint32_t status = NRF_SUCCESS;
            generic_onoff_set_params_t set_params;
            model_transition_t transition_params;
            static uint8_t tid = 0;
            set_params.on_off = state;
            set_params.tid = tid++;
            transition_params.delay_ms = TQT_APP_CONFIG_ONOFF_DELAY_MS;
            transition_params.transition_time_ms = TQT_APP_CONFIG_ONOFF_TRANSITION_TIME_MS;
            
            (void)access_model_reliable_cancel(tqt_pro_m_client.model_handle);
            status = generic_onoff_client_set(&tqt_pro_m_client, &set_params, &transition_params); 
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

void tqt_send_mesh_network_status_request(bool state) {
            //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "SEND MESH NETWORK STATUS REQUEST: %d\n",state);
            uint32_t status = NRF_SUCCESS;
            generic_onoff_set_params_t set_params;
            model_transition_t transition_params;
            static uint8_t tid = 0;
            set_params.on_off = state;
            set_params.tid = tid++;
            transition_params.delay_ms = TQT_APP_CONFIG_ONOFF_DELAY_MS;
            transition_params.transition_time_ms = TQT_APP_CONFIG_ONOFF_TRANSITION_TIME_MS;
            
            (void)access_model_reliable_cancel(tqt_mesh_network_status.model_handle);
            status = generic_onoff_client_set(&tqt_mesh_network_status, &set_params, &transition_params); 
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

void in_pin_handler(nrfx_gpiote_pin_t button_number, nrf_gpiote_polarity_t action)
{
    switch (button_number)
    {
        case BUTTON_2:
        {
            nrf_gpio_pin_toggle(LED_1);
            break;

            /* Clear all the states to reset the node. */
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "----- Node reset -----\n");

            clear_app_data();
            mesh_stack_config_clear();

            //hal_led_blink_ms(1 << BSP_LED_3, LED_BLINK_INTERVAL_MS, LED_BLINK_CNT_PROV);
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "----- Press reset button or power cycle the device  -----\n");
            break;
        }
        case BUTTON_3:
        {
        nrf_gpio_pin_toggle(LED_3);
            /* Check if all devices have been provisioned or not */
            check_network_state();
            break;
        }

        /* Initiate node reset */
        case BUTTON_4:
        {   
            break;
        }

        default:
            break;
    }
    nrf_delay_ms(500);
}


/*
*
*brief: Init external GPIOTE button interupt
*/
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
    __LOG_INIT(LOG_SRC_APP | LOG_SRC_ACCESS, LOG_LEVEL_INFO, LOG_CALLBACK_DEFAULT);
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "----- BLE Mesh Light Switch Provisioner Demo -----\n");

    ERROR_CHECK(app_timer_init());
    //hal_leds_init();

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

    /* Mesh Init */
    mesh_init();
}

static void app_start(void)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Starting application ...\n");
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Provisoned Nodes: %d, Configured Nodes: %d Next Address: 0x%04x\n",
          m_nw_state.provisioned_devices, m_nw_state.configured_devices, m_nw_state.next_device_address);
    __LOG_XB(LOG_SRC_APP, LOG_LEVEL_INFO, "Dev key ", m_nw_state.self_devkey, NRF_MESH_KEY_SIZE);
    __LOG_XB(LOG_SRC_APP, LOG_LEVEL_INFO, "Net key ", m_nw_state.netkey, NRF_MESH_KEY_SIZE);
    __LOG_XB(LOG_SRC_APP, LOG_LEVEL_INFO, "App key ", m_nw_state.appkey, NRF_MESH_KEY_SIZE);
      tqt_appkey = m_nw_state.appkey;
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Press Button 3 to start provisioning and configuration process. \n");
}

static void start(void)
{
    //rtt_input_enable(app_rtt_input_handler, RTT_INPUT_POLL_PERIOD_MS);
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "<start> \n");

#if (!PERSISTENT_STORAGE)
    app_start();
#endif

    ERROR_CHECK(nrf_mesh_enable());

    hal_led_mask_set(LEDS_MASK, LED_MASK_STATE_OFF);
    hal_led_blink_ms(LEDS_MASK, LED_BLINK_INTERVAL_MS, LED_BLINK_CNT_START);

}
void tqt_load_online_node_after_reset() {
    index_online = -1;
    for(int i = number_of_node_current; i >= 0; i--) {
        index_online++;
        tqt_online_node_state[index_online].addr = UNPROV_START_ADDRESS + i * 4;        // To get node address (0x0100,0x0104, 0x0108)
        tqt_online_node_state[index_online].node_model_addr = tqt_online_node_state[index_online].addr + 3; // get model address sub to cloud mesh stt request
        tqt_online_node_state[index_online].active_state = OFFLINE;
        __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "After OFFLINE Node: 0x%04x ONLINE\n",tqt_online_node_state[index_online].addr);
    }
    tqt_reset_nodes_state();
}

int main(void)
{
    initialize();
    start();
    APP_SCHED_INIT(APP_SCHED_EVENT_SIZE, APP_SCHED_QUEUE_SIZE);
    tqt_init_uart();
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "INDEX ONLINE: %d\n",index_online);
    for (;;)
    {
      app_sched_execute();
      bool done = nrf_mesh_process();
      if (done){
        if(door_state_to_server != last_door_state_to_server) {
          if(door_state_to_server == true) {
            tqt_send_string("ON\n");
          } else {
            tqt_send_string("OFF\n");
          }
          last_door_state_to_server = door_state_to_server;
          //save_door_status = door_state_to_server;
        } else if(CLOUD_REQUEST_ON == true) {
            tqt_send_state_to_end_node(true);
            //save_door_status = true;
            CLOUD_REQUEST_ON = false;
        } else if(CLOUD_REQUEST_OFF == true) {
            tqt_send_state_to_end_node(false);
            //save_door_status = false;
            CLOUD_REQUEST_OFF = false;
        } else if(INIT_CNT_REQUEST_UPDATE_MEST_STT == true) {
            counter_request_from_cloud = 0;
            INIT_CNT_REQUEST_UPDATE_MEST_STT = false;
        } else if(CLOUD_REQUEST_LOCK_STT == true) {
            number_of_node_current = m_nw_state.provisioned_devices - 1;
            counter_request_from_cloud = 0;
//            if(save_door_status == true) {
//              tqt_send_string("ON\n");
//            } else {
//              tqt_send_string("OFF\n");
//            }
            nrf_delay_ms(100);
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "NUMBER_OF_DEVICES_IN_MESH: %d number_of_node_current: %d\n",NUMBER_OF_DEFAULT_DEVICES_IN_MESH, number_of_node_current);
            if(number_of_node_current >= NUMBER_OF_DEFAULT_DEVICES_IN_MESH) {
              tqt_send_string("FINISH\n");
            }
            CLOUD_REQUEST_LOCK_STT = false;
        } else if (CLOUD_REQUEST_NETWORK_STT == true) {
            CLOUD_REQUEST_NETWORK_STT = false;
            counter_request_from_cloud++;
            tqt_send_mesh_network_status_request(true);
        } else if (FINISHED_GET_NODE_ALIVE_SIGNAL == true) {
            number_of_node_current = m_nw_state.provisioned_devices - 1;
            tqt_load_online_node_after_reset();
            if(counter_node_signal_index == 0) {
                char all_offline[10] = {'\0'};
                for(int i = 0; i <= number_of_node_current; i++) {
                    strcat(all_offline,"2");
                }
                strcat(all_offline,"\n");
                  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "No Node online\n");
                  tqt_send_string(all_offline);
            } else {
                for(int i = 0 ; i < counter_node_signal_index; i++) {
                    tqt_update_node_online_again(addr_of_node_model_alive[i]);
                }
                //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "UART Ready\n");
                tqt_not_use = tqt_update_add_uart_state();
                __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "NetWork Send: %s\n",tqt_network_state_str);
                tqt_send_string(tqt_network_state_str);
            }
            counter_node_signal_index = 0;
            FINISHED_GET_NODE_ALIVE_SIGNAL = false;
            memset(addr_of_node_model_alive,0,sizeof(addr_of_node_model_alive));
        } else if (tqt_state_network_update == true) {
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Send STT: %s\n",tqt_network_state_str);
            tqt_send_string(tqt_network_state_str);
            tqt_state_network_update = false;
        }  else {
            continue;
        }
        (void)sd_app_evt_wait();
     }
    }
}
