/*
 * nova-esp32 — ESP32-CAM
 *
 * Wi-Fi -> PC API (foto + ML)
 * Serial 9600 -> Arduino Uno (DROP:... / SENSOR:...)
 *
 * 
 */

#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>

// AYARLAR 
const char* WIFI_SSID     = "x";
const char* WIFI_PASS     = "x";
const char* SERVER_HOST   = "x";  // PC ipconfig IPv4
const int   SERVER_PORT   = "x";

const unsigned long PHOTO_MS        = 20000;  // 20 sn'de bir foto
const unsigned long SENSOR_POST_MS  = 2000;
const unsigned long WIFI_RETRY_MS   = 10000;

// AI-Thinker ESP32-CAM pinleri
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#define FLASH_GPIO_NUM     4

String imageUrl() {
  return String("http://") + SERVER_HOST + ":" + SERVER_PORT + "/api/Data/esp32-upload";
}

String sensorUrl() {
  return String("http://") + SERVER_HOST + ":" + SERVER_PORT + "/api/sensor/batch";
}

String lastSensorLine;
String unoLine;

unsigned long lastPhotoAt = 0;
unsigned long lastSensorPostAt = 0;
unsigned long lastWifiTryAt = 0;
bool httpSensorBusy = false;
bool httpPhotoBusy = false;

String foldTurkish(const String& raw) {
  String s = raw;
  s.trim();
  s.replace("\"", "");
  s.replace("'", "");
  s.toLowerCase();
  s.replace("ğ", "g");
  s.replace("ü", "u");
  s.replace("ş", "s");
  s.replace("ı", "i");
  s.replace("ö", "o");
  s.replace("ç", "c");
  return s;
}

String normalizeClass(const String& raw) {
  String s = foldTurkish(raw);
  if (s.indexOf("kagit") >= 0 || s.indexOf("paper") >= 0) return "kagit";
  if (s.indexOf("plastik") >= 0 || s.indexOf("plastic") >= 0) return "plastik";
  if (s.indexOf("cam") >= 0 || s.indexOf("glass") >= 0) return "cam";
  if (s.indexOf("metal") >= 0) return "metal";
  return "";
}

bool connectWiFi(bool verbose) {
  if (WiFi.status() == WL_CONNECTED) return true;

  if (verbose) {
    Serial.print("Wi-Fi baglaniyor: ");
    Serial.println(WIFI_SSID);
  }

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);  // ESP32-CAM için önemli
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  for (int i = 0; i < 60 && WiFi.status() != WL_CONNECTED; i++) {
    delay(500);
    if (verbose && (i % 2 == 1)) Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    if (verbose) {
      Serial.println();
      Serial.print("Wi-Fi baglandi. IP: ");
      Serial.println(WiFi.localIP());
      Serial.print("Sunucu: ");
      Serial.println(imageUrl());
    }
    return true;
  }

  if (verbose) {
    Serial.println();
    Serial.println("Wi-Fi HATA: baglanamadi. Modem 2.4GHz ve sifreyi kontrol et.");
  }
  return false;
}

void pollUnoSerial() {
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (unoLine.length() > 0) {
        unoLine.trim();
        if (unoLine.startsWith("SENSOR:")) {
          lastSensorLine = unoLine;
        }
        unoLine = "";
      }
    } else {
      unoLine += c;
    }
  }
}

void postSensorBatch() {
  if (lastSensorLine.length() == 0 || httpPhotoBusy) return;
  if (!connectWiFi(false)) return;

  httpSensorBusy = true;
  HTTPClient http;
  http.setTimeout(15000);
  http.setReuse(false);
  http.begin(sensorUrl());
  http.addHeader("Content-Type", "text/plain");
  http.addHeader("Connection", "close");
  int code = http.POST((uint8_t*)lastSensorLine.c_str(), lastSensorLine.length());
  Serial.printf("Sensor POST %d\n", code);
  http.end();
  delay(50);
  httpSensorBusy = false;
}

