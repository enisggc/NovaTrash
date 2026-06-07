const int sensor_plastic = 2;
const int sensor_glass   = 3;
const int sensor_paper   = 4;
const int sensor_metal   = 5;

void setup() {
  Serial.begin(9600); 
  pinMode(sensor_plastic, INPUT_PULLUP); 
  pinMode(sensor_glass,   INPUT_PULLUP); 
  pinMode(sensor_paper,   INPUT_PULLUP); 
  pinMode(sensor_metal,   INPUT_PULLUP); 
}

void loop() {
  if (digitalRead(sensor_plastic) == HIGH) {
    Serial.println("plastic:1");
  } else {
    Serial.println("plastic:0");
  }

  if (digitalRead(sensor_glass) == HIGH) {
    Serial.println("glass:1");
  } else {
    Serial.println("glass:0");
  }

  if (digitalRead(sensor_paper) == HIGH) {
    Serial.println("paper:1");
  } else {
    Serial.println("paper:0");
  }

  if (digitalRead(sensor_metal) == HIGH) {
    Serial.println("metal:1");
  } else {
    Serial.println("metal:0");
  }

  delay(500); 
}
