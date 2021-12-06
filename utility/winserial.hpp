
#pragma once

#include <stdint.h>
#include <windows.h>
#include <string>
#include <iostream>
#include <vector>
#include "utility.hpp"
using namespace std;

class SerialPort
{
public:
    SerialPort()
    {
    }

    ~SerialPort()
    {
        if(opened_)
        {
            close();
        }        
    }

    /* Scan COM ports 1-128 to see which ones are mapped to a DOS device name */
    bool scan(int *port_number, vector<string> *devices)
    {
        constexpr size_t kBufferSize = 256;
        constexpr int kPortMin = 1;
        constexpr int kPortMax = 128;

        bool found = false;
        char result[kBufferSize];
        char device_name[kBufferSize];

        for(int index = kPortMin; index < kPortMax; index++)
        {            
            if(get_device_name(index, device_name, sizeof(device_name)))
            {
                /* If the device list exists, add the device name */
                if(devices)
                {
                    devices->push_back(device_name);
                }

                /* Match USBSER000-USBSER999 */
                if(strstr(device_name, "USBSER"))
                {                
                    *port_number = index;
                    found = true;
                }
            }
        }

        if(!found)
        {
            return -1;
        }

        return true;
    }

    /* Open a serial port using 8-N-1, no flow control  */
    bool open(int com_number, int baud_rate)
    {
        /* If port -1 is requested, scan for available ports */
        if(com_number == -1)
        {
            if(!scan(&com_number, NULL))
            {
                printf("Error: No COM port found.\n");
                return false;
            }
        }

        /* Open file mapped to COM port */
        const char *kPortFormat = "\\\\.\\COM%d";
        sprintf(port_name, kPortFormat, com_number);
        handle = CreateFileA(
            port_name,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
            );           
        if(handle == INVALID_HANDLE_VALUE)
        {
            printf("Error: Invalid handle.\n");
            fatal_error();
            return false;
        }

        /* Configure baud rate */
        if(!configure_uart(baud_rate))
        {
            printf("Error: Couldn't configure COM port.\n");
            fatal_error();
            return false;
        }

        /* Configure timeouts */
        if(!set_comm_timeouts(kReadTimeoutMs, kWriteTimeoutMs))
        {
            printf("Error: Couldn't set comm timeouts.\n");
            fatal_error();
            return false;
        }

        /* Enable event when the UART receives a character or when the transmission buffer is empty */
        if(!SetCommMask(handle, EV_RXCHAR | EV_TXEMPTY))
        {
            printf("Error: Couldn't enable EV_RXCHAR event.\n");
            fatal_error();
            return false;
        }

        /* Flag port as successfully open */
        opened_ = true;

        return true;
    }

    /* Close the serial port */
    bool close(void)
    {
        if(!CloseHandle(handle))
        {
            return false;
        }
        return true;
    }

    void fatal_error(void)
    {
        cout << "Windows Error: " << FormatWindowsError();

        if(handle != INVALID_HANDLE_VALUE)
        {
            close();
        }
    }

    /* Retrieve the DOS device name for a given COM port. */
    bool get_device_name(int port_number, char *device_name, size_t size)
    {
        constexpr size_t kBufferSize = 256;
        char port_name[kBufferSize];
        sprintf(port_name, "COM%d", port_number);
        DWORD status = QueryDosDevice(port_name, device_name, size);
        if(status != 0)
        {
            return true;
        }
        return false;
    }


    bool configure_uart(int baud_rate = 115200)
    {
        /* Get UART parmeters */
        DCB parameters = {0};
        parameters.DCBlength = sizeof(parameters);
        
        if(!GetCommState(handle, &parameters))
        {
            fatal_error();
            return false;
        }

        parameters.BaudRate = baud_rate;
        parameters.ByteSize = 8;
        parameters.StopBits = ONESTOPBIT;
        parameters.Parity = NOPARITY;
        parameters.fDtrControl = 0;
        parameters.fRtsControl = 0;

        /* Set new UART parameters */
        return SetCommState(handle, &parameters);
    }

    bool set_comm_timeouts(int read_timeout_ms = 1, int write_timeout_ms = 1)
    {
        COMMTIMEOUTS timeouts = {0};

        /* All in milliseconds */
        timeouts.ReadIntervalTimeout = read_timeout_ms;
        timeouts.ReadTotalTimeoutConstant = 1;
        timeouts.ReadTotalTimeoutMultiplier = 1;

        timeouts.WriteTotalTimeoutConstant = write_timeout_ms;
        timeouts.WriteTotalTimeoutMultiplier = 1;

        /* Set UART timeouts */
        return SetCommTimeouts(handle, &timeouts);
    }


    /* Flush the UART receive queue */
    bool flush_rx_queue(void)
    {
        auto status = PurgeComm(handle, PURGE_RXCLEAR);
        if(!status)
        {
            fatal_error();
            return false;
        }
        return true;
    }

    /* Flush the UART transmit queue */
    bool flush_tx_queue(void)
    {
        auto status = PurgeComm(handle, PURGE_TXCLEAR);
        if(!status)
        {
            fatal_error();
            return false;
        }
        return true;
    }

