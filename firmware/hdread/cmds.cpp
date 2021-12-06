
#include <stdint.h>
#include <Arduino.h>
#include "cmds.hpp"
#include "comms.hpp"
#include "target.hpp"
#include "board.hpp"

constexpr size_t kMaxStates = 4;
target_state_t state[kMaxStates];
uint32_t cycles = 0;
uint32_t bus_cycle = 0;

/*-----------------------------------------------------------*/
/* Utility functions */
/*-----------------------------------------------------------*/

uint8_t shuffle(uint8_t in)
{
  constexpr uint8_t order[] = {0, 2, 6, 7, 5, 4, 3, 1};
  uint8_t temp = 0;

  for(uint8_t index = 0; index < 8; index++) {
    if(in & (1 << index)) temp |= (1 << order[index]);
  }
  return temp;
}

uint8_t nbit(uint8_t value)
{
  int count = 0;
  for(int i = 0; i < 8; i++)
  {
    if(value & (1 << i))
    {
      count++;
    }
  }
  return count;
}


/*-----------------------------------------------------------*/
/*-----------------------------------------------------------*/

int cmd_echo(void)
{
  comms_acknowledge_command(CMD_ECHO);
  bool polling = true;
  while (polling)
  {
    uint8_t value = comms_getb();
    comms_sendb(value);
    if (value == ASCII_ESC)
    {
      polling = false;
      break;
    }
  }
  return 0;
}


void cmd_nop(void)
{
  comms_acknowledge_command(CMD_NOP);
}


/*-----------------------------------------------------------*/
/*-----------------------------------------------------------*/

void run_read()
{
  cycles = 0; 
  bus_cycle = 0;

  /* Reset target */
  comms_printf("Status: Resetting target.\n");
  reset_target();
  
  /* Clock device until test mode starts */
  do {
    clock_target(2);
    get_target_state(&state[0]);
  } while(state[0].strobe == 0);

  comms_printf("Dumping startup\n");
  for(int i = 0; i < 4; i++) // cycle 3/3 of startup is read from 469d, maybe that's real start?
  { 
    clock_target(2);
    get_target_state(&state[1]);

    uint8_t adh = state[0].ah;
    uint8_t adl = shuffle(state[0].adl);
    uint8_t data = state[1].adl;

    comms_printf("%02X%02X [%02X] : %02X [%02X]\n",
      adh, adl, state[0].adl, data, shuffle(data)
      );
    
    // Next
    clock_target(2);
  }

  uint32_t i;
  uint16_t bus_address[2];
  uint8_t bus_data[2];

  int last_adh = -1;
  int k = 0;

  comms_printf("Dumping output\n");
  uint8_t checksum;

  for(int32_t i = 0; i < 0x10000; i++)
  {
    if((i & 0x00ff) == 0)
    {
      comms_printf("Cycle %04X\n", i);
    }
    
    get_target_state(&state[i & 3]);
    clock_target(2);
    
    if((i & 0x03) == 0x03)
    {
      bus_address[0] = state[0].ah << 8 | state[0].adl;
      bus_data[0] = state[1].adl;
      
      bus_address[1] = state[2].ah << 8 | state[2].adl;
      bus_data[1] = state[3].adl;

      checksum += bus_data[0];

      

      if( (bus_address[0] == 0x40) || (bus_address[0] != bus_address[1] || bus_data[0] != bus_data[1]))
      {
        comms_printf("MISMATCH : ");
        comms_printf("%08X :", i); 
        comms_printf("%04X = ", bus_address[0]); 
        comms_printf("%02X | ", bus_data[0]); 
        comms_printf("%04X = ", bus_address[1]); 
        comms_printf("%02X\n", bus_data[1]); 
      }
      
    }
  }
 
  comms_printf("Ready\n");
}

/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/

