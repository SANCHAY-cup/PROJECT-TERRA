// Wi-Fi + camera + live viewer (ESP32-S3 only).
#if defined(TERRA_ESP32S3)
#include <stdarg.h>
#include <stdio.h>
#include <WiFi.h>
#include <esp_camera.h>
#include <esp_http_server.h>
#include "config.h"
#include "telemetry.h"
#include "thermal.h"
#include "camera_web.h"
#include "web_page.h"

namespace {

httpd_handle_t g_web    = nullptr;   // port 80: page + JSON
httpd_handle_t g_stream = nullptr;   // port 81: MJPEG (blocks while a viewer is connected)
bool           g_camOk  = false;

// ------------------------------------------------------------------ Wi-Fi
void wifiBegin() {
  bool sta = false;
  if (WIFI_STA_SSID[0] != '\0') {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_STA_SSID, WIFI_STA_PASS);
    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 12000) delay(200);
    sta = (WiFi.status() == WL_CONNECTED);
  }
  if (!sta) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);
  }
  WiFi.setSleep(false);   // lower latency for the video stream
  IPAddress ip = sta ? WiFi.localIP() : WiFi.softAPIP();
  Serial.printf("# viewer: http://%s/  (%s)\n", ip.toString().c_str(),
                sta ? "joined your Wi-Fi" : "rover hotspot");
}

// ------------------------------------------------------------------ Camera
bool cameraBegin() {
  camera_config_t c = {};
  c.ledc_channel = LEDC_CHANNEL_0;
  c.ledc_timer   = LEDC_TIMER_0;
  c.pin_d0 = CAM_PIN_Y2;  c.pin_d1 = CAM_PIN_Y3;  c.pin_d2 = CAM_PIN_Y4;  c.pin_d3 = CAM_PIN_Y5;
  c.pin_d4 = CAM_PIN_Y6;  c.pin_d5 = CAM_PIN_Y7;  c.pin_d6 = CAM_PIN_Y8;  c.pin_d7 = CAM_PIN_Y9;
  c.pin_xclk  = CAM_PIN_XCLK;
  c.pin_pclk  = CAM_PIN_PCLK;
  c.pin_vsync = CAM_PIN_VSYNC;
  c.pin_href  = CAM_PIN_HREF;
  c.pin_sccb_sda = CAM_PIN_SIOD;
  c.pin_sccb_scl = CAM_PIN_SIOC;
  c.pin_pwdn  = CAM_PIN_PWDN;
  c.pin_reset = CAM_PIN_RESET;
  c.xclk_freq_hz = 20000000;
  c.pixel_format = PIXFORMAT_JPEG;
  c.frame_size   = FRAMESIZE_VGA;        // 640x480 = same 4:3 shape as the overlay
  c.jpeg_quality = CAM_JPEG_QUALITY;
  c.fb_count     = 2;
  c.fb_location  = CAMERA_FB_IN_PSRAM;   // needs the N16R8's PSRAM
  c.grab_mode    = CAMERA_GRAB_LATEST;   // always show the newest frame

  esp_err_t err = esp_camera_init(&c);
  if (err != ESP_OK) {
    Serial.printf("# camera: init failed (0x%x) - check wiring and CAM_PIN_* in config.h\n", err);
    return false;
  }
  sensor_t *s = esp_camera_sensor_get();
  if (s) {
    s->set_hmirror(s, CAM_HFLIP ? 1 : 0);
    s->set_vflip(s, CAM_VFLIP ? 1 : 0);
  }
  Serial.println("# camera: ready");
  return true;
}

// ------------------------------------------------------------------ HTTP handlers
esp_err_t indexHandler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, INDEX_HTML, HTTPD_RESP_USE_STRLEN);
}

esp_err_t noContentHandler(httpd_req_t *req) {   // favicon etc.
  httpd_resp_set_status(req, "204 No Content");
  return httpd_resp_send(req, nullptr, 0);
}

