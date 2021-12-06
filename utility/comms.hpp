
#pragma once

#include "winserial.hpp"
#include "arduino_serial.hpp"

#define TEXT_COLOR_NORMAL       0x07
#define TEXT_COLOR_TARGET       0x0A

class command_context
{
public:
    /* Command to run on target */
    int command;

    /* Command dispatch type */
    int type;

    /* Parameters send to target associated with command */
    vector<uint8_t> parameters;

    /* Function to run in parallel with the target */
    bool (*run)(void);

    /* Bulk data send to the target */
    uint8_t *tx_buffer;
    size_t tx_size;

    /* Bulk data read back from the target */
    uint8_t *rx_buffer;
    size_t rx_size;

    command_context()
    {
        tx_buffer = nullptr;
        tx_size = 0;
        rx_buffer = nullptr;
        rx_size = 0;
        parameters.clear();
        run = nullptr;
        command = 0;
        type = 0;
    }    
};


/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

class Parser
{
public:
    Parser(list<string> &tokens, list<string>::iterator &token_it) : tokens_(tokens), token_it_(token_it) {}
    bool next(string &token) {
        if(token_it_ == tokens_.end()) {
            return false;
        }
        token = *token_it_++;
        return true;
    }    
    list<string> &tokens_;
    list<string>::iterator &token_it_;
};

typedef std::function<bool(Parser &parser)> parse_func;

typedef struct {
    string name;
    string usage;
    string help;
    vector<string> arguments;
    const parse_func parse;

    list<string>::iterator exec(list<string> &tokens, list<string>::iterator &token_it)
    {
        Parser parser(tokens, token_it);
        if(!this->parse(parser))
        {
            printf("Error parsing or running command.\n");
        }
        return parser.token_it_;
    }
} Command;

/* Command response */
#define COMMS_ACK       0xA5
#define COMMS_NACK      0xAA

/* Sub commands we dispatch for the target */
enum {
    SUB_CMD_PASS          =   0x20,
    SUB_CMD_FAIL,
    SUB_CMD_LOG,
    SUB_CMD_SYNC,  
    SUB_CMD_EXIT,
    SUB_CMD_GET_PAGE,
    SUB_CMD_SEND_PAGE,
    SUB_CMD_GET_PARAMETERS,
};

/* Commands we dispatch on the target */
enum {
    CMD_NOP             =   0x00,
    CMD_ECHO            =   0x01,
    CMD_BLINK           =   0x02,
    CMD_WRITE           =   0x20, // write
    CMD_READ            =   0x21, // read
    CMD_STATUS          =   0x22, // read status 
    CMD_ERASE           =   0x23, // erase
    CMD_FUSE            =   0x24, // program fuses
    CMD_MODE            =   0x26, // set running mode -- special case?
    CMD_TEST            =   0x27, // ?
};

enum {
    CMD_NO_RESPONSE,        /* The command returns no information */
    CMD_DISPATCH,           /* The command requires sub dispatch */
    CMD_RUN,                /* The command runs repeatedly */
};

class Comms
{
public:
    const size_t page_size = 0x40;

    uint8_t getb(void)
    {
        uint8_t input = 0;
        while(port.get_rx_queue_size() <= 0)
        {
            Sleep(1);
        }
        port.read(&input, 1);            
        return input;
    }

    void sendb(uint8_t value)
    {
        port.write(value);
    }

    bool send_command(uint8_t command)
    {
        uint8_t response[2];
        sendb(command);

        response[0] = getb();
        response[1] = getb();

        if(response[0] != COMMS_ACK || response[1] != command)
        {
            printf("Error: Command handshake failed (%02X, %02X).\n", response[0], response[1]);
            return false;
        }

        return true;
    }

    bool dispatch_target(uint8_t *tx_buffer, size_t tx_size, uint8_t *rx_buffer, size_t rx_size, vector<uint8_t> *parameters)
    {
        uint32_t tx_offset = 0;
        uint32_t rx_offset = 0;
        bool processing = true;
        static uint8_t last_command = 0xff;

        while(processing)
        {
            uint8_t command = getb();
            switch(command)
            {
                case SUB_CMD_SYNC:
                    // Synchronize with host
                    break;

                case SUB_CMD_EXIT:
                    // Exit dispatch
                    processing = false;
                    break;

                case SUB_CMD_PASS:
                    // Command passed
                    break;

                case SUB_CMD_FAIL:            
                    // Command failed
                    return false;
                    break;

                case SUB_CMD_GET_PARAMETERS:
                    {
                        sendb(parameters->size());
                        for(int i = 0; i < parameters->size(); i++)
                        {
                            sendb(parameters->data()[i]);
                        }
                    }
                    break;

                case SUB_CMD_GET_PAGE:
                    {
                        for(int chunk = 0; chunk < 4; chunk++)
                        {
                            uint8_t checksum = 0x81;
                            for(int j = 0; j < 16; j++)
                            {
                                size_t offset = (tx_offset) + (chunk * 0x10) + j;
                                uint8_t datum = tx_buffer[offset];
                                checksum += datum;
                                sendb(datum);
                            }
                            uint8_t result = getb();
                            printf("Got back %02X, local %02X\n", result, checksum);
                        }
                        tx_buffer += 0x40;
                    }
                    break;

                case SUB_CMD_SEND_PAGE: 
//                    printf("Getting page from target (%08X).\n", rx_offset);
                    port.read(&rx_buffer[rx_offset], page_size);           
                    rx_offset += page_size;
                    break;

                case SUB_CMD_LOG:
                    {
                        uint8_t length = getb();
                        char buffer[256];
                        memset(buffer, 0, sizeof(buffer));
                        for(int i = 0; i < length; i++)
                        {
                            buffer[i] = getb();
                        }
                        buffer[255] = 0;

                        set_terminal_color(TEXT_COLOR_TARGET);
                        printf("%s", buffer);
                        set_terminal_color(TEXT_COLOR_NORMAL);
                    }
                    break;

                default:
                    break;
            }
            last_command = command;
        }
        return true;
    }

    ArduinoSerialPort port;
};