void read_raw_cycles(void)
{
  int cycles = 0;
  bool scanning = true;
  uint16_t bus_address[2];
  uint8_t bus_data[2];

  // ends with 8 cycles of clock going low high
  reset_target();

  comms_printf("START\n");

  while(scanning)
  {
    clock_target(1);
    get_target_state(&state[0]);
    if(scanning && (state[0].strobe == 0 || state[0].ah != 0x0F || state[0].adl != 0xFE))
    {
      cycles++;
      continue;
    }
    scanning = false;
  }

  comms_printf("Started output sequence in %02X cycles.\n", cycles);  
  for(int i = 0; i < 0x18; i++)
  {
    bus_address[0] = state[0].ah << 8 | state[0].adl;      
    comms_printf("%08X : TEST=%d | NUM=%d | AH:%02X ADL:%02X\n",
      i,
      state[0].strobe,
      state[0].num,
      state[0].ah,
      state[0].adl
      );
      
    clock_target(1);
    get_target_state(&state[0]);
  }
  comms_printf("END\n");
}

/*-----------------------------------------------------------*/
/*-----------------------------------------------------------*/
/*-----------------------------------------------------------*/


void validate_adl(void)
{
  int cycles = 0;
  bool scanning = true;
  uint16_t bus_address[2];
  uint8_t bus_data[2];

  // ends with 8 cycles of clock going low high
  reset_target();

  comms_printf("START\n");

  while(scanning)
  {
    clock_target(1);
    get_target_state(&state[0]);
    if(scanning && (state[0].strobe == 0 || state[0].ah != 0x0F || state[0].adl != 0xFE))
    {
      cycles++;
      continue;
    }
    scanning = false;
  }

  comms_printf("Started boot sequence in %02X cycles.\n", cycles);  
  for(int i = 0; i < 4; i++)
  {
    comms_printf("%02X ", state[0].adl);
      
    clock_target(4);
    get_target_state(&state[0]);
  }

  // scan for zero
  cycles = 0;
  comms_printf("Scanning for address zero.\n");
  for(int i = 0; i < 0x10000; i++)
  {
    if(state[0].adl == 0x00 && state[0].ah == 0x00)
      break;
    clock_target(8);
    get_target_state(&state[0]);
    ++cycles;
  }
  clock_target(4);
  comms_printf("Found zero in %08X cycles.\n", cycles);
  

  comms_printf("Started output sequence:\n");  
  for(int i = 0; i < 0x100; i++)
  {
    if((i & 0x0F) == 0x00)
    {
      comms_printf("%08X : %04X: ", i, (i & 0xff) | state[0].ah << 8);
    }

    comms_printf("%02X ", state[0].adl);
      
    clock_target(4);
    get_target_state(&state[0]);

    // skip vpph cycle
    clock_target(4);

    if((i & 0x0F) == 0x0F)
    {
      comms_printf("\n");
    }
  }
  comms_printf("END\n");
}


void validate_adl_1bit(void)
{
  int cycles = 0;
  bool scanning = true;
  uint16_t bus_address[2];
  uint8_t bus_data[2];

  // ends with 8 cycles of clock going low high
  reset_target();

  comms_printf("START\n");

  while(scanning)
  {
    clock_target(1);
    get_target_state(&state[0]);
    if(scanning && (state[0].strobe == 0 || state[0].ah != 0x0F || state[0].adl != 0xFE))
    {
      cycles++;
      continue;
    }
    scanning = false;
  }

  comms_printf("Started boot sequence in %02X cycles.\n", cycles);  
  for(int i = 0; i < 4; i++)
  {
    comms_printf("%02X ", state[0].adl);
      
    clock_target(4);
    get_target_state(&state[0]);
  }

  // scan for zero
  cycles = 0;
  comms_printf("Scanning for address zero.\n");
  for(int i = 0; i < 0x10000; i++)
  {
    if(state[0].adl == 0x00 && state[0].ah == 0x00)
      break;
    clock_target(8);
    get_target_state(&state[0]);
    ++cycles;
  }
  clock_target(4);
  comms_printf("Found zero in %08X cycles.\n", cycles);


  clock_target(4);
    get_target_state(&state[0]);
  
  
  //////////////////////////////
  // scan for zero
  cycles = 0;
  int pf=0;
  comms_printf("Scanning for address max.\n");
  for(int i = 0; i < 0x20000; i++)
  {
    if(++pf==0x800)
    {
      comms_printf("seg=%08X\n", i);
      pf=0;
    }
    if(state[0].adl == 0xff && state[0].ah == 0x0F)
      break;
    clock_target(8);
    get_target_state(&state[0]);
    ++cycles;
  }
  clock_target(4);
  comms_printf("Found max in %08X cycles.\n", cycles);
  
  //////////////////////////////

  comms_printf("Started output sequence:\n");  
  for(int i = 0; i < 0x100; i++)
  {
    if((i & 0x0F) == 0x00)
    {
      comms_printf("%08X : %04X: ", i, (i & 0xff) | state[0].ah << 8);
    }

    comms_printf("%02X ", state[0].adl);
      
    clock_target(4);
    get_target_state(&state[0]);

    // skip vpph cycle
    clock_target(4);

    if((i & 0x0F) == 0x0F)
    {
      comms_printf("\n");
    }
  }
  comms_printf("END\n");
}


