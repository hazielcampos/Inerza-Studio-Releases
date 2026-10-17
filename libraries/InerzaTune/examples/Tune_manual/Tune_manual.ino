// Ajuste en vivo a mano (sin activarlo en Inerza Studio).
#include <InerzaTune.h>

float kp = 1.2;
int velocidad = 100;
bool motores = true;

InerzaTuneVar variables[] = {
  {"kp", kp},
  {"velocidad", velocidad},
  {"motores", motores},
};

void setup() {
  InerzaTune.begin(variables, 3);
}

void loop() {
  delay(20);
}
