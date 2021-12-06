/*----------------------------------------------------------------------------*/
/* Utility program for HD6805 reader shield  */
/*----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <stdint.h>
#include <windows.h>

#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <list>
#include <functional>

#include "comms.hpp"
#include "utility.hpp"
#include "winserial.hpp"
#include "arduino_serial.hpp"
using namespace std;

#define ASCII_ESC                   0x1B
#define COM_BAUD_RATE               115200

/* Global variables */
Comms comms;
int com_port = -1;
int com_baud_rate = COM_BAUD_RATE;
string app_name;

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

bool cmd_generic_handler(Comms &comms, command_context *p)
{
    if(!comms.port.openArduino(com_port, com_baud_rate))
    {
        printf("Error: Couldn't open serial port.\n");
        return false;
    }

    printf("Sending command.\n");
    if(!comms.send_command(p->command))
    {
        printf("Couldn't send command.\n");
    }
    else
    {
        switch(p->type)
        {
            case CMD_RUN:
                if(p->run)
                {
                    p->run();
                }
                break;

            case CMD_DISPATCH:
                if(!comms.dispatch_target(p->tx_buffer, p->tx_size, p->rx_buffer, p->rx_size, &p->parameters))
                {
                    printf("Error: Aborting command processing\n");
                    exit(1);
                }
                break;

            case CMD_NO_RESPONSE:
            default:
                break;
        }
        printf("Status: Normal exit.\n");
    }
    
    if(!comms.port.close())
    { 
        printf("Error: Couldn't close serial port.\n");
        return false;
    }
    return true;
}



/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

extern vector<vector<Command*>*> app_command_list;

Command def_cmd_nop = {
    .name = "nop",
    .usage = "%s",
    .help = "No operation",
    .parse = [](auto &parser) {
    command_context p;
        p.command = CMD_NOP;
        p.type = CMD_NO_RESPONSE;
        return cmd_generic_handler(comms, &p);
    }
};

Command def_cmd_test = {
    .name = "test",
    .usage = "%s",
    .help = "Test enabling ISP mode and report response",
    .parse = [](auto &parser) {
    command_context p;
        p.command = CMD_TEST;
        p.type = CMD_DISPATCH;
        return cmd_generic_handler(comms, &p);
    }
};

Command def_cmd_status = {
    .name = "status",
    .usage = "%s",
    .help = "Get last programming operation status",
    .parse = [](auto &parser) {
    command_context p;
        p.command = CMD_STATUS;
        p.type = CMD_DISPATCH;
        return cmd_generic_handler(comms, &p);
    }
};

/* Display help */
Command def_cmd_help = {
    .name = "help",
    .usage = "%s",
    .help = "Display help",
    .parse = [](auto &parser) { 
        printf("HD Reader utility\n(C) 2021 Charles MacDonald\nWeb: http://techno-junk.org\n\n");
        printf("usage: %s [--options] command\n", app_name.c_str());
        printf("Available commands are:\n");
        for(auto table : app_command_list)
        {
            for(auto element : *table)
            {
                auto command = *element;
                string fragment = format(command.usage.c_str(), command.name.c_str());
                set_terminal_color(0x0E);
                printf("%10s", command.name.c_str());
                set_terminal_color(0x07);
                set_terminal_color(0x0F);
                printf(" : %s\n", command.help.c_str());
                set_terminal_color(0x07);
                printf("%10s   ", " ");
                set_terminal_color(0x0A);
                printf("Usage: ");
                printf("%s %s", app_name.c_str(), fragment.c_str());
                set_terminal_color(0x07);
                printf("\n");
            }
        }
        return true;
     }
};

/* Scan for available COM ports */
Command def_cmd_scan = {
    .name = "scan",
    .usage = "%s",
    .help = "Scan for available COM ports",
    .parse = [](auto &parser) {
        int value = ListComPort(true);
        if(value != -1)
        {
            printf("Found Arduino at COM%d\n", value);
        }
        return true;
    }
};

/* Read raw test data and decode it as ROM data */
Command def_cmd_read = {
    .name = "read",
    .usage = "%s output.bin",
    .help = "Read HD6809V1 device",
    .parse = [](auto &parser) { 
        string filename;
        command_context p;
        size_t buffer_size;
        uint8_t *buffer;

        /* Get filename */
        if(!parser.next(filename)) {
            printf("Error: No file name specified.\n");
            return false;
        }

        /* Allocate dump output buffer (4K addresses x 4 bytes per address) */
        buffer_size = 4096 * 4;
        buffer = new uint8_t [buffer_size];
        if(!buffer) {
            printf("Error: Couldn't allocate %d bytes.\n", buffer_size);
            return false;
        }

        /* Send parameters for read command */
        p.parameters.push_back(6);
        p.command = CMD_READ;
        p.type = CMD_DISPATCH;
        p.rx_buffer = buffer;
        p.rx_size = buffer_size;

        /* Run command */        
        if(!cmd_generic_handler(comms, &p))
        {
            printf("Error: Failed to run command on target.\n");
            return false;
        }

        /* Checksum data */
        uint8_t checksum = 0x81;
        for(int i = 0x200; i < buffer_size; i++)
        {
            checksum += buffer[i];
        }
        printf("Local checksum = %02X\n", checksum);

        FILE *fd = NULL;
        
        printf("Status: Writing ROM image to file `%s'.\n", filename.c_str());
        fd = fopen(filename.c_str(), "wb");
        if(!fd)
        {
            printf("Error: Can't open file `%s' for writing.\n", filename.c_str());
            delete []buffer;
            return false;
        }
        for(int i = 0; i < buffer_size; i+=4)
        {
            uint8_t ah1 =  buffer[i+0];
            uint8_t adl =  buffer[i+1];
            uint8_t ah2 =  buffer[i+2];
            uint8_t data = buffer[i+3];

            if(i < 0x200)
            {
                data = 0xff; /* Blank out first 128 bytes */
            }
            fputc(data, fd);
        }
        fclose(fd);

        filename += ".log";
        printf("Status: Writing raw test data to `%s'.\n", filename.c_str());
        fd = fopen(filename.c_str(), "wb");    
        if(!fd)
        {
            printf("Error: Can't open file `%s' for writing.\n", filename.c_str());
            delete []buffer;
            return false;
        }
        fwrite(buffer, buffer_size, 1, fd);
        fclose(fd);

        delete []buffer;
        return true;
     }
};

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

