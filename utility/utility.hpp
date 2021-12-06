
#pragma once

#include <stdarg.h>
#include <windows.h>
#include <string>
using namespace std;

string format(const char *fmt, ...);
void set_terminal_color(uint8_t attribute);
void print_hexdump(uint8_t *buffer, size_t buffer_size, int stride = 0x10);
string FormatWindowsError(void);
bool QueryComPort(int port_number, char *device_name, size_t size);
int ListComPort(bool verbose);

/* End */