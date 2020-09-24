//  Copyright (C) 2020  strippato@gmail.com
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


// I'm too lazy for a cmakefile
// gcc -Wall cpu.c main.c -o cpu `pkg-config --cflags --libs glib-2.0`

#include <stdio.h>
#include "cpu.h"

int 
main (int argc, char const *argv[])
{
	printf (PRG_NAME " release " PRG_RELEASE "\n");

	cpu_init (CPU_PAL_HZ);

	//cpu_addRom (0xE000, "rom/kernal.rom", 0);	
	//cpu_addRom (0xD000, "rom/character.rom", 0);	
	//cpu_addRom (0xA000, "rom/basic.rom", 0);

	// see https://github.com/Klaus2m5/6502_65C02_functional_tests
	//cpu_addRom (0x0400, "rom/6502test.bin", 0);

// see nestest https://wiki.nesdev.com/w/index.php/Emulator_tests
// http://www.qmtpro.com/~nes/misc/nestest.log
cpu_addRom (0xc000, "rom/nestest.nes", 0x10);
	
	cpu_reset ();
	
cpu.PC= 0xc000;

	cpu_run (); 

	cpu_free ();

	return 0;
}


