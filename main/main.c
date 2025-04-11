#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_gap_ble_api.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BT_BLE_SCANNER";

// ======== Classic BT GAP Callback =========
void bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
    if (event == ESP_BT_GAP_DISC_RES_EVT) {
        char bda_str[18];
        snprintf(bda_str, sizeof(bda_str),
                 "%02x:%02x:%02x:%02x:%02x:%02x",
                 param->disc_res.bda[0], param->disc_res.bda[1], param->disc_res.bda[2],
                 param->disc_res.bda[3], param->disc_res.bda[4], param->disc_res.bda[5]);

        int rssi = 0;
        char name[ESP_BT_GAP_MAX_BDNAME_LEN + 1] = {0};


        for (int i = 0; i < param->disc_res.num_prop; i++) {
            esp_bt_gap_dev_prop_t *p = &param->disc_res.prop[i];
           // printf("ptype = %d", p->type);
            if (p->type == ESP_BT_GAP_DEV_PROP_RSSI) 
        {
                rssi = *(int8_t *)(p->val);
                
            } //else if (p->type == ESP_BT_GAP_DEV_PROP_BDNAME) 
            {
                size_t len = p->len > ESP_BT_GAP_MAX_BDNAME_LEN ? ESP_BT_GAP_MAX_BDNAME_LEN : p->len;
                memcpy(name, (char *)(p->val), p->len);
                name[p->len] = '\0';
             //   printf("ptype = %d", p->type);
               // printf("legth = %d", p->len);
            }
        }
        if (strlen(name) > 0)
        {
            char name_tmp[ESP_BT_GAP_MAX_BDNAME_LEN + 1] = {0};
            for (uint8_t i = 0; i < strlen(name); i ++ )
            {
                if ((name[i] >= 0x20) && (name[i] <= 0x7F))
                    name_tmp[i] = name[i];
                else
                    name_tmp[i] = 8;
                
            }
            ESP_LOGI(TAG, "Classic BT Device: MAC:  %s, RSSI: %d, Name:  %s",bda_str, rssi, strlen(name) > 0 ? name_tmp : "Unknown");
        
        }
        
     //   ESP_LOGI(TAG, "  MAC:  %s", bda_str);
     //   ESP_LOGI(TAG, "  RSSI: %d", rssi);
    //    ESP_LOGI(TAG, "  Name: %s", strlen(name) > 0 ? name : "Unknown");
    } else if (event == ESP_BT_GAP_DISC_STATE_CHANGED_EVT) {
        if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED) {
         //   ESP_LOGI(TAG, "Discovery finished, restarting in 5s...");
     //   vTaskDelay(pdMS_TO_TICKS(5000)); // 5 sec pause
        esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);
        }
    }
}

// ======== BLE ADV Data Parser =========
static void parse_ble_name(const uint8_t *adv_data, uint8_t adv_data_len, char *name_buf, size_t buf_len) {
    uint8_t idx = 0;

    while (idx < adv_data_len) {
        uint8_t len = adv_data[idx++];
        if (len == 0 || idx + len > adv_data_len) break;

        uint8_t type = adv_data[idx];
        if (type == ESP_BLE_AD_TYPE_NAME_CMPL || type == ESP_BLE_AD_TYPE_NAME_SHORT) {
            size_t name_len = len - 1;
            if (name_len >= buf_len) name_len = buf_len - 1;
            memcpy(name_buf, &adv_data[idx + 1], name_len);
            name_buf[name_len] = '\0';
            return;
        }
        idx += len;
    }

    strcpy(name_buf, "Unknown");
}

// ======== BLE GAP Callback =========
void ble_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    if (event == ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT) {
        esp_ble_gap_start_scanning(0);
    } else if (event == ESP_GAP_BLE_SCAN_RESULT_EVT) {
        if (param->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) {
            char bda_str[18];
            snprintf(bda_str, sizeof(bda_str),
                     "%02x:%02x:%02x:%02x:%02x:%02x",
                     param->scan_rst.bda[0], param->scan_rst.bda[1], param->scan_rst.bda[2],
                     param->scan_rst.bda[3], param->scan_rst.bda[4], param->scan_rst.bda[5]);

            char name[ESP_BT_GAP_MAX_BDNAME_LEN + 1] = "Unknown";

            // buf advertising + scan response
            uint8_t adv_total[62] = {0}; // 31 adv + 31 scan_rsp макс
            int total_len = 0;

            if (param->scan_rst.adv_data_len > 0) {
                memcpy(adv_total, param->scan_rst.ble_adv, param->scan_rst.adv_data_len);
                total_len += param->scan_rst.adv_data_len;
            }

            if (param->scan_rst.scan_rsp_len > 0 && (total_len + param->scan_rst.scan_rsp_len) <= sizeof(adv_total)) {
                memcpy(adv_total + total_len, param->scan_rst.ble_adv + param->scan_rst.adv_data_len,
                       param->scan_rst.scan_rsp_len);
                total_len += param->scan_rst.scan_rsp_len;
            }

            //check name
            parse_ble_name(adv_total, total_len, name, sizeof(name));
            if (strncmp(name, "Unknown", 7))
            {
                ESP_LOGI(TAG, "BLE Device: MAC:  %s, RSSI: %d, Name: %s",bda_str, param->scan_rst.rssi, name);
            }
        }
    }
}

// ======== app_main() =========
void app_main(void) {
    // init NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    // init Bluetooth
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BTDM));  // BLE + Classic

    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    ESP_ERROR_CHECK(esp_bt_gap_register_callback(bt_gap_cb));
    ESP_ERROR_CHECK(esp_ble_gap_register_callback(ble_gap_cb));

    // BLE params
    esp_ble_scan_params_t ble_scan_params = {
        .scan_type              = BLE_SCAN_TYPE_PASSIVE,
        .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
        .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
        .scan_interval          = 0x50,
        .scan_window            = 0x30,
        .scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE
    };
    ESP_ERROR_CHECK(esp_ble_gap_set_scan_params(&ble_scan_params));

    // Classic Bluetooth — stat scan
    ESP_ERROR_CHECK(esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0));
}