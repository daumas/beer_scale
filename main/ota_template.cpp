#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_http_server.h"
#include "esp_ota_ops.h"
#include "nvs_flash.h"

#define WIFI_SSID "ESP32_OTA"
#define WIFI_PASS "beer"
#define MAX_FILE_SIZE (1024 * 1024) // 1MB max file size

static const char *TAG = "OTA";
static esp_ota_handle_t ota_handle = 0;
static const esp_partition_t *ota_partition = NULL;

esp_err_t upload_handler(httpd_req_t *req) {
    char buf[1024];
    int received;
    
    ESP_LOGI(TAG, "OTA update started...");
    ota_partition = esp_ota_get_next_update_partition(NULL);
    esp_ota_begin(ota_partition, OTA_SIZE_UNKNOWN, &ota_handle);

    while ((received = httpd_req_recv(req, buf, sizeof(buf))) > 0) {
        esp_ota_write(ota_handle, buf, received);
    }

    esp_ota_end(ota_handle);
    esp_ota_set_boot_partition(ota_partition);
    
    httpd_resp_sendstr(req, "Update complete! Rebooting...");
    ESP_LOGI(TAG, "OTA update finished! Rebooting...");
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    esp_restart();
    
    return ESP_OK;
}

esp_err_t index_handler(httpd_req_t *req) {
    const char *html_page =
        "<html><body><h2>ESP32 OTA Update</h2>"
        "<form method='POST' action='/update' enctype='multipart/form-data'>"
        "<input type='file' name='update'><br>"
        "<input type='submit' value='Upload & Update'></form></body></html>";
    
    httpd_resp_send(req, html_page, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

void start_webserver() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;
    httpd_start(&server, &config);

    httpd_uri_t index_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = index_handler
    };
    httpd_uri_t update_uri = {
        .uri = "/update",
        .method = HTTP_POST,
        .handler = upload_handler
    };

    httpd_register_uri_handler(server, &index_uri);
    httpd_register_uri_handler(server, &update_uri);
}

void init_wifi() {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .ssid_len = strlen(WIFI_SSID),
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    esp_wifi_start();
}

void app_main() {
    ESP_LOGI(TAG, "Starting OTA Web Server...");
    nvs_flash_init();
    init_wifi();
    start_webserver();
}
