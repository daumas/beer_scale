#include "beer_wifi.h"
#include "beer_strain_gauge.h"
#include "beer_display.h"

// esp now
uint8_t peer_last = 0;  //ms last time

uint8_t greeting[]  = "Bier mit mir?!";
uint8_t intro[]     = "BS1.0";
uint8_t broadcast[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

// GETTER
uint32_t peer_start = 0; //ms start time of peering 
uint8_t peer_found = 0; //   we found a second scale
uint8_t disp_remote = 0;//   display remote weight

uint8_t get_peer_start(){return peer_start;}
uint8_t get_peer_found(){return peer_found;}
uint8_t get_disp_remote(){return disp_remote;}
void set_disp_remote(uint8_t val){disp_remote = val;}

void connect() {
  if(peer_start == 0){
    peer_start = millis();
    peer_found = 0;
    disp_remote = 2;
  }

  WiFi.mode(WIFI_STA);
  //WiFi.disconnect();
  WiFi.STA.begin();

  esp_now_init();
  esp_now_register_recv_cb(esp_now_recv_cb_t(data_recv));

  esp_now_peer_info_t peer = {};
  memset(peer.peer_addr, 0xff, 6);
  peer.channel = 0;  
  peer.encrypt = false;
  esp_now_add_peer(&peer);
}

void esp_now_routine(){
  // esp now
  if(((millis() - peer_start) < 10000) || peer_found)
    send_greeting();
  else if(peer_start > 0)
    disconnect();
  if(peer_found && !get_disp_tar())
    send_weight();
}

// esp-net
void data_recv(const uint8_t * mac, const uint8_t *incomingData, int len){
  float avg_rec = get_avg_rec();
  float stable_rec = get_stable_rec();

  if(len == sizeof(intro)+sizeof(get_avg())+sizeof(stable_rec)){
    if(!strncmp((char*)intro, (char*)incomingData, sizeof(intro))){
      memcpy(&avg_rec, incomingData+sizeof(intro), sizeof(avg_rec));
      memcpy(&stable_rec, incomingData+sizeof(intro)+sizeof(avg_rec), sizeof(stable_rec));
    }
    return;
  }
  if(len == sizeof(greeting))
    if(!strncmp((char*)greeting, (char*)incomingData, sizeof(greeting))){
      peer_found = 1;
      #ifdef DEBUG
        Serial.println("found peer!!");
      #endif
    }
}

void send_weight(){
  float avg = get_avg();
  float stable = get_stable();

  uint8_t buffer[sizeof(intro) + sizeof(avg) + sizeof(stable)];
  memcpy(buffer, intro, sizeof(intro));
  memcpy(buffer + sizeof(intro), &avg, sizeof(avg));
  memcpy(buffer + sizeof(intro) + sizeof(avg), &stable, sizeof(stable));
  esp_err_t result = esp_now_send(broadcast, buffer, sizeof(buffer));
}


void send_greeting(){
  if(millis() - peer_last < 300)
    return;
  peer_last = millis();
  esp_err_t result = esp_now_send(broadcast, greeting, sizeof(greeting));
  if(millis() / 500 % 2 || peer_found)
    tft_print_all_it("C", 4, TAR_T, 1);
  else
    tft_print_all_it(" ", 4, TAR_T, 1);
}


void disconnect(){
  tft_print_all_it(" ", 4, TAR_T, 1);
  peer_start = 0;
  esp_now_deinit();
  //WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
}

//-----------------------------------OTA---------------------------------------//

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
