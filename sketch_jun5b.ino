#include "HX711.h"

const int loadcell_dt_plastic = 6;
const int loadcell_sck_plastic = 7;

const int loadcell_dt_glass = 8;
const int loadcell_sck_glass = 9;

const int loadcell_dt_paper = 10;
const int loadcell_sck_paper = 11;

const int loadcell_dt_metal = 12;
const int loadcell_sck_metal = 13;

HX711 scale_plastic;
HX711 scale_glass;
HX711 scale_paper;
HX711 scale_metal;

float cal_plastic = 1.0;
float cal_glass   = 1.0;
float cal_paper   = 1.0;
float cal_metal   = 1.0;

void setup() {
  Serial.begin(115200); 

  scale_plastic.begin(loadcell_dt_plastic, loadcell_sck_plastic);
  scale_glass.begin(loadcell_dt_glass, loadcell_sck_glass);
  scale_paper.begin(loadcell_dt_paper, loadcell_sck_paper);
  scale_metal.begin(loadcell_dt_metal, loadcell_sck_metal);

  scale_plastic.set_scale(); scale_plastic.tare();
  scale_glass.set_scale();   scale_glass.tare();
  scale_paper.set_scale();   scale_paper.tare();
  scale_metal.set_scale();   scale_metal.tare();
}

void loop() {
  scale_plastic.set_scale(cal_plastic);
  scale_glass.set_scale(cal_glass);
  scale_paper.set_scale(cal_paper);
  scale_metal.set_scale(cal_metal);

  float gram_plastic = abs(scale_plastic.get_units(10));
  float gram_glass   = abs(scale_glass.get_units(10));
  float gram_paper   = abs(scale_paper.get_units(10));
  float gram_metal   = abs(scale_metal.get_units(10));

  Serial.print("plastic:"); Serial.println(gram_plastic, 1);
  Serial.print("glass:");   Serial.println(gram_glass, 1);
  Serial.print("paper:");   Serial.println(gram_paper, 1);
  Serial.print("metal:");   Serial.println(gram_metal, 1);

  delay(1000);
}
