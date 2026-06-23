# Sistem Deteksi Kantuk Berbasis ESP32-CAM

Sistem pendeteksi kantuk secara real-time menggunakan ESP32-CAM 
Wrover Module untuk streaming video dan Python untuk analisis 
deteksi kantuk melalui pemrosesan citra.

## Komponen yang Digunakan
- ESP32-CAM Wrover Module
- Kamera OV2640 (bawaan ESP32-CAM)
- PC/Laptop (untuk menjalankan Python)

## Struktur Project
- esp32-wifi/   → Kode ESP32-CAM untuk koneksi WiFi & streaming (Arduino IDE)
- kamera-vscode/ → Kode deteksi kantuk berbasis Python (VS Code)

## Cara Menjalankan

### ESP32-CAM (Arduino IDE)
1. Buka file esp32-wifi/esp32_wifi.ino di Arduino IDE
2. Sesuaikan SSID dan password WiFi di kode:
   const char* ssid = "nama_wifi";
   const char* password = "password_wifi";
3. Pilih board: AI Thinker ESP32-CAM
4. Upload kode ke ESP32-CAM
5. Buka Serial Monitor untuk melihat IP Address

### Deteksi Kantuk Python (VS Code)
1. Install library yang dibutuhkan:
   pip install opencv-python
2. Sesuaikan IP Address ESP32-CAM di main.py
3. Jalankan program:
   python main.py

## Hasil
Sistem berhasil mendeteksi kantuk secara real-time menggunakan 
stream video dari ESP32-CAM Wrover Module dan diproses 
menggunakan Python dengan OpenCV.

## Anggota Kelompok
| No | Nama | NIM |
|----|------|-----|
| 1 | Iqbal Agraprana | 2515031046 |
| 2 | Vidi A Butar Butar | 2515031053 |
| 3 | Dika Ferdianto | 2515031069 |
| 4 | Alif Khaisol | 2515031078 |
| 5 | Rafi Alfarizi | 2515031080 |