    /* Write to serial port */
    bool write(uint8_t *buffer, uint32_t size)
    {
        uint32_t buffer_index = 0;

        struct {
            uint32_t expected;
            DWORD actual;
        } stride;

        while(size)
        {
            /* Try to read all remaining data in one stride */
            stride.expected = size;

            auto status = WriteFile(
                handle,
                &buffer[buffer_index],
                stride.expected,
                &stride.actual,
                NULL
                );

            /* Check if read failed */
            if(!status)
            {
                fatal_error();
                return false;
            }

            if(stride.actual)
            {
                /* Adjust remaining size for the actual amount read */
                size -= stride.actual;

                /* Advance buffer position */
                buffer_index += stride.actual;            
            }
        }
        return true;
    }

    /* Perform a blocking read from the UART until all data is read */
    bool read(uint8_t *buffer, uint32_t size)
    {
        uint32_t buffer_index = 0;

        struct {
            uint32_t expected;
            DWORD actual;
        } stride;

        /* Buffer to hold largest amount of data we can read at once */
        constexpr size_t kFragmentSize = 256;
        uint8_t fragment[kFragmentSize];

        do
        {
            /* Determine how much data we can attempt to read in a single stride */
            if(size > kFragmentSize)
            {
                stride.expected = kFragmentSize;
            }
            else
            {
                stride.expected = size;
            }

            /* Wait until some data has arrived */
            wait_recieve_queue();

            /* Attempt to read one stride from recieve queue */
            bool status = ReadFile(
                handle,
                fragment,
                stride.expected,
                &stride.actual,
                NULL
                );

            /* Check if read failed */
            if(!status)
            {
                fatal_error();
                return false;
            }

            /* If any data was read, update the output buffer and remaining read size */
            if(stride.actual)
            {
                for(int fragment_index = 0; fragment_index < stride.actual; fragment_index++)
                {
                    buffer[buffer_index++] = fragment[fragment_index];
                }

                size -= stride.actual;
            }

        } while(size);

        return true;
    }

    /* Write a single character */
    bool write(uint8_t ch)
    {
        uint8_t buffer[1] = {ch};
        return write(buffer, 1);
    }

    /* Read a single character */
    bool read(uint8_t &ch)
    {
        uint8_t buffer[1] = {0};
        return read(buffer, 1);
    }

    bool wait_transmit(void)
    {
        DWORD event_mask = 0;

        if(get_tx_queue_size() == 0)
        {
            printf("tx queue empty");
            return false;
        }

        bool polling = true;
        while(polling)
        {
            bool status = WaitCommEvent(handle, &event_mask, NULL);

            if(!status)
            {
                printf("Error: WaitCommEvent() failed.\n");
                fatal_error();
                return false;
            }

            if(status & EV_TXEMPTY)
            {
                polling = false;
                break;
            }            
        }
        return true;
    }

    /* Wait until the UART has receieved 1+ characters */
    bool wait_recieve_queue(void)
    {
        DWORD event_mask = 0;

        if(get_rx_queue_size() >= 0)
        {
            return true;
        }

        bool polling = true;
        while(polling)
        {
            bool status = WaitCommEvent(handle, &event_mask, NULL);

            if(!status)
            {
                printf("Error: WaitCommEvent() failed.\n");
                fatal_error();
                return false;
            }

            if(status & EV_RXCHAR)
            {
                polling = false;
                break;
            }            
        }
        return true;
    }

    /* Get number of bytes in recieve queue */
    size_t get_rx_queue_size(void)
    {
        DWORD error_flags;
        COMSTAT comm_stat;
        bool status;

        status = ClearCommError(handle, &error_flags, &comm_stat);
        if(status == 0)
        {
            printf("Error: ClearCommError() failed.\n");
            fatal_error();
            return 0;
        }

        return comm_stat.cbInQue;
    }

    /* Get number of bytes in transmit queue */
    size_t get_tx_queue_size(void)
    {
        DWORD error_flags;
        COMSTAT comm_stat;
        bool status;

        status = ClearCommError(handle, &error_flags, &comm_stat);
        if(status == 0)
        {
            printf("Error: ClearCommError() failed.\n");
            fatal_error();
            return 0;
        }

        return comm_stat.cbOutQue;
    }

    bool read_blocking(uint8_t *ch)
    {
        size_t size = get_rx_queue_size();
        while(size <= 0)
        {
            Sleep(100);
        }
        return read((uint8_t *)&ch, 1);
    }

    /* Control DTR pin */
    bool set_dtr(bool enable)
    {
        auto status = EscapeCommFunction(handle, enable ? SETDTR : CLRDTR);
        if(!status)
        {
            fatal_error();
            return false;
        }
        return true;
    }

    /* Control RTS pin */
    bool set_rts(bool enable)
    {
        auto status = EscapeCommFunction(handle, enable ? SETRTS : CLRRTS);
        if(!status)
        {
            fatal_error();
            return false;
        }
        return true;
    }

    HANDLE handle;
    char port_name[256];

private:
    static constexpr int kReadTimeoutMs = 100;
    static constexpr int kWriteTimeoutMs = 100;
    bool opened_ = false;

}; /* end class */



