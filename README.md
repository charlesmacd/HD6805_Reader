# HD6805_Reader
(C) 2021 Charles MacDonald

WWW: http://techno-junk.org


Arduino shield used to read the internal ROM of some Hitachi HD6805 devices.

Based on the information and research of Sean Riddle (https://seanriddle.com/mc6805p2.html)

**NEW**: Check out the Dumping Log in the Wiki: https://github.com/charlesmacd/HD6805_Reader/wiki
 
## Overview

This project came about from the need to read the HD6805V1 MCU used in Sega's SP-400 printer.

## Files

```
/windows/hdread.exe
	Utilty to talk to Arduino via USB and retrieve HD6805 data
/utility 
	Utility source code
/firmware
	Arduino Uno firmware
/pcb/v1 
        Prototype shield
	Eagle schematics and layout for the first version of the shield. 
	Needs some wire links and a capacitor marked in yellow.
/pcb/v2
        Revised version of shield
	Eagle schematics and layout for the second version, fixes all the issues in v1
/pcb/gerbers
	Gerber files. These were set up using design rules for Elecrow/JLCPCB but 
	you definitely want to generate your own gerber	files for the fab you use.
/images
	Pictures of the shield
/datasheets
	Datasheets, patents, and other relevant material
```

## How to use

1. Insert the HD6805V1 device into the DIP socket with the pin 1 indicator facing toward the Arduino UNO's USB port.
2. Plug the Arduino Uno's USB cable into your PC.
3. Run the utility program: ```hdread read dump.bin```. It gives the following output

```
Status: Found port COM8.
Status: Attempting to open COM8.
Status: Waiting for Arduino to reboot.
Sending command.
Got command parameter = 06
Status: Test address wrapping.
Status: Seek first bus cycle.
Result: Found first bus cycle in 19 clocks.
Status: Seek first bus cycle.
Result: Found output sequence in 16 clocks.
Status: Seek zero bus cycle.
Result: Found address wrap in 4880 clocks.
Reading offset 0000
:
Reading offset 0F00
Checksum = 78
Status: Finished.
Normal exit.
Status: Normal exit.
Local checksum = 78
Status: Writing ROM image to file `dump.bin'.
Status: Writing decoded data to `dump.bin.log'.  
```

4. The ROM data is written to ```dump.bin```, and the raw data output for each NUM cycle is written to ```dump.bin.log```. The latter isn't necessary but can be useful for examining the reading sequence in more detail.

Note that while the entire 4K address space is dumped ROM data only corresponds to offsets 0x0080 to 0xFFF. The first 128 bytes are mapped to internal I/O, RAM, and unused areas and will not be consistent from dump to dump. The utility program blanks them out to 0xFF, but doesn't do that to the log file.

## Analyzing dumped data

You can run ```hdread check <file.bin>``` to analyze the self-check ROM data, if present. For a HD6805V1 it reports the following:

```
Self-check vectors:
* TIMER = $0FEA
* INT#  = $0FEC
* SWI   = $0FE7
* RES#  = $0F80
Customer vectors:
* TIMER = $0A46
* INT#  = $0A10
* SWI   = $07F0
* RES#  = $06E0
Self-check ROM analysis:
* Reset vector is valid.
* Internal checksum   = EA
* Calculated checksum = EA
* ROM SHA256 = 9088fee917e5a748c2f0b4f5458c1cbdabbd696291c69be6e605bae0ef779e8f
* ROM matches device type HD6805V1
```

The internal checksum is stored within the self-check ROM at address 0xFEF. It is computed by calculating the exclusive OR of all bytes from 0x080 to 0xFFF excluding 0xFEF and inverting the result. If the internal and calculated checksums don't match it's likely the ROM dump is bad and should be redumped.

The sha256sum covers addresses 0xF80 through 0xFE7. Two devices I dumped have identical programs, but it's possible other HD6805 variants have different
self-check programs. Please get in touch if you find a device with a different sha256sum.

## Board assembly and configuration

The original design had a TPS2041B to control power to the HD6805V1 in case there was any behavior worth examining that was tied to a power-up event, but in practice that isn't necessary. Thus components IC1, C1A, C1B, C2A, C2B, R8, R10, R24 and the yellow LED can be omitted.

### Board types

The V1 design was the first one I made and it works fine but needs some corrections:

* Wire links added from C0-C4 and NUM to Arduino inputs A0-A4 and A5.
* A 0.1uF capacitor across DIP socket pins 1,4.

The V2 design has these changes made and the schematic was cleaned up considerably so I'd advise building that one instead.
I haven't tested it myself but it was a minor set of changes.

### Bill of materials

| Location | Size | Type | Manufacturer P/N |
|- |- |- |- |
| C1A,C2A | 0603 | Ceramic, 0.1uF, 10V | C0603C104Z3VACTU |
 |C1B,C2B | 0603 | Ceramic, 0.01uF, 10V | 06035C103KAT2A |
| R1-R8/R11-R15/R16-R24 | 0603 | 1K ohm, 1/10W, any tolerance | CRCW06031K00FKEAC |
| R9/R10 | 0603 | 10K ohm, any tolerance | SFR03EZPF1002 |
| FLT | 3mm | Yellow LED, diffused | TLHY4400-AS12Z |
| IC1 | SOIC-8 | TPS2041B load switch | TPS2041BD |
| JP3/4/6/7 | N/A | Arduino UNO stacking headers | Adafruit P/N: 485 or Mouser P/N: 485-85 |

### Jumper configurations

See the photo for more details.

* For JP11 pins 2-3 should be shorted together so that the Arduino 5V is used for the HD6805V1 +5V input.

* For JP5 the jumper across all pins should be installed. Note on the V1 board the middle jumper (XTL) should be left open.

* For JP9-10 the jumpers should be arranged to specify 0x9D (from top to bottom is MSB to LSB, left=VCC, right=GND so jumper order is LRRLLLRL)

## Tested devices

1. ```Hitachi HD6805V1P A08```
   Dump seems good. Internal checksum is 0xEA.

2. ```Termbray 117-017 HD6805V1B65P 8A3```
   Dump seems good. Internal checksum is 0x8A. Same self-check ROM content as above.

3. ```Hitachi HD6805V1A81P 800-5 A24```

   Dump seems good. This is a variant where the self-check ROM is not present and instead contains more customer ROM code and data.
   Thus the self-check mode is not supported and only the user vector table is present.

Note that all the chips run a somewhat hot normally.
The Arduino Uno I tested with draws about 40mA idle, and with a HD6805V1 installed it draws 120mA when the clock is static.
But use caution when testing the PCB after assembly and check all voltages **before** inserting a chip and attempting to read it.

## Technical overview

The HD6805V1 is a variant of the Motorola MC6805 designed by Hitachi. Check out the datasheet for more information.

### Memory map
```
0000-0009 : Internal registers (I/O, timer)
000A-001F : Unused
0020-007F : Work RAM (96 bytes)
0080-0F7F : Customer ROM (3848 bytes)
0F80-0FEF : Self-check ROM (120 bytes)
0FF0-0FF7 : Self-check ROM vector table
```

## Operating modes
* Normal
  * Customer ROM is used
* Self-check
  * Self-test ROM is used
  * Enable by pulling TIMER pin to +9V 
* Non-user mode
  * External ROM is used
  * Enable by pulling NUM pin to +5V

## Non-user mode

In non-user mode the I/O ports are repurposed to present an external memory bus similar to the MC146805E2:

* Port A is the external data bus where opcodes are input.
* Port B is multiplexed between part of the address bus and the internal data bus.
* Port C outputs several address lines on PC[4,2:0] and a control signal, possibly R/W# on PC3.

This arrangement would allow the HD6805V1 to execute program code from an external EPROM. A slight difference between it and the MC146805E2 is how the address bus is arranged:

* PB[7:0] => {A9, A8, A5, A5, A6, A1, A7, A0}
* PC[4,2:0] => {A11, A3, A2, A10}

What's missing is a signal used to demultiplex port B. Hitachi's US patent 5,179,694 describes a function for the HD6301V microcontroller where the E clock (a synchronization signal that runs at 1/4th of the oscillator clock) isn't normally accessible but can be output in some conditions. It follows with a brief note that the HD6805 NUM pin performs a similar function when it is pulled high, outputting the sync signal for device testing purposes.

Tests on the HD6805V1 confirms this; in non-user mode the NUM pin will output a clock signal at 1/4th of the rate presented on the EXTAL pin. Conceptually this seems to map closest to the MC146805E2's DS signal. This can be used to demultiplex port B.

## Device configuration in NUM mode for dumping internal ROM

For dumping ROM data I configured the pins as follows on the Arduino shield:

* RES# : Pulled high via 1K ohm resistor, can be driven low to reset the device and restart the output stream of data.
* INT# : Pulled high via 1K ohm resistor, unused
* EXTAL : Driven by Arduino to clock the HD6805V1
* XTAL : I've left it floating with good results, but it should probably be grounded.
* NUM : Pulled high via 1K ohm resistor, in NUM mode it becomes a sync output used to demultiplex port B
* TIMER : Pulled high via 1K ohm resistor
* PB[7:0],PC[2:0],NUM : Input to Arduino for logging
* PA[7:0] : Connected to eight jumpers to specify an opcode (in this case 0x9D for NOP)
* PD[7:0] : Unused, pulled high via 1K ohm resistor network. The pull-ups may not be necessary.
* PC[7:3] : Unused

## Exploiting non-user mode

The most useful side effect of non-user mode is that the internal data bus multiplexed on port B outputs whatever internal memory location the program counter is currently addressing, allowing the entirety of the HD6805V1 address space to be read out. To control this process to dump ROM data a simple 1-byte instruction like NOP or RSP can have their opcode driven on to port A and the HD6805 will continuously execute it, output data from the current internal address, and increment the program counter by one. This process can continue indefinitely until all ROM data has been read.

## Reset timing

* Clock cycle 1 : NUM low, port B,C output address 0xFFE.
* Clock cycle 2 : NUM high, port B outputs data from internal ROM address 0xFFE. This is the reset vector LSB.
* Clock cycle 3 : NUM low, port B,C output address 0xFFF.
* Clock cycle 4 : NUM high, port B outputs data from internal ROM address 0xFFF. This is the reset vector MSB.
* Clock cycle 5 : NUM low, port B,C output address 0x000.
* Clock cycle 6 : NUM high, port B outputs 0xFF.
* Clock cycle 7 : NUM low, port B,C output address 0xD9D.
* Clock cycle 8 : NUM high, port B outputs data from internal ROM address 0xD9D.

Before this sequence starts there can be 18 to 20 clocks needed for the HD6805V1 to come out of the reset state.

Since we're gating NOP (0x9D) on to port A, the reset vector read is 0xD9D.

## NOP execution timing

* Clock cycle 1 : NUM low, Port B outputs address.
* Clock cycle 2 : NUM high, port B outputs internal data bus.
* Clock cycle 3 : NUM low, Port B outputs address.
* Clock cycle 4 : NUM high, port B outputs internal data bus. At the next clock edge the program counter increments.

As long as NOP or a similar type of opcode is presented this sequence repeats indefinitely.

Here cycles 3,4 present the same data and address as cycles 1,2. When reading ROM the data will be consistent, but when reading first 128 bytes assigned to
I/O, RAM, and unused locations the data can differ.

## Dumping ROM data

As the Arduino shield always outputs 0x9D on to port D, the reset vector is 0xD9D and internal data starts getting fetched from that address.
A more complex device might want to dynamically drive port A to define a reset vector like 0x000 or 0x080, it's simple enough to clock the device 
until the address bus increments from 0xD9D to 0xFFF and then wraps to 0x000. At this point the entire 4K internal address space can be read out.

