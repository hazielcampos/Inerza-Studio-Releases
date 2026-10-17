#include "InerzaTune.h"

#include <Preferences.h>

InerzaTuneClass InerzaTune;

static InerzaTuneVar *s_vars = nullptr;
static size_t s_count = 0;
// Último valor enviado de cada variable (para avisar sólo de cambios).
static String *s_sent = nullptr;
static SemaphoreHandle_t s_lock = nullptr;

// --- Valores ↔ texto ----------------------------------------------------------------------

static int64_t readInt(const InerzaTuneVar &v) {
  switch (v.size) {
    case 1: return v.kind == 'i' ? (int64_t) * (int8_t *)v.ptr : (int64_t) * (uint8_t *)v.ptr;
    case 2: return v.kind == 'i' ? (int64_t) * (int16_t *)v.ptr : (int64_t) * (uint16_t *)v.ptr;
    case 4: return v.kind == 'i' ? (int64_t) * (int32_t *)v.ptr : (int64_t) * (uint32_t *)v.ptr;
    default: return *(int64_t *)v.ptr;
  }
}

static void writeInt(const InerzaTuneVar &v, int64_t x) {
  switch (v.size) {
    case 1: *(uint8_t *)v.ptr = (uint8_t)x; break;
    case 2: *(uint16_t *)v.ptr = (uint16_t)x; break;
    case 4: *(uint32_t *)v.ptr = (uint32_t)x; break;
    default: *(int64_t *)v.ptr = x; break;
  }
}

static String valueOf(const InerzaTuneVar &v) {
  char buf[32];
  switch (v.kind) {
    case 'b': return *(bool *)v.ptr ? "1" : "0";
    case 'i': snprintf(buf, sizeof buf, "%lld", (long long)readInt(v)); return buf;
    case 'u': snprintf(buf, sizeof buf, "%llu", (unsigned long long)(uint64_t)readInt(v)); return buf;
    case 'f': snprintf(buf, sizeof buf, "%.7g", v.size == 4 ? (double)*(float *)v.ptr : *(double *)v.ptr); return buf;
    case 's': {
      // Una línea: los saltos se envían como \n.
      String s = *(String *)v.ptr;
      s.replace("\\", "\\\\");
      s.replace("\n", "\\n");
      return s;
    }
    default: return "";
  }
}

static void setFrom(const InerzaTuneVar &v, const String &text) {
  switch (v.kind) {
    case 'b': *(bool *)v.ptr = text == "1" || text.equalsIgnoreCase("true"); break;
    case 'i': writeInt(v, strtoll(text.c_str(), nullptr, 10)); break;
    case 'u': writeInt(v, (int64_t)strtoull(text.c_str(), nullptr, 10)); break;
    case 'f':
      if (v.size == 4) {
        *(float *)v.ptr = strtof(text.c_str(), nullptr);
      } else {
        *(double *)v.ptr = strtod(text.c_str(), nullptr);
      }
      break;
    case 's': {
      String s = text;
      s.replace("\\n", "\n");
      s.replace("\\\\", "\\");
      *(String *)v.ptr = s;
      break;
    }
  }
}

// --- Valores guardados en la placa ---------------------------------------------------------

// Clave de NVS (máx. 15 caracteres): hash del nombre y del tipo.
static String keyOf(const InerzaTuneVar &v) {
  uint32_t h = 2166136261u;
  for (const char *p = v.name; *p; p++) {
    h = (h ^ (uint8_t)*p) * 16777619u;
  }
  char k[16];
  snprintf(k, sizeof k, "v%08lx%c%u", (unsigned long)h, v.kind, (unsigned)v.size);
  return k;
}

static void applySaved() {
  Preferences p;
  if (!p.begin("inerzatune", true)) {
    return;
  }
  for (size_t i = 0; i < s_count; i++) {
    String k = keyOf(s_vars[i]);
    if (p.isKey(k.c_str())) {
      setFrom(s_vars[i], p.getString(k.c_str()));
    }
  }
  p.end();
}

bool InerzaTuneClass::save() {
  Preferences p;
  if (!p.begin("inerzatune", false)) {
    return false;
  }
  bool ok = true;
  for (size_t i = 0; i < s_count; i++) {
    ok &= p.putString(keyOf(s_vars[i]).c_str(), valueOf(s_vars[i])) > 0;
  }
  p.end();
  return ok;
}

void InerzaTuneClass::clear() {
  Preferences p;
  if (p.begin("inerzatune", false)) {
    p.clear();
    p.end();
  }
}

size_t InerzaTuneClass::count() const {
  return s_count;
}

// --- Protocolo ------------------------------------------------------------------------------

static void sendLine(const String &s) {
  xSemaphoreTake(s_lock, portMAX_DELAY);
  Serial.print("@it ");
  Serial.println(s);
  xSemaphoreGive(s_lock);
}

static void sendAll() {
  sendLine(String("hello ") + s_count);
  for (size_t i = 0; i < s_count; i++) {
    s_sent[i] = valueOf(s_vars[i]);
    sendLine(String("var ") + i + " " + s_vars[i].kind + " " + s_vars[i].name + " " + s_sent[i]);
  }
}

static void handle(const String &line) {
  if (!line.startsWith("@it ")) {
    return;
  }
  String cmd = line.substring(4);
  cmd.trim();
  if (cmd == "hello") {
    sendAll();
  } else if (cmd.startsWith("set ")) {
    int sp = cmd.indexOf(' ', 4);
    if (sp < 0) {
      return;
    }
    size_t i = cmd.substring(4, sp).toInt();
    if (i < s_count) {
      setFrom(s_vars[i], cmd.substring(sp + 1));
    }
  } else if (cmd == "save") {
    sendLine(InerzaTune.save() ? "ok save" : "error save");
  } else if (cmd == "clear") {
    InerzaTune.clear();
    sendLine("ok clear");
  }
}

static void tuneTask(void *) {
  String rx;
  uint32_t lastReport = 0;
  // Se anuncia al arrancar (por si el IDE ya estaba escuchando).
  vTaskDelay(pdMS_TO_TICKS(1500));
  sendAll();
  for (;;) {
    // Sólo se leen las líneas del ajuste: el resto del Serial queda para tu programa.
    while (Serial.available()) {
      if (rx.isEmpty() && Serial.peek() != '@') {
        break;
      }
      char c = (char)Serial.read();
      if (c == '\n') {
        rx.trim();
        handle(rx);
        rx = "";
      } else if (c != '\r' && rx.length() < 200) {
        rx += c;
      }
    }
    // Avisa de los cambios que hace tu programa (para verlos en los medidores y gráficas).
    if (millis() - lastReport >= 100) {
      lastReport = millis();
      for (size_t i = 0; i < s_count; i++) {
        String v = valueOf(s_vars[i]);
        if (v != s_sent[i]) {
          s_sent[i] = v;
          sendLine(String("val ") + i + " " + v);
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void InerzaTuneClass::begin(InerzaTuneVar *vars, size_t count, unsigned long baud) {
  if (s_vars) {
    return;
  }
  s_vars = vars;
  s_count = count;
  s_sent = new String[count];
  s_lock = xSemaphoreCreateMutex();
  // Antes de setup(): los valores guardados sustituyen a los del código.
  applySaved();
  Serial.begin(baud);
  xTaskCreatePinnedToCore(tuneTask, "InerzaTune", 4096, nullptr, 1, nullptr, ARDUINO_RUNNING_CORE);
}
