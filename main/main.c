#include <stdio.h>
#include "esp_err.h"


#include "esp_log.h"
#include "esp_log_level.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/gpio.h>
#include "driver/gpio.h"
#include "string.h"
#include "inc/bluetooth_user.h"
#define LED_PIN 2
//uint8_t mac_cmp[6] = {0xc4, 0xde, 0xe2, 0x5c, 0x74, 0x46}; //another esp
//uint8_t mac_cmp[6] = {0x38, 0x9c, 0xb2, 0xe5, 0x5a, 0xda}; //iphone
uint8_t mac_cmp[6] = {0xf0, 0x20, 0xff, 0xcc, 0xbc, 0x7a}; //pc
uint8_t mac_get[6] = {
    0,
};
char *TAG1 = "ESP32";
uint8_t compare_mac = 0;
bool ledon = 0;

esp_ble_scan_params_t ble_scan_params = {
    .scan_type = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_duplicate = BLE_SCAN_DUPLICATE_ENABLE,
    .scan_interval = 0x50,
    .scan_window = 0x30,
};

static void get_device_name(esp_ble_gap_cb_param_t *param, char *name, int name_len)
{
    uint8_t *adv_name = NULL;
    uint8_t adv_name_len = 0;

    // Extract the device name from the advertisement data
    adv_name = esp_ble_resolve_adv_data(param->scan_rst.ble_adv, ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);

    if (adv_name != NULL && adv_name_len > 0)
    {
        // Copy the device name to the output buffer
        snprintf(name, name_len, "%.*s", adv_name_len, adv_name);
    }
    else
    {
        // If no name is found, show "Unknown device"
        snprintf(name, name_len, "Unknown device");
    }
}

void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
        if (param->scan_param_cmpl.status == ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGI(TAG1, "Scan parameters set, starting scan...");
            esp_ble_gap_start_scanning(86400); // Scan for 10 seconds
        }
        else
        {
            ESP_LOGE(TAG1, "Failed to set scan parameters");
        }
        break;

    case ESP_GAP_BLE_SCAN_RESULT_EVT:
        if (param->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT)
        {
            // Get the device name
            char device_name[64];
            get_device_name(param, device_name, sizeof(device_name));

            // Log the device name and RSSI
            char *name_device_cmp = "Unknown device";
            bool cmp_names_devs;
            cmp_names_devs = strncmp(device_name, name_device_cmp, 14);
           // if (cmp_names_devs)
            {
                ESP_LOGI(TAG1, "Device found: Name: %s, RSSI %d", device_name, param->scan_rst.rssi);
                printf("MAC Address: ");
                for (uint8_t addr = 0; addr < ESP_BD_ADDR_LEN - 1; addr++)
                {
                    // ESP_LOGI(TAG, "%x :", param->scan_rst.bda[0]);
                    printf("%x:", param->scan_rst.bda[addr]);
                    mac_get[addr] = param->scan_rst.bda[addr];
                }
                printf("%x", param->scan_rst.bda[ESP_BD_ADDR_LEN - 1]);
                mac_get[ESP_BD_ADDR_LEN - 1] = param->scan_rst.bda[ESP_BD_ADDR_LEN - 1];
                printf("\n\n");
                printf("GET MAC = ");

                for (uint8_t macnum = 0; macnum < 6; macnum++)
                {
                    printf("%x", mac_get[macnum]);
                }
                printf("\n\n");
                compare_mac = 0;
                for (uint8_t macnum = 0; macnum < 6; macnum++)
                {
                    if (mac_get[macnum] == mac_cmp[macnum])
                        compare_mac++;
                }
                // compare_mac = 0;
                printf("compare MAC NUMBER = %d\n", compare_mac);
            }
            if ((compare_mac == 6) && (param->scan_rst.rssi > -28))
            {
                ledon = 1;
            }

            //   else
            //   {
            // ledon = 0;
            //   }
        }
        break;

    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        ESP_LOGI(TAG1, "Scan complete");
        break;

    default:
        break;
    }
}

void app_main(void)
{
    // Initialize NVS
    init_nvs();

    // Initialize BLE
    init_ble();
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
    esp_rom_gpio_pad_select_gpio(LED_PIN);
    // Register GAP callback
    esp_ble_gap_register_callback(gap_event_handler);

    // Set scan parameters
    esp_err_t ret = esp_ble_gap_set_scan_params(&ble_scan_params);
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG1, "BLE scan parameters set successfully");
    }
    else
    {
        ESP_LOGE(TAG1, "Failed to set scan params: %s", esp_err_to_name(ret));
    }

    int ON = 0;
    while (true)
    {
        // ON = !ON;
        //    printf("\nled = %d\n", ledon);
        if (ledon)
        {
            gpio_set_level(LED_PIN, 1);
            vTaskDelay(10000 / portTICK_PERIOD_MS);
             gpio_set_level(LED_PIN, 0);
             vTaskDelay(100 / portTICK_PERIOD_MS);
            ledon = 0;
        }
        else
        {
            gpio_set_level(LED_PIN, 0);
              vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }
}