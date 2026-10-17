// InerzaOTA — actualizar el programa de un ESP32 por Wi-Fi (OTA).
//
// Uso automático: activa «Subir por Wi-Fi (OTA)» en Inerza Studio. Al compilar se añade
// la configuración (`inerzaOTAConfig`) y el OTA arranca solo antes de setup(), en su
// propia tarea: no hay que llamar a nada desde tu código, aunque loop() use delay().
//
// Uso manual: incluye esta cabecera y llama a InerzaOTA.begin({...}) en setup().
#pragma once

#include <Arduino.h>

struct InerzaOTAConfig {
  const char *hostname;      // nombre en la red (robot → robot.local)
  const char *password;      // contraseña para subir ("" = sin contraseña)
  const char *ssid;          // red Wi-Fi si tu programa no se conecta ("" = la conecta tu código)
  const char *wifiPassword;  // contraseña de esa red
};

// Inerza Studio la define al compilar con el OTA activado. Si no existe, el OTA no
// arranca solo (se usa InerzaOTA.begin a mano).
extern const InerzaOTAConfig inerzaOTAConfig __attribute__((weak));

class InerzaOTAClass {
public:
  // Arranca el OTA en segundo plano: espera a que haya Wi-Fi y queda escuchando.
  void begin(const InerzaOTAConfig &config);
  // El OTA está escuchando (hay Wi-Fi y se puede subir).
  bool ready() const;
  // Se está recibiendo un programa nuevo.
  bool updating() const;
  // Función a llamar justo antes de actualizar (p. ej. parar los motores del robot).
  void onStart(void (*callback)());
};

extern InerzaOTAClass InerzaOTA;
