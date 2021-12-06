
#pragma once

#define ASCII_ESC             0x1B    /* Escape key */
#define COMMS_ACK             0xA5    /* Command accepted */
#define COMMS_NACK            0xAA    /* Command rejected */

/* Commands the PC sends us to process */
enum  main_cmd {
  CMD_NOP                 =   0x00,
  CMD_ECHO                =   0x01,
  CMD_BLINK               =   0x02,
  CMD_WRITE               =   0x20,
  CMD_READ                =   0x21,
  CMD_STATUS              =   0x22,
  CMD_ERASE               =   0x23,
  CMD_FUSE                =   0x24,  
  CMD_MODE                =   0x26,
  CMD_TEST                =   0x27,
};

/* Commands we send the PC to process */
enum sub_cmd {
  SUB_CMD_LOG             =   0x22,   /* Print message to PC */
  SUB_CMD_EXIT            =   0x24,   /* Tell PC to stop processing commands */
  SUB_CMD_GET_PAGE        =   0x25,   /* Get binary data from PC */
  SUB_CMD_SEND_PAGE       =   0x26,   /* Send binary data to PC */
  SUB_CMD_GET_PARAMETERS  =   0x27,   /* Get parameter list from PC */
};

constexpr size_t kMaxParameters = 0x10;
constexpr size_t kPageSize      = 0x40;
constexpr size_t kMaxMsgSize    = 0x80;
constexpr uint8_t kChecksumInit = 0x81;

extern uint8_t parameters[kMaxParameters];
extern uint8_t page_buffer[kPageSize];

uint8_t comms_getb(void);
void comms_sendb(uint8_t data);
void comms_send(uint8_t *data, size_t size);
void comms_get(uint8_t *data, size_t size);
void comms_puts(const char *msg);
void comms_printf(const char *fmt, ...);
void comms_get_parameters(uint8_t *parameters);
void comms_acknowledge_command(uint8_t command);
