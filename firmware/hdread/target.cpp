
#include <stdint.h>
#include "target.hpp"
#include "board.hpp"

// Pulse target clock pin N times
void clock_target(int count)
{
  for(int i = 0; i < count; i++)
  {
    digitalWrite(pin_extal, LOW);
    delayMicroseconds(kExtalPulseWidthLoUs);
    digitalWrite(pin_extal, HIGH);
    delayMicroseconds(kExtalPulseWidthHiUs);
  }
}

// Reset target
void reset_target(void)
{
  digitalWrite(pin_res_n, LOW);
  clock_target(kNumResetClocks);
  digitalWrite(pin_res_n, HIGH);
}

// Sample ports B and C
void get_target_state(target_state_t *state)
{
#if 1
  state->adl = (PIND & 0xFC) | ((PINB >> 3) & 0x03);
  state->ah = (PINC & 0x07) | ((PINC >> 1) & 0x08);
  state->strobe = (PINC >> 3) & 1;
  state->num = (PINC >> 5) & 1;
#else
  /* Read port B */
  uint8_t temp = 0;
  temp |= digitalRead(pin_b0) << 0;
  temp |= digitalRead(pin_b1) << 1;
  temp |= digitalRead(pin_b2) << 2;
  temp |= digitalRead(pin_b3) << 3;
  temp |= digitalRead(pin_b4) << 4;
  temp |= digitalRead(pin_b5) << 5;
  temp |= digitalRead(pin_b6) << 6;
  temp |= digitalRead(pin_b7) << 7;

  /* Read port C ,*/
  state->adl = temp;
  state->ah = 0;
  state->ah |= digitalRead(pin_c0) << 0;
  state->ah |= digitalRead(pin_c1) << 1;
  state->ah |= digitalRead(pin_c2) << 2;
  state->ah |= digitalRead(pin_c4) << 3;
  state->strobe = digitalRead(pin_c3);
  state->num = digitalRead(pin_num) & 1;
#endif  
}


/* End */
