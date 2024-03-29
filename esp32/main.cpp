#include "esp32.h"
#include <time.h>
#include <esp_http_server.h>
#include <lwip/apps/sntp.h>
#include <soc/uart_pins.h>
#include "app/fs.h"
#include "app/httpd.h"
#include "app/https.h"
#include "app/mqtt.h"
#include "app/ota.h"

_LIBCPP_BEGIN_NAMESPACE_STD
  template string operator+<char, char_traits<char>, allocator<char>>(char const*, string const&);
_LIBCPP_END_NAMESPACE_STD

#define TAG __FILE_NAME__

extern const char wifi_format[] __attribute__((weak));
extern const char pass_format[] __attribute__((weak));
const char wifi_format[] = "ESP32_%02X%02X%02X";
const char pass_format[] = "32ESP_%02X%02X%02X";
char thisname[24] = "";
char number[256] = "";
bool debug;
bool wifi_connected;
uint16_t wifi_sta_disconnected RTC_DATA_ATTR;
uint16_t wifi_sta_got_ip RTC_DATA_ATTR;

int uart0_tx IRAM_BSS_ATTR = U0TXD_GPIO_NUM;
int uart0_rx IRAM_BSS_ATTR = U0RXD_GPIO_NUM;
int uart1_tx IRAM_BSS_ATTR = U1TXD_GPIO_NUM;
int uart1_rx IRAM_BSS_ATTR = U1RXD_GPIO_NUM;
esp_netif_t* ap_netif IRAM_BSS_ATTR;
esp_netif_t* sta_netif IRAM_BSS_ATTR;

httpd_handle_t httpd_server IRAM_BSS_ATTR;
extern esp_err_t web_system(httpd_req_t* req);
extern esp_err_t web_ssid(httpd_req_t* req);
extern esp_err_t web_ota(httpd_req_t* req);
extern esp_err_t web_udp(httpd_req_t* req);
extern esp_err_t web_mqtt(httpd_req_t* req);
extern esp_err_t web_ntp(httpd_req_t* req);
extern esp_err_t web_reset(httpd_req_t* req);

extern "C" void init_udp_console(const char* ip);

extern "C" void app_wifi() __attribute__((weak));
extern "C" void app_wifi() {}
static void wifi_handler(void)
{
    // HTTP
    httpd_uri_t web_root_uri = { .uri = "/", .method = HTTP_GET, .handler = web_system };
    httpd_uri_t web_system_uri = { .uri = "/setup", .method = HTTP_GET, .handler = web_system };
    httpd_unregister_uri_handler(httpd_server, "/", HTTP_GET);
    httpd_register_uri_handler(httpd_server, &web_root_uri);
    httpd_register_uri_handler(httpd_server, &web_system_uri);
#ifdef DEMO
    // HTTPS
    https_connect("https://raw.githubusercontent.com/metarutaiga/esp32-Xcode/master/LICENSE.txt", "Connection: close", [](void* arg, char* data, int length)
    {
        for (int i = 0; i < length; ++i)
        {
            putchar(data[i]);
        }
    }, nullptr);
#endif
    // MQTT
    int fd = fs_open("mqtt", "r");
    if (fd >= 0)
    {
        string mqtt = fs_gets(number, 256, fd);
        string port = fs_gets(number, 256, fd);
        mqtt_setup(mqtt.c_str(), strtol(port.c_str(), nullptr, 10));
        fs_close(fd);
    }

    // NTP
    string ntp = "pool.ntp.org";
    string zone = "GMT-8";
    fd = fs_open("ntp", "r");
    if (fd >= 0)
    {
        ntp = fs_gets(number, 256, fd);
        zone = fs_gets(number, 256, fd);
        fs_close(fd);
    }
    free((void*)sntp_getservername(0));
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, strdup(ntp.c_str()));
    sntp_init();
    setenv("TZ", zone.c_str(), 1);
    tzset();

    // OTA
    fd = fs_open("ota", "r");
    if (fd >= 0)
    {
        if (strcmp(fs_gets(number, 256, fd), "YES") == 0)
        {
            ota_init(8685);
            debug = true;
        }
        fs_close(fd);
    }

    // UDP
    fd = fs_open("udp", "r");
    if (fd >= 0)
    {
        if (strchr(fs_gets(number, 256, fd), '.') != 0)
        {
            init_udp_console(number);
            debug = true;
        }
        fs_close(fd);
    }

    app_wifi();
}

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    bool call_wifi_handler = false;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        wifi_connected = false;
        wifi_sta_disconnected++;

        esp_wifi_connect();
        ESP_LOGI(TAG, "retry to connect to the AP");

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        call_wifi_handler = wifi_connected;
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        wifi_connected = true;
        wifi_sta_got_ip++;

        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:%s", esp_ip4addr_ntoa(&event->ip_info.ip, number, 256));

        call_wifi_handler = wifi_connected;
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_AP_STAIPASSIGNED)
    {
        // OTA
        int fd = fs_open("ota", "r");
        if (fd >= 0)
        {
            if (strcmp(fs_gets(number, 256, fd), "YES") == 0)
                ota_init(8685);
            fs_close(fd);
        }
    }

    // WiFi Handler
    if (call_wifi_handler)
    {
        wifi_sta_list_t sta = {};
        esp_wifi_ap_get_sta_list(&sta);
        if (sta.num == 0)
        {
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
            wifi_handler();
        }
    }
}