uint32_t seek_bus_cycle(uint16_t address, bool debug)
{
  uint32_t cycles_elapsed = 0;
  bool seeking = true;

  uint8_t adh = (address >> 8) & 0xFF;
  uint8_t adl = (address >> 0) & 0xFF;

  while(seeking) {
    get_target_state(&state[0]);
    if(state[0].strobe == 1 && state[0].num == 0 && state[0].ah == adh && state[0].adl == adl)
    {
      break;
    }
    if(debug)
    {
      comms_printf("AH=%02X ADL=%02X NUM=%d TEST=%d\n", 
        state[0].ah, state[0].adl, state[0].num, state[0].strobe);
    }
    clock_target(1);
    ++cycles_elapsed;
  }

  return cycles_elapsed;
}


void test_address_wrapping(void)
{
  uint32_t cycles; 
  comms_printf("Status: Test address wrapping.\n");
  reset_target();

  // Find first bus cycle of reset vector fetch (reading vector LSB at 0x0FFE)
  comms_printf("Status: Seek first bus cycle.\n");
  cycles = seek_bus_cycle(0x0FFE, false);  
  comms_printf("Result: Found first bus cycle in %d clocks.\n", cycles);

  // Seek past startup sequence to beginning of output sequence
  comms_printf("Status: Seek first bus cycle.\n");
  cycles = seek_bus_cycle(0x0EEA, false);  
  comms_printf("Result: Found output sequence in %d clocks.\n", cycles);

  // In output sequence, find when address counter resets
  comms_printf("Status: Seek zero bus cycle.\n");
  cycles = seek_bus_cycle(0x0000, false);  
  comms_printf("Result: Found address wrap in %d clocks.\n", cycles);
  
  // In output sequence, find when address counter resets again
  comms_printf("Status: Seek zero bus cycle.\n");
  cycles = seek_bus_cycle(0x0FFF, false);  
  comms_printf("Result: Found address wrap-1 in %d clocks.\n", cycles);

  // So it takes 32768 clocks to wrap, which is 32K/8 = 4096 NUM cycles
  comms_printf("Status: Finished.\n");
}

/*-----------------------------------------------------------*/
/*-----------------------------------------------------------*/
/*-----------------------------------------------------------*/



