
#include <stdint.h>
#include <Arduino.h>
#include "comms.hpp"
#include "board.hpp"

char fmt_buffer[kMaxMsgSize];
char msg_buffer[kMaxMsgSize];
uint8_t parameters[kMaxParameters];
uint8_t page_buffer[kPageSize];

// Get byte from host PC
uint8_t comms_getb(void)
{
  while(Serial.available() == 0)
  {
      delayMicroseconds(1);
  }
  
  return Serial.read();
}

// Send byte to host PC
void comms_sendb(uint8_t data)
{
  Serial.write(data);
  Serial.flush();
}

// Send bytes to host PC
void comms_send(uint8_t *data, size_t size)
{
  Serial.write(data, size);
  Serial.flush();
}

// Get bytes from host PC
void comms_get(uint8_t *data, size_t size)
{  
  size_t offset = 0;
  while(offset < size)
  {
    int bytes_ready = Serial.available();
    if(bytes_ready)
    {
      int bytes_read = Serial.readBytes(&data[offset], bytes_ready);
      offset += bytes_read;
    }
    else
    {
      /* Wait for 1ms if no data is ready */
      delay(1); 
    }
  }
}


void comms_puts(const char *msg)
{
  size_t size = strlen(msg);
  comms_sendb(SUB_CMD_LOG);
  comms_sendb(size);  
  comms_send(msg, size);
}

void comms_printf(const char *fmt, ...)
{
  memset(msg_buffer, 0, sizeof(msg_buffer));
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(msg_buffer, sizeof(msg_buffer) - 1, fmt, ap);
  va_end(ap);
  comms_puts(msg_buffer);
}

void comms_get_parameters(uint8_t *parameters)
{
  comms_sendb(SUB_CMD_GET_PARAMETERS);
  uint8_t size = comms_getb();
  comms_printf(PSTR("Get %d parameters from host\n"), size);
  for(int i = 0; i < size; i++) {
    parameters[i] = comms_getb();
  }
}

// Send COMMS_ACK and the command back
void comms_acknowledge_command(uint8_t command)
{
  comms_sendb(COMMS_ACK);
  comms_sendb(command);
}


/* End */
