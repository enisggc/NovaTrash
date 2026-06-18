/*
 * nova-uno — Arduino UNO
 *
 * Lazer (dolu/bos), HX711 (gram), 2 servo (rampa)
 * ESP32-CAM komutlari: hardware Serial pin 0/1 @ 9600 baud

 *
 * Motorlar:
 *   Servo secici (rampa yonu) -> pin 9
 *   Servo egim             -> pin 10
 *
 * Kutuphane: HX711 , Servo 
 */

#include <Servo.h>
#include <HX711.h>

// Lazer pinleri 
const uint8_t PIN_LASER_KAGIT   = 2;
const uint8_t PIN_LASER_PLASTIK = 3;
const uint8_t PIN_LASER_CAM     = 4;
const uint8_t PIN_LASER_METAL   = 5;

// Servo pinleri 
const uint8_t PIN_SERVO_SECICI = 9;
const uint8_t PIN_SERVO_EGIM   = 10;

// HX711 pinleri (ortak SCK) 
const uint8_t PIN_HX711_SCK          = 12;
const uint8_t PIN_HX711_DT_KAGIT     = A0;
const uint8_t PIN_HX711_DT_PLASTIK   = A1;
const uint8_t PIN_HX711_DT_CAM       = A2;
const uint8_t PIN_HX711_DT_METAL     = A3;

// Ayarlar 
const bool LASER_DOLU_WHEN_HIGH = true;
const unsigned long SERIAL_BAUD   = 9600;  // ESP32 ile ayni 
const unsigned long TILT_HOLD_MS  = 5000;
const unsigned long SENSOR_PRINT_MS = 500;
const int SERVO_STEP_DELAY_MS = 60;  // buyuk = daha yavas hareket
const float WEIGHT_TRIM_DIVISOR = 100.0f;  // 20000 -> 200 (son 2 basamak atmak için)

// Ortak bekleme
const int RAMPA_BEKLE       =40;  // secici: orta bekleme yeri 40
const int RAMPA_EGIM_BEKLE  = 60;  // egim: beklerken 60

//  KAGIT 
const int AC_KAGIT     = 25;
const int EGIM_KAGIT   = 30;

// PLASTIK 
const int AC_PLASTIK   = 40;
const int EGIM_PLASTIK = 45;

//  CAM 
const int AC_CAM     = 60;
const int EGIM_CAM   = 50;

//  METAL 
const int AC_METAL     = 75;
const int EGIM_METAL   = 50;

float calKagit   = 1.0f;
float calPlastik = 1.0f;
float calCam     = 1.0f;
float calMetal   = 1.0f;

// Nesneler 
Servo servoSecici;
Servo servoEgim;

HX711 scaleKagit;
HX711 scalePlastik;
HX711 scaleCam;
HX711 scaleMetal;

struct BinState {
  const char* id;
  uint8_t laserPin;
  HX711* scale;
  float* cal;
  int status;
  float weightGram;
};

BinState bins[] = {
  { "kagit",   PIN_LASER_KAGIT,   &scaleKagit,   &calKagit,   0, 0.0f },
  { "plastik", PIN_LASER_PLASTIK, &scalePlastik, &calPlastik, 0, 0.0f },
  { "cam",     PIN_LASER_CAM,     &scaleCam,     &calCam,     0, 0.0f },
  { "metal",   PIN_LASER_METAL,   &scaleMetal,   &calMetal,   0, 0.0f }
};

const uint8_t BIN_COUNT = sizeof(bins) / sizeof(bins[0]);

String serialLine;
unsigned long lastSensorPrintAt = 0;
int currentSeciciAngle = RAMPA_BEKLE;
int currentEgimAngle = RAMPA_EGIM_BEKLE;

// Sensörler 
int readLaserFull(uint8_t pin) {
  int v = digitalRead(pin);
  return LASER_DOLU_WHEN_HIGH ? (v == HIGH ? 1 : 0) : (v == LOW ? 1 : 0);
}

bool waitScaleReady(HX711& scale, unsigned long timeoutMs) {
  unsigned long start = millis();
  while (!scale.is_ready()) {
    if (millis() - start >= timeoutMs) return false;
    delay(10);
  }
  return true;
}

bool safeTare(HX711& scale, unsigned long timeoutMs = 3000) {
  if (!waitScaleReady(scale, timeoutMs)) return false;
  scale.tare();
  return true;
}

float readWeightGram(HX711& scale, float cal) {
  if (!waitScaleReady(scale, 200)) return 0.0f;
  scale.set_scale(cal);
  return abs(scale.get_units(1));
}

void updateSensors() {
  for (uint8_t i = 0; i < BIN_COUNT; i++) {
    bins[i].status = readLaserFull(bins[i].laserPin);
    bins[i].weightGram = readWeightGram(*bins[i].scale, *bins[i].cal);
  }
}

void sendSensorToEsp() {
  Serial.print("SENSOR");
  for (uint8_t i = 0; i < BIN_COUNT; i++) {
    Serial.print(i == 0 ? ':' : ';');
    Serial.print(bins[i].id);
    Serial.print(':');
    Serial.print(bins[i].status);
    Serial.print(':');
    long shownGram = (long)(bins[i].weightGram / WEIGHT_TRIM_DIVISOR);
    Serial.print(shownGram);
  }
  Serial.println();
}

// ML komutu -> motor 
String foldTurkish(String s) {
  s.trim();
  s.toLowerCase();
  s.replace("ğ", "g");
  s.replace("ü", "u");
  s.replace("ş", "s");
  s.replace("ı", "i");
  s.replace("ö", "o");
  s.replace("ç", "c");
  return s;
}

