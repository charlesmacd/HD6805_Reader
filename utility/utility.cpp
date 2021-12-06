
#include "utility.hpp"

/* Find device name associated with port if present */
bool QueryComPort(int port_number, char *device_name, size_t size)
{
    char com_name[256];
    sprintf(com_name, "COM%d", port_number);
    DWORD status = QueryDosDevice(com_name, device_name, size);

    if(status != 0) {
        return true;
    }

    return false;
}


int ListComPort(bool verbose)
{
    constexpr size_t buffer_size = 256;
    const char *serial_converter_prefix = "USBSER";
    int port_min = 1;
    int port_max = 128;
    bool found = false;
    char result[buffer_size];
    char device_name[buffer_size];
    int index = -1;

    if(verbose) {
        printf("Scanning for devices:\n");
    }

    for(int i = port_min; i < port_max; i++)
    {
        if(QueryComPort(i, device_name, sizeof(device_name)))
        {
            if(verbose)
                printf("- Found device COM%d (%s)\n", i, device_name);

            if(strstr(device_name, serial_converter_prefix))
            {                
                index = i;
                found = true;
            }
        }
    }

    if(!found)
    {
        if(verbose)
            printf("- No devices available.\n");
        return -1;
    }

    return index;
}


/* Return a formatted string */
string format(const char *fmt, ...)
{
    constexpr size_t kBufferSize = 256;
    char buffer[kBufferSize];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, ap);
    va_end(ap);
    return string(buffer);
}


/* Set terminal text color */
void set_terminal_color(uint8_t attribute)
{
    static HANDLE console = nullptr;
    if(console == nullptr) {
        console = GetStdHandle(STD_OUTPUT_HANDLE);
    }
    SetConsoleTextAttribute(console, attribute);
}

/* Print buffer as hex dump to stdout */
void print_hexdump(uint8_t *buffer, size_t buffer_size, int stride)
{
    int wrap = (stride - 1);
    int base = 0;
    uint8_t checksum = 0x81;
    uint8_t last_checksum = 0x80;
    string line;

    for(int index = 0; index < buffer_size; index++)
    {
        /* New line, reset checksum, base address, and print offset */
        if((index & wrap) == 0x00)
        {
            line.clear();
            line += format("%04X: ", index);
            base = index;
            checksum = 0x81;
        }

        /* Print data and update checksum */
        line += format("%02X ", buffer[index]);
        checksum += buffer[index];

        /* End of line processing */
        if(((index & wrap) == wrap) || (index == buffer_size))
        {
            /* Pad hex dump if stride isn't complete */
            size_t stride_remainder = buffer_size - index;
            if(index == buffer_size && stride_remainder)
            {
                for(int j = stride_remainder; j < stride; j++)
                {
                    line += "-- ";
                }
            }

            /* Add ASCII dump */
            line += "| ";
            for(int j = 0; j < stride; j++)
            {
                char ch = buffer[base+j];
                line += isprint(ch) ? ch : '.';
            }
            line += "\n";

            /* If line content is unique, print it */
            if(checksum != last_checksum)
            {
                last_checksum = checksum;
                printf(line.c_str());
                fflush(stdout);
                line.clear();
            }
        }
    }

    /* If the last line was a duplicate but we're out of data,
       print the last line anyway */
    if(line.size())
    {
        printf(":\n%s", line.c_str());
        fflush(stdout);
    }
}

/* Print a Windows error message */
string FormatWindowsError(void)
{
    DWORD error = GetLastError();
    LPVOID lpMsgBuf;

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, 
        NULL 
        );
    return string((const char *)lpMsgBuf);
}

/* End */