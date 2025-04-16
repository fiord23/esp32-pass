#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_gap_bt_api.h"
#include "esp_gap_ble_api.h"
#include "esp_bt_main.h"

#define TAG "BT_MATCH"
#define LED_GPIO GPIO_NUM_2
#define SCAN_DURATION 9
#define SCAN_INTERVAL 10
#define LED_ON_TIME_SECONDS 5

// ==== Список целевых MAC'ов ====
static const uint8_t target_macs[][6] = {
    {0xF0, 0x20, 0xFF, 0xCC, 0xBC, 0x7A},
    {0x38, 0x9c, 0xb2, 0xe5, 0x5a, 0xda}
};
static const size_t num_targets = sizeof(target_macs) / sizeof(target_macs[0]);

static TimerHandle_t led_timer;

// ==== LED ====
void led_init() {
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GPIO, 0);
}

void led_on() {
    gpio_set_level(LED_GPIO, 1);
    xTimerReset(led_timer, 0);
}

void led_off_cb(TimerHandle_t xTimer) {
    gpio_set_level(LED_GPIO, 0);
}

// ==== Сравнение MAC ====
bool is_target_mac(const uint8_t *mac) {
    for (size_t i = 0; i < num_targets; i++) {
        if (memcmp(mac, target_macs[i], 6) == 0) return true;
    }
    return false;
}

// ==== BLE ====
void ble_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    if (event == ESP_GAP_BLE_SCAN_RESULT_EVT && param->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) {
        if (is_target_mac(param->scan_rst.bda)) {
            ESP_LOGI(TAG, "[BLE] Найдено совпадение!");
            led_on();
        }
    }
}

// ==== Classic ====
void bt_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
    if (event == ESP_BT_GAP_DISC_RES_EVT) {
        if (is_target_mac(param->disc_res.bda)) {
            ESP_LOGI(TAG, "[Classic] Найдено совпадение!");
            led_on();
        }
    }
}

// ==== Сканирование ====
void scanner_task(void *arg) {
    while (1) {
        ESP_LOGI(TAG, "🔍 Запуск сканирования BLE и Classic...");
        esp_ble_gap_start_scanning(SCAN_DURATION);
        esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, SCAN_DURATION, 0);

        vTaskDelay(pdMS_TO_TICKS(SCAN_DURATION * 1000));
        ESP_LOGI(TAG, "⏸ Сканирование завершено, пауза %d секунд", SCAN_INTERVAL);
        vTaskDelay(pdMS_TO_TICKS((SCAN_INTERVAL - SCAN_DURATION) * 1000));
    }
}

void app_main() {
    ESP_ERROR_CHECK(nvs_flash_init());
    led_init();

    led_timer = xTimerCreate("led_timer", pdMS_TO_TICKS(LED_ON_TIME_SECONDS * 1000), pdFALSE, NULL, led_off_cb);

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BTDM));
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    ESP_ERROR_CHECK(esp_bt_gap_register_callback(bt_cb));
    ESP_ERROR_CHECK(esp_ble_gap_register_callback(ble_cb));

    esp_ble_scan_params_t scan_params = {
        .scan_type = BLE_SCAN_TYPE_PASSIVE,
        .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
        .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
        .scan_interval = 0x50,
        .scan_window = 0x30,
        .scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE
    };
    ESP_ERROR_CHECK(esp_ble_gap_set_scan_params(&scan_params));

    xTaskCreate(scanner_task, "scanner_task", 4096, NULL, 5, NULL);
}