#include "InerzaOTA.h"

#include <ArduinoOTA.h>
#include <WiFi.h>

InerzaOTAClass InerzaOTA;

static InerzaOTAConfig s_config = {"inerza", "", "", ""};
static volatile bool s_ready = false;
static volatile bool s_updating = false;
static void (*s_onStart)() = nullptr;

static void otaTask(void *) {
  const uint32_t started = millis();
  bool ownWifi = false;
  for (;;) {
    if (WiFi.status() != WL_CONNECTED) {
      // Si el programa no conecta el Wi-Fi en unos segundos, lo conecta el OTA (si tiene red).
      const bool hasNetwork = s_config.ssid && s_config.ssid[0];
      if (!ownWifi && hasNetwork && millis() - started > 4000 && WiFi.getMode() == WIFI_MODE_NULL) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(s_config.ssid, s_config.wifiPassword);
        ownWifi = true;
      }
      vTaskDelay(pdMS_TO_TICKS(250));
      continue;
    }
    if (!s_ready) {
      ArduinoOTA.setHostname(s_config.hostname);
      if (s_config.password && s_config.password[0]) {
        ArduinoOTA.setPassword(s_config.password);
      }
      ArduinoOTA.onStart([]() {
        s_updating = true;
        if (s_onStart) {
          s_onStart();
        }
        Serial.println("[OTA] Recibiendo un programa nuevo por Wi-Fi...");
      });
      ArduinoOTA.onEnd([]() {
        Serial.println("[OTA] Listo, reiniciando");
      });
      ArduinoOTA.onError([](ota_error_t error) {
        s_updating = false;
        Serial.printf("[OTA] Error %u\n", (unsigned)error);
      });
      ArduinoOTA.begin();
      s_ready = true;
      Serial.printf("[OTA] Listo para subir por Wi-Fi: %s.local (%s)\n", s_config.hostname, WiFi.localIP().toString().c_str());
    }
    ArduinoOTA.handle();
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void InerzaOTAClass::begin(const InerzaOTAConfig &config) {
  static bool started = false;
  if (started) {
    return;
  }
  started = true;
  s_config = config;
  xTaskCreatePinnedToCore(otaTask, "InerzaOTA", 6144, nullptr, 1, nullptr, ARDUINO_RUNNING_CORE);
}

bool InerzaOTAClass::ready() const {
  return s_ready;
}

bool InerzaOTAClass::updating() const {
  return s_updating;
}

void InerzaOTAClass::onStart(void (*callback)()) {
  s_onStart = callback;
}