void captureAndUpload() {
  if (httpPhotoBusy) return;
  unsigned long waitStart = millis();
  while (httpSensorBusy && millis() - waitStart < 3000) delay(20);

  if (!connectWiFi(false)) {
    Serial.println("Foto atlandi: Wi-Fi yok.");
    return;
  }

  httpPhotoBusy = true;

  digitalWrite(FLASH_GPIO_NUM, HIGH);
  delay(400);

  for (int i = 0; i < 2; i++) {
    camera_fb_t* stale = esp_camera_fb_get();
    if (stale) esp_camera_fb_return(stale);
    delay(150);
  }

  camera_fb_t* fb = esp_camera_fb_get();
  digitalWrite(FLASH_GPIO_NUM, LOW);

  if (!fb) {
    Serial.println("Foto alinamadi.");
    httpPhotoBusy = false;
    return;
  }

  Serial.printf("Foto boyutu: %u byte\n", fb->len);

  HTTPClient http;
  http.setTimeout(60000);
  http.setReuse(false);
  http.begin(imageUrl());
  http.addHeader("Content-Type", "application/octet-stream");
  http.addHeader("Connection", "close");

  int code = http.POST(fb->buf, fb->len);
  Serial.printf("Foto POST %d\n", code);

  if (code == 200) {
    String response = http.getString();
    response.trim();
    Serial.printf("ML cevap: [%s]\n", response.c_str());
    String wasteClass = normalizeClass(response);
    if (wasteClass.length() > 0) {
      Serial.println("DROP:" + wasteClass);
    } else {
      Serial.println("ML cevap anlasilmadi.");
    }
  } else if (code <= 0) {
    Serial.println("Sunucuya ulasilamadi. PC IP ve VS F5 kontrol et.");
  }

  http.end();
  delay(100);
  esp_camera_fb_return(fb);
  httpPhotoBusy = false;
}

void testServerReachable() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Sunucu testi atlandi: Wi-Fi yok.");
    return;
  }

  String url = String("http://") + SERVER_HOST + ":" + SERVER_PORT + "/api/sensor/latest";
  HTTPClient http;
  http.setTimeout(10000);
  http.setReuse(false);
  http.begin(url);
  int code = http.GET();
  Serial.printf("Sunucu test GET %d (%s)\n", code, url.c_str());
  if (code <= 0) {
    Serial.println("PC'ye ulasilamadi -> Windows Firewall veya VS F5 kontrol et.");
  }
  http.end();
}

bool initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 12;
  config.fb_count = 2;
  config.grab_mode = CAMERA_GRAB_LATEST;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Kamera HATA: 0x%x\n", err);
    return false;
  }
  return true;
}

void setup() {
  Serial.begin(9600);
  delay(1000);  // USB serial için

  Serial.println();
  Serial.println("=== nova-esp32 basliyor ===");

  pinMode(FLASH_GPIO_NUM, OUTPUT);
  digitalWrite(FLASH_GPIO_NUM, LOW);

  // Wi-Fi gelene kadar bekle 
  while (!connectWiFi(true)) {
    Serial.println("30 sn sonra tekrar denenecek...");
    delay(30000);
  }

  testServerReachable();

  if (initCamera()) {
    Serial.println("Kamera hazir.");
  }

  Serial.println("nova-esp32 hazir.");
  lastWifiTryAt = millis();
}

void loop() {
  pollUnoSerial();

  unsigned long now = millis();

  if (WiFi.status() != WL_CONNECTED && now - lastWifiTryAt >= WIFI_RETRY_MS) {
    lastWifiTryAt = now;
    connectWiFi(true);
  }

  if (now - lastSensorPostAt >= SENSOR_POST_MS) {
    lastSensorPostAt = now;
    postSensorBatch();
  }

  if (now - lastPhotoAt >= PHOTO_MS) {
    lastPhotoAt = now;
    captureAndUpload();
  }
}