int angleForBin(const String& name) {
  String n = foldTurkish(name);

  if (n.indexOf("kagit") >= 0 || n.indexOf("paper") >= 0)     return AC_KAGIT;
  if (n.indexOf("plastik") >= 0 || n.indexOf("plastic") >= 0) return AC_PLASTIK;
  if (n.indexOf("cam") >= 0 || n.indexOf("glass") >= 0)       return AC_CAM;
  if (n.indexOf("metal") >= 0)                                 return AC_METAL;
  return -1;
}

int tiltForBin(const String& name) {
  String n = foldTurkish(name);

  if (n.indexOf("kagit") >= 0 || n.indexOf("paper") >= 0)     return EGIM_KAGIT;
  if (n.indexOf("plastik") >= 0 || n.indexOf("plastic") >= 0) return EGIM_PLASTIK;
  if (n.indexOf("cam") >= 0 || n.indexOf("glass") >= 0)       return EGIM_CAM;
  if (n.indexOf("metal") >= 0)                                 return EGIM_METAL;
  return -1;
}

void goToBekleme() {
  moveServoSlow(servoSecici, currentSeciciAngle, RAMPA_BEKLE);
  holdSeciciAt(RAMPA_BEKLE);
  moveServoSlow(servoEgim, currentEgimAngle, RAMPA_EGIM_BEKLE);
  holdEgimAt(RAMPA_EGIM_BEKLE);
}

int clampAngle(int angle) {
  if (angle < 0) return 0;
  if (angle > 180) return 180;
  return angle;
}

void moveServoSlow(Servo& servo, int& currentAngle, int targetAngle) {
  targetAngle = clampAngle(targetAngle);

  if (currentAngle == targetAngle) {
    servo.write(targetAngle);
    return;
  }

  if (currentAngle < targetAngle) {
    for (int a = currentAngle + 1; a <= targetAngle; a++) {
      servo.write(a);
      delay(SERVO_STEP_DELAY_MS);
    }
  } else {
    for (int a = currentAngle - 1; a >= targetAngle; a--) {
      servo.write(a);
      delay(SERVO_STEP_DELAY_MS);
    }
  }

  currentAngle = targetAngle;
  servo.write(targetAngle);
}

void holdSeciciAt(int angle) {
  angle = clampAngle(angle);
  for (int i = 0; i < 3; i++) {
    servoSecici.write(angle);
    delay(80);
  }
  currentSeciciAngle = angle;
}

void holdEgimAt(int angle) {
  angle = clampAngle(angle);
  for (int i = 0; i < 5; i++) {
    servoEgim.write(angle);
    delay(100);
  }
  currentEgimAngle = angle;
}

void holdTiltWithSeciciLock(int seciciAngle, int egimAngle) {
  egimAngle = clampAngle(egimAngle);
  unsigned long holdStart = millis();
  while (millis() - holdStart < TILT_HOLD_MS) {
    holdSeciciAt(seciciAngle);
    holdEgimAt(egimAngle);
    delay(250);
  }
}

void runDropSequence(const String& binName) {
  String name = binName;
  name.trim();

  int target = angleForBin(name);
  int tilt = tiltForBin(name);
  if (target < 0 || tilt < 0) return;
  target = clampAngle(target);
  tilt = clampAngle(tilt);

  moveServoSlow(servoSecici, currentSeciciAngle, target);
  holdSeciciAt(target);
  delay(400);

  moveServoSlow(servoEgim, currentEgimAngle, tilt);
  holdEgimAt(tilt);
  holdTiltWithSeciciLock(target, tilt);

  goToBekleme();
}

void handleEspLine(const String& line) {
  if (!line.startsWith("DROP:") && !line.startsWith("DROP ")) {
    return;
  }

  String binName = line.startsWith("DROP:") ? line.substring(5) : line.substring(5);
  binName.trim();
  runDropSequence(binName);
}

void pollEspCommands() {
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (serialLine.length() > 0) {
        serialLine.trim();
        handleEspLine(serialLine);
        serialLine = "";
      }
    } else {
      serialLine += c;
    }
  }
}

// Arduino 
void setup() {
  Serial.begin(SERIAL_BAUD);

  pinMode(PIN_LASER_KAGIT, INPUT_PULLUP);
  pinMode(PIN_LASER_PLASTIK, INPUT_PULLUP);
  pinMode(PIN_LASER_CAM, INPUT_PULLUP);
  pinMode(PIN_LASER_METAL, INPUT_PULLUP);

  servoSecici.attach(PIN_SERVO_SECICI, 500, 2500);
  servoEgim.attach(PIN_SERVO_EGIM, 500, 2500);
  currentSeciciAngle = clampAngle(RAMPA_BEKLE);
  currentEgimAngle = clampAngle(RAMPA_EGIM_BEKLE);
  servoSecici.write(currentSeciciAngle);
  servoEgim.write(currentEgimAngle);

  scaleKagit.begin(PIN_HX711_DT_KAGIT, PIN_HX711_SCK);
  scalePlastik.begin(PIN_HX711_DT_PLASTIK, PIN_HX711_SCK);
  scaleCam.begin(PIN_HX711_DT_CAM, PIN_HX711_SCK);
  scaleMetal.begin(PIN_HX711_DT_METAL, PIN_HX711_SCK);

  delay(500);
  safeTare(scaleKagit);
  safeTare(scalePlastik);
  safeTare(scaleCam);
  safeTare(scaleMetal);
}

void loop() {
  pollEspCommands();
  updateSensors();

  unsigned long now = millis();
  if (now - lastSensorPrintAt >= SENSOR_PRINT_MS) {
    lastSensorPrintAt = now;
    sendSensorToEsp();
  }
}
