#include "esp_camera.h"
#include <WiFi.h>
#include "esp_timer.h"
#include "img_converters.h"
#include "fb_gfx.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_http_server.h"

// ==========================================
// ðŸ”´ GANTI DENGAN WI-FI RUMAH / HOTSPOT HP MU
// ==========================================
const char* ssid = "Redmi12";
const char* password = "11111111";

// --- KONFIGURASI ALARM BUZZER & AI k-NN ---
#define PIN_BUZZER   13     
#define N_FEATURES   3      
#define K_VALUE      3      
#define MAX_SAMPLES  15     

struct Sample {
  float features[N_FEATURES];
  int label; 
};

// Dataset Latih k-NN
Sample trainingData[MAX_SAMPLES] = {
  {{0.35, 0.41, 0.02}, 0}, {{0.33, 0.40, 0.01}, 0}, {{0.36, 0.42, 0.03}, 0}, {{0.34, 0.39, 0.02}, 0}, {{0.32, 0.41, 0.01}, 0}, 
  {{0.15, 0.40, 0.02}, 1}, {{0.13, 0.42, 0.03}, 1}, {{0.14, 0.41, 0.01}, 1}, {{0.16, 0.39, 0.02}, 1}, {{0.12, 0.40, 0.02}, 1}, 
  {{0.32, 0.65, 0.04}, 2}, {{0.28, 0.68, 0.05}, 2}, {{0.34, 0.43, 0.25}, 2}, {{0.30, 0.70, 0.03}, 2}, {{0.31, 0.42, 0.22}, 2}  
};

// --- PIN DEFINITION UNTUK ESP32-WROVER-DEV ---
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     21
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       19
#define Y4_GPIO_NUM       18
#define Y3_GPIO_NUM        5
#define Y2_GPIO_NUM        4
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

httpd_handle_t camera_httpd = NULL;

// --- VARIABEL TIMER UNTUK DELAY KANTUK (3.5 DETIK) ---
unsigned long waktuMulaiKantuk = 0;
bool sedangMendeteksiKantuk = false;

// Fungsi Hitung Jarak k-NN
float hitungJarak(float* f1, float* f2) {
  float total = 0;
  for (int i = 0; i < N_FEATURES; i++) {
    float diff = f1[i] - f2[i];
    total += diff * diff;
  }
  return sqrt(total);
}

// Fungsi Klasifikasi k-NN
int klasifikasiKNN(float* inputFitur) {
  float jarak[MAX_SAMPLES];
  int indeks[MAX_SAMPLES];
  for (int i = 0; i < MAX_SAMPLES; i++) {
    jarak[i] = hitungJarak(inputFitur, trainingData[i].features);
    indeks[i] = i;
  }
  for (int i = 0; i < MAX_SAMPLES - 1; i++) {
    for (int j = 0; j < MAX_SAMPLES - i - 1; j++) {
      if (jarak[j] > jarak[j + 1]) {
        float tempJarak = jarak[j];
        jarak[j] = jarak[j + 1];
        jarak[j + 1] = tempJarak;
        int tempIndeks = indeks[j];
        indeks[j] = indeks[j + 1];
        indeks[j + 1] = tempIndeks;
      }
    }
  }
  int countKelas[3] = {0, 0, 0};
  for (int i = 0; i < K_VALUE; i++) {
    int lbl = trainingData[indeks[i]].label;
    countKelas[lbl]++;
  }
  int kelasHasil = 0;
  if (countKelas[1] > countKelas[kelasHasil]) kelasHasil = 1;
  if (countKelas[2] > countKelas[kelasHasil]) kelasHasil = 2;
  return kelasHasil;
}

// Handler Streaming Kamera
static esp_err_t capture_handler(httpd_req_t *req){
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Kamera gagal mengambil gambar");
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
  res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
  esp_camera_fb_return(fb);
  return res;
}

void startCameraServer(){
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;
  httpd_uri_t capture_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = capture_handler,
    .user_ctx  = NULL
  };
  if (httpd_start(&camera_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(camera_httpd, &capture_uri);
  }
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
  Serial.begin(115200);
  
  // Memberi waktu hardware kamera booting stabil
  delay(500); 
  
  // Inisialisasi Buzzer (Low-Level Trigger)
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, HIGH); // Buzzer mati di awal
  
  // Konfigurasi Kamera
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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  if(psramFound()){
    config.frame_size = FRAMESIZE_VGA; 
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Jalankan Kamera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Inisialisasi kamera gagal: 0x%x\n", err);
    return;
  }

  // Jalankan Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Menghubungkan ke Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi Terhubung!");

  // Jalankan Server Kamera
  startCameraServer();

  Serial.print("[SYSTEM] Kamera aktif di IP: http://");
  Serial.println(WiFi.localIP());
  Serial.println("[SYSTEM] AI k-NN Ready & Menunggu data dari laptop...");
}

void loop() {
  // Membaca data kiriman dari laptop via kabel USB
  if (Serial.available() > 0) {
    String dataInput = Serial.readStringUntil('\n');
    
    float inputFitur[N_FEATURES];
    int indexKoma1 = dataInput.indexOf(',');
    int indexKoma2 = dataInput.indexOf(',', indexKoma1 + 1);

    if (indexKoma1 != -1 && indexKoma2 != -1) {
      inputFitur[0] = dataInput.substring(0, indexKoma1).toFloat();
      inputFitur[1] = dataInput.substring(indexKoma1 + 1, indexKoma2).toFloat();
      inputFitur[2] = dataInput.substring(indexKoma2 + 1).toFloat();

      int hasilPrediksi = klasifikasiKNN(inputFitur);

      if (hasilPrediksi == 1 || hasilPrediksi == 2) {
        // Jika ini awal mula mata terpejam/kantuk, kunci waktu mulainya
        if (!sedangMendeteksiKantuk) {
          waktuMulaiKantuk = millis();
          sedangMendeteksiKantuk = true;
        }

        // Hitung durasi realtime mata terpejam
        unsigned long durasiKantuk = millis() - waktuMulaiKantuk;

        if (durasiKantuk >= 3500) {
          // === KONDISI KANTUK SEBENARNYA (> 3.5 DETIK) ===
          tone(PIN_BUZZER, 2700); 
          Serial.print("-> STATUS: BAHAYA KANTUK! | Kelas: ");
          Serial.println(hasilPrediksi);
        } else {
          // === HANYA KEDIPAN / MASA TUNGGU (BUZZER TETAP SENYAP) ===
          noTone(PIN_BUZZER); 
          Serial.print("-> STATUS: MATA TERPEJAM... Menunggu: ");
          Serial.print((3500 - durasiKantuk) / 1000.0);
          Serial.println(" detik lagi.");
        }
      } else {
        // === KONDISI AMAN / MATA KEMBALI TERBUKA SEBELUM 3.5 DETIK ===
        sedangMendeteksiKantuk = false; // Reset detektor waktu
        noTone(PIN_BUZZER); 
        Serial.println("-> STATUS: AMAN/SEGAR");
      }
    }
  }
  delay(1);
}
