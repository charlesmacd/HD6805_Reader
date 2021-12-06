
#pragma once

#include "winserial.hpp"

class ArduinoSerialPort : public SerialPort
{
public:
    bool openArduino(int com_port, int com_baud_rate)
    {
        if(com_port == -1)
        {
            if(!scan(&com_port, NULL))
            {
                printf("Status: Couldn't find a COM port.\n");
            }
            else
            {
                printf("Status: Found port COM%d.\n", com_port);
            }
        }

        printf("Status: Attempting to open COM%d.\n", com_port);

        if(!open(com_port, com_baud_rate))
        {
            printf("Error: Couldn't open serial port.\n");
            return false;
        }

        /* Reset Arduino */
        printf("Status: Waiting for Arduino to reboot. \n");
        resetArduino();
        return true;
    }

    bool closeArduino()
    {
        return close();
    }

    /*   
        The DTR# line is AC coupled to the Arduino's ATmega328 RESET# pin.
        When DTR# is pulled low it causes a ~250us low pulse on RESET#,
        the duration of which is not controlled by DTR#
    */
    void resetArduino(uint32_t delay_ms = 2000)
    {
        /* Flush UART recieve buffer to clear out any garbage */
        flush_rx_queue();
        flush_tx_queue();

        /* Drive DTR# low for 1ms, Arduino reset pulse happens within first 250us */
        set_dtr(true);
        Sleep(1);
        set_dtr(false);

        /* Wait for Arduino to boot after reset is relesaed */
        Sleep(delay_ms);
    }
};

