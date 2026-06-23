import cv2
import numpy as np
import requests
import serial
import time
import mediapipe as mp

# ==========================================
# 🔴 KONFIGURASI: SESUAIKAN DENGAN ALATMU
# ==========================================
ESP32_IP = "10.226.40.250"  # 👈 GANTI dengan IP yang muncul di Serial Monitor tadi
COM_PORT = "COM3"         # 👈 GANTI dengan Port COM ESP32-mu (misal: COM3, COM4, COM5)
BAUD_RATE = 115200

# Hubungkan Laptop ke ESP32 lewat Kabel USB
try:
    esp32_serial = serial.Serial(port=COM_PORT, baudrate=BAUD_RATE, timeout=.1)
    print(f"[SUKSES] Berhasil terhubung ke USB {COM_PORT}")
except Exception as e:
    print(f"[EROR] Gagal membuka {COM_PORT}. APAKAH SERIAL MONITOR ARDUINO IDE SUDAH DITUTUP? Eror: {e}")
    exit()

# Inisialisasi MediaPipe Face Mesh (Deteksi Wajah)
mp_face_mesh = mp.solutions.face_mesh
face_mesh = mp_face_mesh.FaceMesh(max_num_faces=1, refine_landmarks=True, min_detection_confidence=0.5, min_tracking_confidence=0.5)

URL = f"http://{ESP32_IP}/"

def hitung_ear(landmarks, eye_indices):
    # Rumus matematika sederhana untuk mendeteksi mata terbuka/tertutup
    p2_p6 = np.linalg.norm(np.array(landmarks[eye_indices[1]]) - np.array(landmarks[eye_indices[5]]))
    p3_p5 = np.linalg.norm(np.array(landmarks[eye_indices[2]]) - np.array(landmarks[eye_indices[4]]))
    p1_p4 = np.linalg.norm(np.array(landmarks[eye_indices[0]]) - np.array(landmarks[eye_indices[3]]))
    return (p2_p6 + p3_p5) / (2.0 * p1_p4)

# Titik koordinat mata pada MediaPipe
LEFT_EYE = [362, 385, 387, 263, 373, 380]
RIGHT_EYE = [33, 160, 158, 133, 153, 144]

print("\n[SYSTEM] Memulai Sistem AI... Tekan tombol 'q' pada jendela video untuk keluar.")

while True:
    try:
        # 1. Ambil gambar dari Kamera ESP32 via Wi-Fi
        img_resp = requests.get(URL, timeout=2)
        img_arr = np.array(bytearray(img_resp.content), dtype=np.uint8)
        frame = cv2.imdecode(img_arr, -1)
        
        if frame is None:
            continue
            
        h, w, _ = frame.shape
        rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        results = face_mesh.process(rgb_frame)
        
        # Nilai default jika wajah tidak terdeteksi (Kondisi Aman)
        ear_rata = 0.35
        pitch = 0.41
        roll = 0.02
        
        # 2. Jika wajah terdeteksi, hitung koordinatnya
        if results.multi_face_landmarks:
            for face_landmarks in results.multi_face_landmarks:
                landmarks = [(lm.x, lm.y, lm.z) for lm in face_landmarks.landmark]
                
                # Hitung Nilai Mata (EAR)
                ear_left = hitung_ear(landmarks, LEFT_EYE)
                ear_right = hitung_ear(landmarks, RIGHT_EYE)
                ear_rata = (ear_left + ear_right) / 2.0
                
                # Hitung Kemiringan Kepala Sederhana (Pitch & Roll)
                hidung = landmarks[4]
                dahi = landmarks[10]
                dagu = landmarks[152]
                
                dist_dahi_hidung = np.abs(dahi[1] - hidung[1])
                dist_dahi_dagu = np.abs(dahi[1] - dagu[1])
                if dist_dahi_dagu > 0:
                    pitch = dist_dahi_hidung / dist_dahi_dagu
                
                roll = np.abs(landmarks[234][1] - landmarks[454][1])

        # 3. KIRIM DATA KE ESP32 LEWAT KABEL SERIAL
        # Format data: "EAR,Pitch,Roll\n" -> Contoh: "0.35,0.41,0.02\n"
        data_kirim = f"{ear_rata:.2f},{pitch:.2f},{roll:.2f}\n"
        esp32_serial.write(bytes(data_kirim, 'utf-8'))
        
        # Tampilkan visualisasi data di layar laptop
        cv2.putText(frame, f"EAR: {ear_rata:.2f}", (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
        cv2.putText(frame, f"Pitch: {pitch:.2f}", (10, 60), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
        cv2.imshow("MATA EDGE AI - ESP32 STREAM", frame)
        
    except Exception as e:
        print(f"[PERINGATAN] Koneksi tersendat/Eror: {e}")
        time.sleep(1)
        
    # Tekan 'q' untuk mematikan program
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cv2.destroyAllWindows()
if 'esp32_serial' in locals():
    esp32_serial.close()
print("[SYSTEM] Program Dihentikan.")