void test_address_output(bool dump)
{
  uint32_t cycles; 
  comms_printf("Status: Test address wrapping.\n");
  reset_target();

  // Find first bus cycle of reset vector fetch (reading vector LSB at 0x0FFE)
  comms_printf("Status: Seek first bus cycle.\n");
  cycles = seek_bus_cycle(0x0FFE, false);  
  comms_printf("Result: Found first bus cycle in %d clocks.\n", cycles);

  // Seek past startup sequence to beginning of output sequence
  comms_printf("Status: Seek first bus cycle.\n");
  cycles = seek_bus_cycle(0x0EEA, false);
  comms_printf("Result: Found output sequence in %d clocks.\n", cycles);

  // In output sequence, find when address counter resets
  comms_printf("Status: Seek zero bus cycle.\n");
  cycles = seek_bus_cycle(0x0000, false);  
  comms_printf("Result: Found address wrap in %d clocks.\n", cycles);

  int write_count = 0;
  uint32_t cycles_elapsed = 0;
  bool seeking = true;
  uint8_t latched_ah = -1;

  while(seeking) 
  {
    get_target_state(&state[0]);

    if(state[0].num == 0)
    {
      if(dump)
      {
        comms_printf("%04X,", state[0].ah << 8 | state[0].adl);
      }
      else
      {
        if((write_count & 0x0F) == 0x00)
        {
          comms_printf("%04X: ", cycles_elapsed);
          comms_printf("%02X: ", state[0].ah);
          latched_ah = state[0].ah;
        }
    
        comms_printf("%02X%c", state[0].adl, 
          (latched_ah != state[0].ah) ? '*' : ' ');
   
        if((write_count & 0x0F) == 0x0F)
        {
          comms_printf("\n");
        }
      
        ++write_count;
      }
    }

    // Stop scanning if we get to the last address
    if(state[0].strobe == 1 && state[0].num == 0 && state[0].ah == 0x0F && state[0].adl == 0xFF)
    {
      break;
    }
    
    clock_target(8);
    ++cycles_elapsed;
  }
  comms_printf("\n");
  
  // In output sequence, find when address counter resets again
  
  comms_printf("Status: Seek zero bus cycle.\n");
  cycles = seek_bus_cycle(0x0FFF, false);  
  comms_printf("Result: Found address wrap-1 in %d clocks.\n", cycles);

  // So it takes 32768 clocks to wrap, which is 32K/8 = 4096 NUM cycles
  comms_printf("Status: Finished.\n");
}


/*-----------------------------------------------------------*/
/*-----------------------------------------------------------*/

void test_dump(bool dump)
{
  uint32_t cycles; 
  comms_printf("Status: Test address wrapping.\n");
  reset_target();

  // Find first bus cycle of reset vector fetch (reading vector LSB at 0x0FFE)
  comms_printf("Status: Seek first bus cycle.\n");
  cycles = seek_bus_cycle(0x0FFE, false);  
  comms_printf("Result: Found first bus cycle in %d clocks.\n", cycles);

  // Seek past startup sequence to beginning of output sequence
  comms_printf("Status: Seek first bus cycle.\n");
  cycles = seek_bus_cycle(0x0EEA, false);  
  comms_printf("Result: Found output sequence in %d clocks.\n", cycles);

  // In output sequence, find when address counter resets
  comms_printf("Status: Seek zero bus cycle.\n");
  cycles = seek_bus_cycle(0x0000, false);  
  comms_printf("Result: Found address wrap in %d clocks.\n", cycles);

  // Dump address data


  int write_count = 0;
  uint32_t cycles_elapsed = 0;
  
  for(int cycles_elapsed = 0; cycles_elapsed < 4096; cycles_elapsed++)
  {
    get_target_state(&state[0]);
    clock_target(2);
    
    get_target_state(&state[1]);
    clock_target(2);

    get_target_state(&state[2]);
    clock_target(2);
    
    get_target_state(&state[3]);
    clock_target(2);

    comms_printf("%02X", state[0].ah);
    comms_printf("%02X", state[0].adl);
    comms_printf("%02X", state[1].adl);
    comms_printf("%02X,", state[3].adl);

}
  comms_printf("\n");
  
  
  // In output sequence, find when address counter resets again
  comms_printf("Status: Seek zero bus cycle1.\n");
  cycles = seek_bus_cycle(0x0FFF, false);  
  comms_printf("Result: Found address wrap-1 in %d clocks.\n", cycles);

  // So it takes 32768 clocks to wrap, which is 32K/8 = 4096 NUM cycles
  comms_printf("Status: Finished.\n");
}

