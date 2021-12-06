
#include <Arduino.h>
#include "board.hpp"

#if DEBUG_ENABLED

#define DebugSerial Serial

constexpr size_t kDebugMsgBufferSize = 0x100;
uint8_t debug_msg_buffer[kDebugMsgBufferSize];

void debug_init(uint32_t baud_rate)
{
  DebugSerial.begin(baud_rate);
  debug_printf(F("*** DEBUG TERMINAL ***\n"));
}

void debug_printf(const char *fmt, ...)
{  
  /* Clear buffer */
  memset(debug_msg_buffer, 0, sizeof(debug_msg_buffer));

  /* Print formatted message */
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(debug_msg_buffer, sizeof(debug_msg_buffer)-1, fmt, ap);
  va_end(ap);

  /* Send buffer */
  DebugSerial.write(debug_msg_buffer, strlen(debug_msg_buffer));
  DebugSerial.flush();
}

#endif /* DEBUG_ENABLED */

/* End */
