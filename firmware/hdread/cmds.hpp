
#pragma once

uint8_t shuffle(uint8_t in);
uint8_t nbit(uint8_t value);
void run_read();
int cmd_echo(void);
void read_raw_cycles(void);
void validate_adl(void);
void validate_adl_1bit(void);
uint32_t seek_bus_cycle(uint16_t address, bool debug);
void test_address_wrapping(void);
void test_address_output(bool dump);
void test_dump(bool dump);
void free_run(void);
void binary_dump(bool dump);
void cmd_read(void);
void comms_dispatch(void);
