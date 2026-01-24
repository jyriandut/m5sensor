/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

/**
 * @Hardwares: AtomS3R-CAM / AtomS3R-M12
 * @Platform Version: Arduino M5Stack Board Manager v2.1.4
 */

#ifndef _M5_ATOM_S3R_CAM_H_
#define _M5_ATOM_S3R_CAM_H_

#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  21
#define SIOD_GPIO_NUM  12
#define SIOC_GPIO_NUM  9
#define Y9_GPIO_NUM    13
#define Y8_GPIO_NUM    11
#define Y7_GPIO_NUM    17
#define Y6_GPIO_NUM    4
#define Y5_GPIO_NUM    48
#define Y4_GPIO_NUM    46
#define Y3_GPIO_NUM    42
#define Y2_GPIO_NUM    3
#define VSYNC_GPIO_NUM 10
#define HREF_GPIO_NUM  14
#define PCLK_GPIO_NUM  40

#define POWER_GPIO_NUM 18

#endif

#include <WiFi.h>
#include "esp_camera.h"
#include <string.h>

#define USE_ATOMS3R_CAM
// #define USE_ATOMS3R_M12

#define STA_MODE
//#define AP_MODE

const char* ssid     = "ut-public";
const char* password = "";

WiFiServer server(80);
camera_fb_t* fb    = NULL;
uint8_t* out_jpg   = NULL;
size_t out_jpg_len = 0;

static void jpegStream(WiFiClient* client);
static void sendJpegSnapshot(WiFiClient* client);
static void readRequestPath(WiFiClient* client, char* path, size_t path_len);
static void sendNotFound(WiFiClient* client);
static void sendIndex(WiFiClient* client);

static const uint8_t kJpegQuality = 90;
static camera_config_t camera_config = {
    .pin_pwdn     = PWDN_GPIO_NUM,
    .pin_reset    = RESET_GPIO_NUM,
    .pin_xclk     = XCLK_GPIO_NUM,
    .pin_sscb_sda = SIOD_GPIO_NUM,
    .pin_sscb_scl = SIOC_GPIO_NUM,
    .pin_d7       = Y9_GPIO_NUM,
    .pin_d6       = Y8_GPIO_NUM,
    .pin_d5       = Y7_GPIO_NUM,
    .pin_d4       = Y6_GPIO_NUM,
    .pin_d3       = Y5_GPIO_NUM,
    .pin_d2       = Y4_GPIO_NUM,
    .pin_d1       = Y3_GPIO_NUM,
    .pin_d0       = Y2_GPIO_NUM,

    .pin_vsync = VSYNC_GPIO_NUM,
    .pin_href  = HREF_GPIO_NUM,
    .pin_pclk  = PCLK_GPIO_NUM,

    .xclk_freq_hz = 20000000,
    .ledc_timer   = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

#ifdef USE_ATOMS3R_CAM
    .pixel_format = PIXFORMAT_RGB565,
    .frame_size   = FRAMESIZE_UXGA,
#endif

#ifdef USE_ATOMS3R_M12
    .pixel_format = PIXFORMAT_JPEG,
    .frame_size   = FRAMESIZE_UXGA,
#endif

    .jpeg_quality  = kJpegQuality,
    .fb_count      = 1,
    .fb_location   = CAMERA_FB_IN_PSRAM,
    .grab_mode     = CAMERA_GRAB_LATEST,
    .sccb_i2c_port = 0,
};

void setup()
{
  Serial.begin(115200);
  Serial.println("Hello world");
    pinMode(POWER_GPIO_NUM, OUTPUT);
    digitalWrite(POWER_GPIO_NUM, LOW);
    delay(500);
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        Serial.println("Camera Init Fail");
        delay(1000);
        esp_restart();
    } else {
        Serial.println("Camera Init Success");
    }
    delay(100);

#ifdef STA_MODE

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    WiFi.setSleep(false);
    Serial.println("");

    Serial.print("Connecting to ");
    Serial.println(ssid);

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
#endif

#ifdef AP_MODE
    if (!WiFi.softAP(ssid, password)) {
        log_e("Soft AP creation failed.");
        while (1);
    }

    Serial.println("AP SSID:");
    Serial.println(ssid);
    Serial.println("AP PASSWORD:");
    Serial.println(password);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
#endif

    server.begin();
}

void loop()
{
    WiFiClient client = server.available();  // listen for incoming clients
    if (client) {                            // if you get a client,
        char path[64] = "/";
        uint32_t start_ms = millis();
        while (client.connected() && (millis() - start_ms) < 2000) {
            if (client.available()) {
                readRequestPath(&client, path, sizeof(path));
                if (strcmp(path, "/capture") == 0) {
                    sendJpegSnapshot(&client);
                    break;
                } else if (strcmp(path, "/stream") == 0) {
                    jpegStream(&client);
                    return;
                } else if (strcmp(path, "/") == 0) {
                    sendIndex(&client);
                } else {
                    sendNotFound(&client);
                }
                break;
            }
            delay(1);
        }
        client.stop();
        Serial.println("Client Disconnected.");
    }
}

// used to image stream
#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY     = "--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART         = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

