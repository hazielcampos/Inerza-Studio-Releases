// InerzaTune — ajustar y ver variables en vivo desde Inerza Studio por el puerto serie.
//
// Uso automático: activa «Ajuste en vivo» en Inerza Studio. Al compilar se añaden las
// variables globales de tu programa y el ajuste arranca solo antes de setup(): no hay que
// cambiar el código. Los valores que hagas «permanentes» se guardan en la placa (NVS) y se
// aplican al arrancar, antes de setup().
//
// Protocolo (líneas de texto que empiezan por «@it », el resto del Serial no se toca):
//   placa → IDE:  @it hello <n> · @it var <i> <tipo> <nombre> <valor> · @it val <i> <valor>
//                 @it ok <orden>
//   IDE → placa:  @it hello · @it set <i> <valor> · @it save · @it clear
#pragma once

#include <Arduino.h>
#include <type_traits>

struct InerzaTuneVar {
  const char *name;
  void *ptr;
  uint8_t size;
  // 'b' bool · 'i' entero con signo · 'u' sin signo · 'f' decimal · 's' String
  char kind;

  template <typename T>
  InerzaTuneVar(const char *n, T &v) : name(n), ptr((void *)&v), size(sizeof(T)), kind('?') {
    using U = std::remove_cv_t<T>;
    if constexpr (std::is_same_v<U, bool>) {
      kind = 'b';
    } else if constexpr (std::is_integral_v<U>) {
      kind = std::is_signed_v<U> ? 'i' : 'u';
    } else if constexpr (std::is_floating_point_v<U>) {
      kind = 'f';
    } else if constexpr (std::is_same_v<U, String>) {
      kind = 's';
    }
  }
};

class InerzaTuneClass {
public:
  // Registra las variables, aplica las guardadas en la placa y arranca la tarea del ajuste.
  void begin(InerzaTuneVar *vars, size_t count, unsigned long baud = 115200);
  // Guarda en la placa los valores actuales (se aplican al arrancar).
  bool save();
  // Olvida los valores guardados (vuelven los del código).
  void clear();
  size_t count() const;
};

extern InerzaTuneClass InerzaTune;
