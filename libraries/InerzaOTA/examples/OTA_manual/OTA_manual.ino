// OTA a mano (sin activarlo en Inerza Studio): sube este programa una vez por USB y
// después elige la placa «inerza-ota» en el selector de puertos para subir por Wi-Fi.
#include <WiFi.h>
#include <InerzaOTA.h>

void pararMotores() {
  // Aquí lo que tu robot deba hacer antes de actualizarse.
}

void setup() {
  Serial.begin(115200);
  WiFi.begin("MiRed", "clave");
  InerzaOTA.onStart(pararMotores);
  InerzaOTA.begin({"inerza-ota", "", "", ""});
}

void loop() {
  delay(1000);  // el OTA funciona aunque loop() espere: va en su propia tarea
}
