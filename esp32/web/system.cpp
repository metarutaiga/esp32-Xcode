#include "esp32.h"
#include <sys/time.h>
#include <esp_http_server.h>
#include "app/fs.h"
#include "app/httpd.h"

extern const char web_css[] __attribute__((weak));
const char web_css[] = "";

esp_err_t web_system(httpd_req_t* req)
{
    string html;

    // Head
    wifi_mode_t mode = WIFI_MODE_APSTA;
    esp_wifi_get_mode(&mode);
    html += "<html>";
    html += "<head>";
    html +=     mode == WIFI_MODE_STA ? web_css : "";
    html +=     "<title>";
    html +=         thisname;
    html +=     "</title>";
    html += "</head>";
    html += "<body>";
    html += "<table>";
    if (httpd_resp_send_chunk(req, html.data(), html.length()) != ESP_OK)
        return ESP_FAIL;
    html.clear();

    // SSID
    string ssid;
    int fd = fs_open("ssid", "r");
    if (fd >= 0)
    {
        ssid = fs_gets(number, 128, fd);
        fs_close(fd);
    }
    html += "<form method='get' action='ssid'>";
    html +=     "<tr>";
    html +=         "<td>";
    html +=             "<label>SSID</label>";
    html +=         "</td>";
    html +=         "<td>";
    html +=             "<input name='ssid' length=32 value='" + ssid + "'>";
    html +=         "</td>";
    html +=     "</tr>";
    html +=     "<tr>";
    html +=         "<td>";
    html +=             "<label>PASS</label>";
    html +=         "</td>";
    html +=         "<td>";
    html +=             "<input type='password' name='pass' length=32>";
    html +=         "</td>";
    html +=         "<td>";
    html +=             "<input type='submit'>";
    html +=         "</td>";
    html +=     "</tr>";
    html += "</form>";
    if (httpd_resp_send_chunk(req, html.data(), html.length()) != ESP_OK)
        return ESP_FAIL;
    html.clear();

    // OTA
    string ota;
    fd = fs_open("ota", "r");
    if (fd >= 0)
    {
        ota = fs_gets(number, 128, fd);
        fs_close(fd);
    }
    html += "<form method='get' action='ota'>";
    html +=     "<tr>";
    html +=         "<td>";
    html +=             "<label>OTA</label>";
    html +=         "</td>";
    html +=         "<td>";
    html +=             "<input name='ota' length=32 value='" + ota + "'>";
    html +=         "</td>";
    html +=         "<td>";
    html +=             "<input type='submit'>";
    html +=         "</td>";
    html +=     "</tr>";
    html += "</form>";
    if (httpd_resp_send_chunk(req, html.data(), html.length()) != ESP_OK)
        return ESP_FAIL;
    html.clear();

    // MQTT
    string mqtt;
    string mqttPort;
    fd = fs_open("mqtt", "r");
    if (fd >= 0)
    {
        mqtt = fs_gets(number, 128, fd);
        mqttPort = fs_gets(number, 128, fd);
        fs_close(fd);
    }
    if (mqttPort.empty())
    {
        mqttPort = "1883";
    }
    html += "<form method='get' action='mqtt'>";
    html +=     "<tr>";
    html +=         "<td>";
    html +=             "<label>MQTT</label>";
    html +=         "</td>";
    html +=         "<td>";
    html +=             "<input name='mqtt' length=32 value='" + mqtt + "'>";
    html +=         "</td>";
    html +=     "</tr>";
    html +=     "<tr>";
    html +=         "<td>";
    html +=             "<label>PORT</label>";
    html +=         "</td>";
    html +=         "<td>";
    html +=             "<input name='port' length=32 value='" + mqttPort + "'>";
    html +=         "</td>";
    html +=         "<td>";
    html +=             "<input type='submit'>";
    html +=         "</td>";
    html +=     "</tr>";
    html += "</form>";
    if (httpd_resp_send_chunk(req, html.data(), html.length()) != ESP_OK)
        return ESP_FAIL;
    html.clear();

    // NTP
    string ntp;
    string ntpZone;
    fd = fs_open("ntp", "r");
    if (fd >= 0)
    {
        ntp = fs_gets(number, 128, fd);
        ntpZone = fs_gets(number, 128, fd);
        fs_close(fd);
    }
    if (ntp.empty())
    {
        ntp = "pool.ntp.org";
    }
    if (ntpZone.empty())
    {
        ntpZone = "GMT-8";
    }
    html += "<form method='get' action='ntp'>";
    html +=     "<tr>";
    html +=         "<td>";
    html +=             "<label>NTP</label>";
    html +=         "</td>";
    html +=         "<td>";
    html +=             "<input name='name' length=32 value='" + ntp + "'>";
    html +=         "</td>";
    html +=     "</tr>";
    html +=     "<tr>";
    html +=         "<td>";
    html +=             "<label>ZONE</label>";
    html +=         "</td>";
    html +=         "<td>";
    html +=             "<input name='zone' length=32 value='" + ntpZone + "'>";
    html +=         "</td>";
    html +=         "<td>";
    html +=             "<input type='submit'>";
    html +=         "</td>";
    html +=     "</tr>";
    html += "</form>";
    if (httpd_resp_send_chunk(req, html.data(), html.length()) != ESP_OK)
        return ESP_FAIL;
    html.clear();

    // Time
    time_t now;
    struct tm timeinfo;
    char strftime_buf[64];
    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    html += "</table>";
    html += strftime_buf;

    // Reset
    html += "<form method='get' action='reset'>";
    html +=     "<button type='submit'>Reset</button>";
    html += "</form>";

    // Tail
    html += "</body>";
    html += "</html>";
    if (httpd_resp_send_chunk(req, html.data(), html.length()) != ESP_OK)
        return ESP_FAIL;
    html.clear();

    return httpd_resp_send_chunk(req, NULL, 0);
}