// Appends printf-style text to buf (no heap, no ArduinoJson: the output is tiny and fixed-shape).
void jsonPut(char *buf, size_t size, size_t &n, const char *fmt, ...) {
  if (n >= size) return;
  va_list ap;
  va_start(ap, fmt);
  int w = vsnprintf(buf + n, size - n, fmt, ap);
  va_end(ap);
  if (w > 0) n += (size_t)w;
  if (n >= size) n = size - 1;
}

esp_err_t thermalHandler(httpd_req_t *req) {
  ThermalFrame f;
  bool ok = thermalLatest(f);
  Telemetry t{};
  terraLatestTelemetry(t);

  static char buf[1280];
  size_t n = 0;
  jsonPut(buf, sizeof(buf), n, "{\"ok\":%s,\"ms\":%lu,\"gas\":%u,\"snd\":%u,\"haz\":%s",
      ok ? "true" : "false", (unsigned long)t.uptimeMs, (unsigned)t.gas, (unsigned)t.soundPP,
      anyHazard(t) ? "true" : "false");
  if (ok) {
    jsonPut(buf, sizeof(buf), n, ",\"t\":[");
    for (uint8_t i = 0; i < terra::THERMAL_N; i++) jsonPut(buf, sizeof(buf), n, i ? ",%.1f" : "%.1f", (double)f.c[i]);
    jsonPut(buf, sizeof(buf), n, "],\"min\":%.1f,\"max\":%.1f,\"base\":%.1f,\"cell\":%u,\"found\":%s,\"ang\":%.1f,"
        "\"dir\":\"%s\",\"act\":\"%s\"",
        (double)f.r.minC, (double)f.r.maxC, (double)f.r.baselineC, (unsigned)f.r.hotIndex, f.r.found ? "true" : "false",
        (double)f.r.angleDeg, terra::directionName(f.r.dir), terra::actionFor(f.r));
  }
  jsonPut(buf, sizeof(buf), n, "%c", '}');

  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, buf, n);
}

esp_err_t streamHandler(httpd_req_t *req) {
  if (!g_camOk) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "camera not available");
    return ESP_FAIL;
  }
  httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=terraframe");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");

  esp_err_t res = ESP_OK;
  while (res == ESP_OK) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) { res = ESP_FAIL; break; }
    char hdr[96];
    int hl = snprintf(hdr, sizeof(hdr),
                      "\r\n--terraframe\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",
                      (unsigned)fb->len);
    res = httpd_resp_send_chunk(req, hdr, hl);
    if (res == ESP_OK) res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
    esp_camera_fb_return(fb);       // always give the buffer back
  }
  return res;
}

}  // namespace

// ------------------------------------------------------------------ public
void webBegin() {
  wifiBegin();
  g_camOk = cameraBegin();

  httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
  cfg.server_port = 80;
  cfg.stack_size  = 8192;
  cfg.max_uri_handlers = 6;
  cfg.lru_purge_enable = true;

  httpd_uri_t indexUri   = {.uri = "/",            .method = HTTP_GET, .handler = indexHandler,     .user_ctx = nullptr};
  httpd_uri_t thermalUri = {.uri = "/thermal",     .method = HTTP_GET, .handler = thermalHandler,   .user_ctx = nullptr};
  httpd_uri_t iconUri    = {.uri = "/favicon.ico", .method = HTTP_GET, .handler = noContentHandler, .user_ctx = nullptr};
  if (httpd_start(&g_web, &cfg) == ESP_OK) {
    httpd_register_uri_handler(g_web, &indexUri);
    httpd_register_uri_handler(g_web, &thermalUri);
    httpd_register_uri_handler(g_web, &iconUri);
  } else {
    Serial.println("# web: could not start the page server");
  }

  httpd_config_t scfg = HTTPD_DEFAULT_CONFIG();
  scfg.server_port = 81;
  scfg.ctrl_port  += 1;              // a second server needs its own control port
  scfg.stack_size  = 8192;
  httpd_uri_t streamUri = {.uri = "/stream", .method = HTTP_GET, .handler = streamHandler, .user_ctx = nullptr};
  if (httpd_start(&g_stream, &scfg) == ESP_OK) {
    httpd_register_uri_handler(g_stream, &streamUri);
  } else {
    Serial.println("# web: could not start the stream server");
  }
}

#endif  // TERRA_ESP32S3
