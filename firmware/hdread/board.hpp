
#pragma once
#include <Arduino.h>

#define DEBUG_ENABLED         0

/* Arduino pin assignments */
constexpr int pin_b2        = 2;
constexpr int pin_b3        = 3;
constexpr int pin_b4        = 4;
constexpr int pin_b5        = 5;
constexpr int pin_b6        = 6;
constexpr int pin_b7        = 7;
constexpr int pin_res_n     = 8;
constexpr int pin_extal     = 9;
constexpr int pin_tps_en_n  = 10;
constexpr int pin_b0        = 11;
constexpr int pin_b1        = 12;
constexpr int pin_tps_flt_n = 13;
constexpr int pin_c0        = A0; // A8
constexpr int pin_c1        = A1; // A9
constexpr int pin_c2        = A2; // A10
constexpr int pin_c3        = A3; // Strobe (used by other opcodes than NOP)
constexpr int pin_c4        = A4; // A11
constexpr int pin_num       = A5; // NUM

#if DEBUG_ENABLED
void debug_init(uint32_t baud_rate);
void debug_printf(const char *fmt, ...);
#else
#define debug_init(x)
#define debug_printf(...)
#endif

constexpr uint32_t kHostBaudRate = 115200;