/* Diagnostic: Echo test */
Command def_cmd_echo = {
    .name = "echo",
    .usage = "%s",
    .help = "Run echo loopback test",
    .parse = [](auto &parser) { 
        auto echo_runner = [](void) -> bool {
            printf("Status: Echo test. Press any key to exit.\n");
            bool running = true;
            while(running)
            {
                if(_kbhit())
                {
                    uint8_t ch = getch();
                    printf("Got user key input: %02X\n", ch);
                    printf("Sending input\n");
                    comms.sendb(ch);
                    printf("Polling for response\n");
                    uint8_t response = comms.getb();
                    printf("Got Arduino response %02X\n", response);

                    if(ch == ASCII_ESC)
                    {
                        printf("User requested break.\n");
                        running = false;
                        return true;
                    }
                }
            }
            return false;
        };    

        command_context p;
        p.command = CMD_ECHO;
        p.type = CMD_RUN;
        p.run = echo_runner;
        if(!cmd_generic_handler(comms, &p))
        {
            printf("Error: Failed to run command on target.\n");
        }
        return true;
     }
};

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

/* Option: Specify COM port */
Command def_opt_port = {
    .name = "--port",
    .usage = "%s number",
    .help = "Specify COM port to use",
    .parse = [](auto &parser) { 
        string parameter;
        if(!parser.next(parameter)) {
            printf("Error: Missing argument.\n");
            return false;
        }
        com_port = atoi(parameter.c_str());
        printf("Status: Using serial port COM%d\n", com_port);
        return true;
     }
};

/* Option: Specify baud rate */
Command def_opt_baudrate = {
    .name = "--baudrate",
    .usage = "%s rate (default 115200 bps)",
    .help = "Specify baud rate",
    .parse = [](auto &parser) { 
        string parameter;
        if(!parser.next(parameter)) {
            printf("Error: Missing argument.\n");
            return false;
        }
        com_baud_rate = atoi(parameter.c_str());
        printf("Status: Using baud rate of %d\n", com_baud_rate);
        return true;
     }
};

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

/* Options */
vector<Command*> sub_option_list = { 
    &def_opt_port,
    &def_opt_baudrate
};

/* Commands */
vector<Command*> sub_command_list = {
    // Application
    &def_cmd_help,
    &def_cmd_scan,    

    // Diagnostic
    &def_cmd_nop,
    &def_cmd_echo,

    // Device
    &def_cmd_read, 
};

/* All supported commands and options */
vector<vector<Command*>*> app_command_list = {
    &sub_option_list,
    &sub_command_list
};

/* Scan token list for matching keywords and run associated command */
int parse_commands(vector<Command *> table, list<string> &tokens, bool retry)
{
    constexpr bool verbose = true;
    auto token_it = tokens.begin();
    while(token_it != tokens.end())
    {        
        bool matched = false;
        auto token = *token_it;

        for(auto &opt : table)
        {
            if(token == opt->name)
            {
                /* Parse arguments to token */
                auto start = token_it;
                //auto end = opt->parse(tokens, ++token_it);

                auto end = opt->exec(tokens, ++token_it);

                /* Erase what we used */
                token_it = tokens.erase(start, end);

                /* Stop processing at the first valid command we parsed */
                if(!retry)
                {
                    return tokens.size();
                }

                /* Restart parsing */
                matched = true;
                break;
            }
        }

        /* Advance to next token if we couldn't match and there's more left */
        if(!matched && token_it != tokens.end())
        {
            ++token_it;
        }
    }    
    return tokens.size();
}

int parse_args(int argc, char *argv[])
{
    /* Convert argv to list of tokens */
    list<string> tokens;
    for(int i = 1; i < argc; i++) {
        tokens.push_back(argv[i]);
    }

    if(argc == 1)
    {
        tokens.push_back("help");
    }

    /* Parse options and one command */
    parse_commands(sub_option_list, tokens, true);
    parse_commands(sub_command_list, tokens, false);

    /* Warn user of input we couldn't parse */
    if(tokens.size())
    {
        printf("Unrecognized input: ");
        for(const auto &token : tokens)
            printf("%s ", token.c_str());
        printf("\n");
    }

    return 0;
}

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

int main (int argc, char *argv[])
{
    app_name = argv[0];
    return parse_args(argc, argv);
}

/* End */