/*-----------------------------------------------------------*/
/*-----------------------------------------------------------*/

void free_run(void)
{
  comms_sendb(SUB_CMD_EXIT);    
  while(1)
  {
    reset_target();
    for(int i = 0; i < 192; i++)
    {
      clock_target(1);
    }
  }
}

/*-----------------------------------------------------------*/
/*-----------------------------------------------------------*/

void binary_dump(bool dump)
{
  constexpr uint16_t startAddress1 = 0xFFE;
  constexpr uint16_t startAddress2 = 0xEEA;
  
  uint32_t cycles; 
  comms_printf("Status: Test address wrapping.\n");
  reset_target();

  comms_printf("Status: Seek first bus cycle.\n");
  cycles = seek_bus_cycle(startAddress1, false);  
  comms_printf("Result: Found first bus cycle in %d clocks.\n", cycles);

  comms_printf("Status: Seek first bus cycle.\n");
  cycles = seek_bus_cycle(startAddress2, false);  
  comms_printf("Result: Found output sequence in %d clocks.\n", cycles);

  comms_printf("Status: Seek zero bus cycle.\n");
  cycles = seek_bus_cycle(0x0000, false);  
  comms_printf("Result: Found address wrap in %d clocks.\n", cycles);

  size_t index = 0;
  uint32_t address = 0;
  uint8_t checksum = kChecksumInit;
  uint16_t last = -1;
  
  for(int address = 0; address < kMemorySize; address++)
  {
    uint16_t next = address;
    constexpr uint16_t mask = 0x0100;
    if((last & mask) != (next & mask))
    {
      comms_printf("Reading offset %04X\n", next);
      last = next;
    }

    for(int i = 0; i < 4; i++)
    {
      get_target_state(&state[i]);
      clock_target(2);
    }

    page_buffer[index + 0] = state[0].ah;
    page_buffer[index + 1] = state[0].adl;
    page_buffer[index + 2] = state[1].ah;
    page_buffer[index + 3] = state[3].adl;

    // Don't checksum first 128 bytes as this is RAM, I/O, and unallocated memory locations
    if(address >= kRiotSize)
    {
      for(int i = 0; i < 4; i++) {
        checksum += page_buffer[index + i];
      }
    }    
    index += 4;

    if(index >= kPageSize)
    {
      comms_sendb(SUB_CMD_SEND_PAGE);
      comms_send(page_buffer, kPageSize);
      index -= kPageSize;
    }
  }
  comms_printf("Checksum = %02X\n", checksum);
  comms_printf("Status: Finished.\n");
}


void cmd_read(void)
{
  comms_acknowledge_command(CMD_READ);
  comms_get_parameters(parameters);  
  comms_printf("Got command parameter = %02X\n", parameters[0]);

  switch(parameters[0])
  {
    case 0x00:
      read_raw_cycles();
      break;

    case 0x01:
      validate_adl_1bit();
      break;

    case 0x02:
      test_address_wrapping();
      break;

    case 0x03:
      test_address_output(true);
      break;
      
    case 0x04:
      test_address_output(false);
      break;

    case 0x05:
      test_dump(false);
      break;

    case 0x06:
      binary_dump(false);
      break;

    case 0x07:
      free_run();
      break;
      
    default:
      comms_printf("Unknown parameter value %02X\n", parameters[0]);
      break;
  }
  
  comms_printf("Normal exit.\n");
  comms_sendb(SUB_CMD_EXIT);    
}


void comms_dispatch(void)
{
  uint8_t command = comms_getb();
  switch (command)
  {
    /* Windows sends 0xF0-0xFF when opening a COM port
       Here we just reject anything with bit 7 set.
    */
    case 0x80 ... 0xFF:
      break;

    case CMD_NOP:
      cmd_nop();
      break;

    case CMD_ECHO:
      cmd_echo();
      break;


    case CMD_READ:
      cmd_read();
      break;

    default:
      comms_sendb(COMMS_NACK);
      comms_sendb(command);
      break;
  }
}

/* End */
