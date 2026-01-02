#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

static const char *TAG = "LOCALIZER";

void app_main(void)
{
    ESP_LOGI(TAG, "Localizer application started");
    
    while (1) {
        ESP_LOGI(TAG, "Hello from Localizer!");
        vTaskDelay(pdMS_TO_SEC(1));
    }
}
