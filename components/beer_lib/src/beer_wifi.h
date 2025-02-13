#include <WiFi.h>
#include <esp_now.h>
#include "hal/ledc_types.h"

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

uint8_t get_peer_start();
uint8_t get_peer_found();
uint8_t get_disp_remote();
void set_disp_remote(uint8_t val);

void connect();
void esp_now_routine();
void data_recv(const uint8_t * mac, const uint8_t *incomingData, int len);
void send_weight();
void send_greeting();
void disconnect();

// OTA

esp_err_t upload_handler(httpd_req_t *req);
esp_err_t index_handler(httpd_req_t *req);
void start_webserver();
void init_wifi();
