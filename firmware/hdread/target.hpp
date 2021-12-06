
#pragma once

#define NUM_ADDR          0
#define NUM_DATA          1

class target_state_t {
public:
  uint8_t adl;      /* Multiplexed address/data */
  uint8_t ah;       /* High order address and control signals */
  uint8_t num;      /* State of NUM pin */
  uint8_t strobe;   /* State of STROBE pin */
};

constexpr int kExtalPulseWidthLoUs  = 10;
constexpr int kExtalPulseWidthHiUs  = 10;
constexpr int kNumResetClocks       = 8;        /* Seems to be fine with four */

constexpr uint32_t kMemorySize      = 0x1000;   /* 4K address bus */
constexpr uint32_t kRiotSize        = 0x80;     /* RAM, I/O, timer area */

void get_target_state(target_state_t *state);
void clock_target(int count);
void reset_target(void);