extern "C" void app_setup() __attribute__((weak));
extern "C" void app_setup() {}
extern "C" void app_main()
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ap_netif = esp_netif_create_default_wifi_ap();
    sta_netif = esp_netif_create_default_wifi_sta();

    // Component
    fs_init();

    // WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    // MAC
    uint8_t macaddr[6] = {};
    esp_wifi_get_mac(WIFI_IF_STA, macaddr);

    // Soft AP
    wifi_config_t config = {};
    sprintf((char*)config.ap.ssid, wifi_format, macaddr[3], macaddr[4], macaddr[5]);
    sprintf((char*)config.ap.password, pass_format, macaddr[3], macaddr[4], macaddr[5]);
    config.ap.ssid_len = strlen((char*)config.ap.ssid);
    config.ap.channel = 1;
    config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    config.ap.ssid_hidden = 0;
    config.ap.max_connection = 4;
    config.ap.beacon_interval = 100;
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &config));

    // SSID
    int fd = fs_open("ssid", "r");
    if (fd >= 0)
    {
        wifi_config_t config = {};
        strcpy((char*)config.sta.ssid, fs_gets(number, 256, fd));
        strcpy((char*)config.sta.password, fs_gets(number, 256, fd));
        config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
        config.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
        fs_close(fd);
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &config));
    }

    ESP_ERROR_CHECK(esp_wifi_start());

    // Start the httpd server
    httpd_config_t httpd_config = HTTPD_DEFAULT_CONFIG();
    httpd_config.stack_size = 12288;
    httpd_config.max_open_sockets = CONFIG_LWIP_MAX_SOCKETS - 3 - 3 - 2; // ota / log / mqtt / preserved
    httpd_config.max_uri_handlers = 32;
    httpd_config.lru_purge_enable = true;
    ESP_LOGI(TAG, "Starting server on port: '%d'", httpd_config.server_port);
    httpd_start(&httpd_server, &httpd_config);

    // Set URI handlers
    ESP_LOGI(TAG, "Registering URI handlers");
    httpd_uri_t web_root_uri = { .uri = "/", .method = HTTP_GET, .handler = web_system };
    httpd_uri_t web_ssid_uri = { .uri = "/ssid", .method = HTTP_GET, .handler = web_ssid };
    httpd_uri_t web_ota_uri = { .uri = "/ota", .method = HTTP_GET, .handler = web_ota };
    httpd_uri_t web_udp_uri = { .uri = "/udp", .method = HTTP_GET, .handler = web_udp };
    httpd_uri_t web_mqtt_uri = { .uri = "/mqtt", .method = HTTP_GET, .handler = web_mqtt };
    httpd_uri_t web_ntp_uri = { .uri = "/ntp", .method = HTTP_GET, .handler = web_ntp };
    httpd_uri_t web_reset_uri = { .uri = "/reset", .method = HTTP_GET, .handler = web_reset };
    httpd_register_uri_handler(httpd_server, &web_root_uri);
    httpd_register_uri_handler(httpd_server, &web_ssid_uri);
    httpd_register_uri_handler(httpd_server, &web_ota_uri);
    httpd_register_uri_handler(httpd_server, &web_udp_uri);
    httpd_register_uri_handler(httpd_server, &web_mqtt_uri);
    httpd_register_uri_handler(httpd_server, &web_ntp_uri);
    httpd_register_uri_handler(httpd_server, &web_reset_uri);

    // Setup
    strcpy(thisname, (char*)config.ap.ssid);
    esp_netif_set_hostname(ap_netif, thisname);
    esp_netif_set_hostname(sta_netif, thisname);
    app_setup();
}