esp_err_t web_ssid(httpd_req_t* req)
{
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);

    size_t len;
    char buffer[(len = httpd_req_get_url_query_len(req) + 1)];
    httpd_req_get_url_query_str(req, buffer, len);

    string text;
    char value[64];
    value[0] = 0; httpd_query_key_value(buffer, "ssid", value, 64); text += url_decode(value); text += '\n';
    value[0] = 0; httpd_query_key_value(buffer, "pass", value, 64); text += url_decode(value); text += '\n';

    int fd = fs_open("ssid", "w");
    if (fd >= 0)
    {
        fs_write(text.data(), text.length(), fd);
        fs_close(fd);
    }

    return ESP_OK;
}

esp_err_t web_ota(httpd_req_t* req)
{
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);

    size_t len;
    char buffer[(len = httpd_req_get_url_query_len(req) + 1)];
    httpd_req_get_url_query_str(req, buffer, len);

    string text;
    char value[64];
    value[0] = 0; httpd_query_key_value(buffer, "ota", value, 64); text += url_decode(value); text += '\n';

    int fd = fs_open("ota", "w");
    if (fd >= 0)
    {
        fs_write(text.data(), text.length(), fd);
        fs_close(fd);
    }

    return ESP_OK;
}

esp_err_t web_mqtt(httpd_req_t* req)
{
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);

    size_t len;
    char buffer[(len = httpd_req_get_url_query_len(req) + 1)];
    httpd_req_get_url_query_str(req, buffer, len);

    string text;
    char value[64];
    value[0] = 0; httpd_query_key_value(buffer, "mqtt", value, 64); text += url_decode(value); text += '\n';
    value[0] = 0; httpd_query_key_value(buffer, "port", value, 64); text += url_decode(value); text += '\n';

    int fd = fs_open("mqtt", "w");
    if (fd >= 0)
    {
        fs_write(text.data(), text.length(), fd);
        fs_close(fd);
    }

    return ESP_OK;
}

esp_err_t web_ntp(httpd_req_t* req)
{
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);

    size_t len;
    char buffer[(len = httpd_req_get_url_query_len(req) + 1)];
    httpd_req_get_url_query_str(req, buffer, len);

    string text;
    char value[64];
    value[0] = 0; httpd_query_key_value(buffer, "name", value, 64); text += url_decode(value); text += '\n';
    value[0] = 0; httpd_query_key_value(buffer, "zone", value, 64); text += url_decode(value); text += '\n';

    int fd = fs_open("ntp", "w");
    if (fd >= 0)
    {
        fs_write(text.data(), text.length(), fd);
        fs_close(fd);
    }

    return ESP_OK;
}

esp_err_t web_reset(httpd_req_t* req)
{
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);

    TimerHandle_t timer = xTimerCreate("Reset Timer", 1000 / portTICK_PERIOD_MS, pdTRUE, (void*)"Reset Timer", [](TimerHandle_t)
    {
        esp_restart();
    });
    xTimerStart(timer, 1000 / portTICK_PERIOD_MS);

    return ESP_OK;
}
