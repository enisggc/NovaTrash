#include <Servo.h>

Servo motor1; // Pin 9  -> Rampayı sağa sola döndüren motor (Yönlendirme)
Servo motor2; // Pin 10 -> Rampayı aşağı eğen motor (Eğim/Kaydırak)

// --- YENI RAMPA MEKANIK AYARLARI ---
const int BEKLEME_ACISI = 15;  // Hazırda beklerken rampanın duracağı orta nokta
const int DUZ_RAMPA = 0;       // Rampanın düz durma açısı (eğilmemiş hali)
const int EGIK_RAMPA = 45;     // Çöpün kayması için rampanın aşağı eğilme açısı

// Kutuların konumlandığı hedef açılar 
const int AC_KAGIT = 0;
const int AC_PLASTIK = 10;
const int AC_CAM = 20;
const int AC_METAL = 30;

String gelenKelime = ""; // ESP32'den akan kelimeyi burada biriktireceğiz

void setup() {
  Serial.begin(9600); // ESP32 ile aynı hızda iletişim
  
  motor1.attach(9, 500, 2500);
  motor2.attach(10, 500, 2500);
  
  // İlk açılışta rampayı orta noktaya getir ve düz tut 
  motor1.write(BEKLEME_ACISI);
  motor2.write(DUZ_RAMPA);
}

void loop() {
  // Seri porttan veri akışı var mı kontrol et
  while (Serial.available() > 0) {
    char c = Serial.read(); // Karakter karakter oku
    
    if (c == '\n') { // ESP32 satır sonuna geldiyse
      gelenKelime.trim(); // Varsa sağdaki soldaki boşlukları temizle
      
      int hedefAci = -1;
      
      // ESP'den gelen kelimeye göre hangi kutunun açısına dönüleceğini seçiyoruz
      if (gelenKelime.indexOf("Paper") >= 0) {
        hedefAci = AC_KAGIT;
      }
      else if (gelenKelime.indexOf("Plastic") >= 0) {
        hedefAci = AC_PLASTIK;
      }
      else if (gelenKelime.indexOf("Glass") >= 0) {
        hedefAci = AC_CAM;
      }
      else if (gelenKelime.indexOf("Metal") >= 0) {
        hedefAci = AC_METAL;
      }
      
      // Eğer geçerli bir atık algılandıysa senkronize hareketi başlat
      if (hedefAci != -1) {
        // ADIM 1: Rampayı ilgili kutunun hizasına döndür
        motor1.write(hedefAci);
        delay(1500); // Motorun hedefe varması için süre tanı beybi
        
        // ADIM 2: Rampayı aşağı doğru eğ 
        motor2.write(EGIK_RAMPA);
        delay(2500); // Çöpün kayıp düşmesi için bekleme süresi
        
        // ADIM 3: Rampayı tekrar düz konuma kaldır
        motor2.write(DUZ_RAMPA);
        delay(1000); // Rampanın doğrulmasını bekle
        
        // ADIM 4: Rampayı yeni atıklar için merkeze (başlangıç konumuna) geri çek
        motor1.write(BEKLEME_ACISI);
      }
      
      gelenKelime = ""; // Yeni kelime için hafızayı boşalt 
    } 
    else {
      gelenKelime += c; // Satır sonu gelmediyse karakterleri yan yana ekle
    }
  }
}