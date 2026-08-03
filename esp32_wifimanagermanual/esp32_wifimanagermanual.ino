#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>

WebServer server(80);

// =========================================================
// FUNGSI BANTUAN: BACA & TULIS LITTLEFS
// =========================================================
String bacaCatatan(const char* namaFile) {
  File file = LittleFS.open(namaFile, "r");
  if (!file) return ""; 
  String isi = file.readString();
  file.close();
  return isi;
}

void tulisCatatan(const char* namaFile, String isiTeks) {
  File file = LittleFS.open(namaFile, "w");
  if (file) {
    file.print(isiTeks);
    file.close();
  }
}

// =========================================================
// PENGATURAN AWAL SISTEM
// =========================================================
void setup() {
  Serial.begin(115200);
  delay(1000); // Jeda sejenak agar Serial Monitor siap
  
  if (!LittleFS.begin(true)) {
    Serial.println("Gagal memuat LittleFS!");
    return;
  }

  // 1. Cek isi memori saat baru dinyalakan
  String ssidTersimpan = bacaCatatan("/ssid.txt");
  String passTersimpan = bacaCatatan("/pass.txt");

  // Bersihkan spasi kosong atau enter yang tidak sengaja terbaca
  ssidTersimpan.trim(); 
  passTersimpan.trim();

  // 2. Persimpangan Logika: Punya Memori atau Kosong?
  if (ssidTersimpan == "") {
    
    // ---------------------------------------------------
    // KONDISI A: ALAT BARU (MODE PEMANCAR / AP)
    // ---------------------------------------------------
    Serial.println("\n[MODE SETUP] Memori kosong. Memancarkan WiFi...");
    
    WiFi.softAP("Setup_Alat_IoT"); 
    Serial.print("Sambungkan HP ke WiFi 'Setup_Alat_IoT'. Lalu buka IP: ");
    Serial.println(WiFi.softAPIP());

    // Tampilkan formulir HTML
    server.serveStatic("/", LittleFS, "/setup.html");

    // Resepsionis penerima paket data (POST)
    server.on("/simpan", HTTP_POST, []() {
      String ssidBaru = server.arg("namawifi");
      String sandiBaru = server.arg("sandi");

      // Simpan ke LittleFS
      tulisCatatan("/ssid.txt", ssidBaru);
      tulisCatatan("/pass.txt", sandiBaru);

      // Balas ke HP agar browser tahu proses sukses
      String balasan = "<h2>Data Tersimpan!</h2><p>Alat sedang memproses restart. Silakan tutup halaman ini.</p>";
      server.send(200, "text/html", balasan);
      
      Serial.println("\n[INFO] Data baru diterima!");
      Serial.println("SSID: " + ssidBaru);
      Serial.println("Restarting dalam 2 detik...");
      
      delay(2000); // Beri waktu agar struk balasan sampai ke HP sebelum mati
      ESP.restart(); // Mulai ulang otak sistem!
    });

    server.begin();

  } else {
    
    // ---------------------------------------------------
    // KONDISI B: SUDAH ADA DATA (MODE PENUMPANG / STA)
    // ---------------------------------------------------
    Serial.println("\n[MODE NORMAL] Memori ditemukan.");
    Serial.print("Mencoba konek ke: ");
    Serial.println(ssidTersimpan);

    WiFi.begin(ssidTersimpan.c_str(), passTersimpan.c_str());

    int waktuTunggu = 0;

    // Logika Timeout: Tunggu maksimal ~10 detik (20 x 500ms)
    while (WiFi.status() != WL_CONNECTED && waktuTunggu < 20) {
      delay(500);
      Serial.print(".");
      waktuTunggu++;
    }

    // Evaluasi hasil koneksi
    if (WiFi.status() == WL_CONNECTED) {
      // SUKSES KONEK
      Serial.println("\n[BERHASIL] Terhubung ke Jaringan Utama!");
      Serial.print("IP Address Alat: ");
      Serial.println(WiFi.localIP());

      // Di sinilah nanti kode Project 3 (LED) atau alat lain dimasukkan
      server.on("/", []() {
        server.send(200, "text/plain", "Status Mesin: ONLINE. Sistem Kontrol Aktif.");
      });
      
      server.begin();
      
    } else {
      // GAGAL KONEK (Solusi B Dieksekusi)
      Serial.println("\n[ERROR] Koneksi Gagal (Timeout)!");
      Serial.println("Kemungkinan alat dipindah atau password salah.");
      Serial.println("Sistem akan menghapus memori dan me-reset alat...");
      
      LittleFS.remove("/ssid.txt");
      LittleFS.remove("/pass.txt");
      
      delay(1000);
      ESP.restart(); // Kembali jadi Mode AP
    }
  }
}

void loop() {
  server.handleClient();
}