static void jpegStream(WiFiClient* client)
{
    Serial.println("Image stream start");
    client->println("HTTP/1.1 200 OK");
    client->printf("Content-Type: %s\r\n", _STREAM_CONTENT_TYPE);
    client->println("Content-Disposition: inline; filename=capture.jpg");
    client->println("Access-Control-Allow-Origin: *");
    client->println();
    static int64_t last_frame = 0;
    if (!last_frame) {
        last_frame = esp_timer_get_time();
    }

    for (;;) {
        fb = esp_camera_fb_get();
        if (fb) {
            bool needs_free = false;
#ifdef USE_ATOMS3R_CAM
            if (!frame2jpg(fb, kJpegQuality, &out_jpg, &out_jpg_len)) {
                out_jpg = NULL;
                out_jpg_len = 0;
            } else {
                needs_free = true;
            }
#endif

#ifdef USE_ATOMS3R_M12
            out_jpg     = fb->buf;
            out_jpg_len = fb->len;
#endif

            if (out_jpg == NULL || out_jpg_len == 0) {
                Serial.println("JPEG encode failed");
            } else {
                Serial.printf("pic size: %d\n", out_jpg_len);
                client->print(_STREAM_BOUNDARY);
                client->printf(_STREAM_PART, out_jpg_len);
                int32_t to_sends    = out_jpg_len;
                int32_t now_sends   = 0;
                uint8_t* out_buf    = out_jpg;
                uint32_t packet_len = 8 * 1024;
                while (to_sends > 0) {
                    now_sends = to_sends > packet_len ? packet_len : to_sends;
                    if (client->write(out_buf, now_sends) == 0) {
                        goto client_exit;
                    }
                    out_buf += now_sends;
                    to_sends -= now_sends;
                }

                int64_t fr_end     = esp_timer_get_time();
                int64_t frame_time = fr_end - last_frame;
                last_frame         = fr_end;
                frame_time /= 1000;
                Serial.printf("MJPG: %luKB %lums (%.1ffps)\r\n", (long unsigned int)(out_jpg_len / 1024),
                              (long unsigned int)frame_time, 1000.0 / (long unsigned int)frame_time);
            }

            if (fb) {
                esp_camera_fb_return(fb);
                fb = NULL;
            }
#ifdef USE_ATOMS3R_CAM
            if (out_jpg && needs_free) {
                free(out_jpg);
            }
            out_jpg     = NULL;
            out_jpg_len = 0;
#endif
            delay(1);
        } else {
            Serial.println("Camera capture failed");
        }
    }

client_exit:
    if (fb) {
        esp_camera_fb_return(fb);
        fb = NULL;
    }
#ifdef USE_ATOMS3R_CAM
    if (out_jpg) {
        free(out_jpg);
        out_jpg     = NULL;
        out_jpg_len = 0;
    }
#endif
    client->stop();
    Serial.printf("Image stream end\r\n");
}

static void sendJpegSnapshot(WiFiClient* client)
{
    fb = esp_camera_fb_get();
    if (!fb) {
        client->println("HTTP/1.1 503 Service Unavailable\r\n\r\n");
        return;
    }

    bool needs_free = false;
#ifdef USE_ATOMS3R_CAM
    if (!frame2jpg(fb, kJpegQuality, &out_jpg, &out_jpg_len)) {
        out_jpg = NULL;
        out_jpg_len = 0;
    } else {
        needs_free = true;
    }
#endif

#ifdef USE_ATOMS3R_M12
    out_jpg     = fb->buf;
    out_jpg_len = fb->len;
#endif

    if (out_jpg == NULL || out_jpg_len == 0) {
        client->println("HTTP/1.1 503 Service Unavailable\r\n\r\n");
        goto snapshot_cleanup;
    }

    client->println("HTTP/1.1 200 OK");
    client->println("Content-Type: image/jpeg");
    client->println("Access-Control-Allow-Origin: *");
    client->println("Cache-Control: no-store");
    client->printf("Content-Length: %u\r\n", (unsigned int)out_jpg_len);
    client->println("Connection: close");
    client->println();
    client->write(out_jpg, out_jpg_len);
    client->flush();
    delay(20);
    client->stop();

snapshot_cleanup:
    if (fb) {
        esp_camera_fb_return(fb);
        fb = NULL;
    }
#ifdef USE_ATOMS3R_CAM
    if (out_jpg && needs_free) {
        free(out_jpg);
    }
    out_jpg     = NULL;
    out_jpg_len = 0;
#endif
}

static void readRequestPath(WiFiClient* client, char* path, size_t path_len)
{
    String line = client->readStringUntil('\r');
    if (client->available()) {
        client->read(); // consume '\n'
    }

    int sp1 = line.indexOf(' ');
    int sp2 = sp1 >= 0 ? line.indexOf(' ', sp1 + 1) : -1;
    if (sp1 >= 0 && sp2 > sp1 + 1) {
        String p = line.substring(sp1 + 1, sp2);
        size_t len = p.length();
        if (len >= path_len) {
            len = path_len - 1;
        }
        memcpy(path, p.c_str(), len);
        path[len] = '\0';
    }

    uint32_t start_ms = millis();
    bool saw_newline = false;
    while (client->connected() && (millis() - start_ms) < 200) {
        while (client->available()) {
            char c = (char)client->read();
            if (c == '\n') {
                if (saw_newline) {
                    return;
                }
                saw_newline = true;
            } else if (c != '\r') {
                saw_newline = false;
            }
        }
    }
}

static void sendNotFound(WiFiClient* client)
{
    client->println("HTTP/1.1 404 Not Found");
    client->println("Content-Type: text/plain");
    client->println("Connection: close");
    client->println();
    client->println("Not found. Use /capture for single frame or /stream for MJPEG.");
}

static void sendIndex(WiFiClient* client)
{
    client->println("HTTP/1.1 200 OK");
    client->println("Content-Type: text/html");
    client->println("Connection: close");
    client->println();
    client->println("<html><body>");
    client->println("<p><a href=\"/capture\">/capture</a> (single JPEG)</p>");
    client->println("<p><a href=\"/stream\">/stream</a> (MJPEG stream)</p>");
    client->println("</body></html>");
}
