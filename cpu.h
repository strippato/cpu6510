//  Copyright (C) 2019  strippato@gmail.com
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <time.h>

// https://www.ktverkko.fi/~msmakela/8bit/cbm.html
// http://www.oxyron.de/html/opcodes02.html
// http://archive.6502.org/datasheets/mos_6510_mpu.pdf
// https://fynydd.com/blog/crash-course-on-emulating-the-mos-6510-cpu/
// https://github.com/pda/c64.rb/tree/master/c64
// https://www.masswerk.at/6502/6502_instruction_set.html#ASL
// https://www.c64-wiki.com/wiki/Kernal
//

#define PRG_NAME    "c6510"
#define PRG_RELEASE "0.9"
#define PRG_AUTHOR  "Strippato"
#define PRG_MAIL    "strippato@gmail.com"

//  PAL  17734475/18 Hz (17734475.0/18.0)
//  NTSC 14318180/14 Hz (14318180.0/14.0)
#define CPU_PAL_HZ  ( 985248.611111111)
#define CPU_NTSC_HZ (1022727.142857143)

struct {
	// cpu freq
	double freq;

	// internal state
	uint64_t cycle;

	// start time
	struct timespec starttime; 
	
	// istruction register
	uint8_t IR;

	// stack pointer
	// top down, 8 bit range, 0x0100 - 0x01FF (implicit first page)
	uint8_t SP;

	// register
	uint8_t A;
	uint8_t Y;
	uint8_t X;

	// processor status
	union {
		uint8_t P;
		struct 	{
			uint8_t C :1;      // 0 carry                                1
			uint8_t Z :1;      // 1 zero                                 2 
			uint8_t I :1;      // 2 irq disable	                         4
			uint8_t D :1;      // 3 decimal                              8
			uint8_t B :1;      // 4 break (1 if int is fired by BRK)    16
			uint8_t X :1;      // 5 not used, always 1                  32
			uint8_t V :1;      // 6 overflow                            64
			uint8_t N :1;      // 7 negative                           128
		};
	} P;

	// program counter
	union {
		uint16_t PC;
		struct {
			uint8_t  PCL;
			uint8_t  PCH;	
		};
	};
} cpu;

uint8_t mem[0xFFFF];

void cpu_init  (double frq);
void cpu_free  (void);

void cpu_addRom (uint16_t address, char* romfile, uint16_t offset);
void cpu_reset (void);
void cpu_run   (void);

// DEBUG
void cpu_dump  (char *message);
void cpu_FIXME (char *message);


union cpu_addr {
	uint16_t addr;
	struct {
		uint8_t  addrL;
		uint8_t  addrH;	
	};
};

#endif // CPU_H

