#ifndef BLUETOOTH_USER_H
#define BLUETOOTH_USER_H

#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_log.h"
#include "esp_log_level.h"
#include "esp_gap_ble_api.h"





void init_nvs(void);
void init_ble(void);
#endif