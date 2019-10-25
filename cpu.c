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

#include <stdio.h>
#include <glib.h>
#include <assert.h>

#include "cpu.h"

#define CFLAG(X,Y) ((X) >= (Y)         ? 1:0)
#define ZFLAG(X)   ((X) == 0           ? 1:0)
#define NFLAG(X)   (((X) & 0b10000000) ? 1:0)
#define STACKBASE 0x0100

// not gcc? try this
//#define NFLAG(X) (((X) & ((uint8_t) 128)) ? 1:0)

// vic http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt

// ISA
// http://www.ffd2.com/fridge/docs/c64-diss.html
// https://www.masswerk.at/6502/6502_instruction_set.html
// https://www.c64-wiki.com/wiki/Opcode
// http://www.floodgap.com/retrobits/ckb/display.cgi?121
// https://amaus.net/static/S100/commodore/brochure/The%20Complete%20Commodore%20Inner%20Space%20Anthology.pdf
// https://wiki.nesdev.com/w/index.php/Emulator_tests

struct isa_t {
    char opcode[3];
    uint8_t ist_len;
    uint8_t pc_step;
    uint8_t clock;
    void (*f)();
} ISA[];

static inline int8_t
REL2ABS (uint8_t address)
{
    return (NFLAG (address)? -((address ^ 0xFF)+1):address);
}

void 
cpu_ADC_IMM (void)
{
    uint16_t tot = cpu.A + mem[cpu.PC+1] + cpu.P.C;
    int16_t vtot = (int8_t)cpu.A + (int8_t)mem[cpu.PC+1] + cpu.P.C; 

    cpu.A = tot & 0x00FF;

    cpu.P.N = NFLAG (cpu.A);
    cpu.P.Z = ZFLAG (cpu.A);
    cpu.P.C = (tot>0x00FF ? 1:0);    
    cpu.P.V = (vtot < -128 || vtot > 127);

}

void 
cpu_AND_IMM (void)
{
    cpu.A = cpu.A & mem[cpu.PC+1];
    cpu.P.N = NFLAG (cpu.A);
    cpu.P.Z = ZFLAG (cpu.A);
}

void 
cpu_AND_ZEROX (void)
{
    union cpu_addr addr;
    addr.addrL = mem[cpu.PC+1];
    addr.addrH = 0x00;

    cpu.A = cpu.A & mem[addr.addr + cpu.X];

    cpu.P.N = NFLAG (cpu.A);
    cpu.P.Z = ZFLAG (cpu.A);

}

void 
cpu_ASL (void)
{
    cpu.P.C = ((cpu.A & 0b10000000) == 0 ? 0:1);
    cpu.A = cpu.A << 1;
    cpu.P.Z = ZFLAG (cpu.A);
    cpu.P.N = NFLAG (cpu.A);
}

void
cpu_BCC (void)
{
    if (cpu.P.C == 0) {
        // jump relative
        cpu.cycle++; 

        union cpu_addr oldaddr;
        oldaddr.addr = cpu.PC + ISA[cpu.IR].ist_len; // ist len

        cpu.PC += REL2ABS (mem[cpu.PC+1]);

        union cpu_addr newaddr;
        newaddr.addr = cpu.PC + ISA[cpu.IR].ist_len; // ist len

        if (newaddr.addrH != oldaddr.addrH) {
            cpu.cycle++; 
        }
    }
}

void
cpu_BCS (void)
{
    if (cpu.P.C == 1) {
        // jump relative
        cpu.cycle++;

        union cpu_addr oldaddr;
        oldaddr.addr = cpu.PC + ISA[cpu.IR].ist_len; // ist len

        cpu.PC += REL2ABS (mem[cpu.PC+1]);

        union cpu_addr newaddr;
        newaddr.addr = cpu.PC + ISA[cpu.IR].ist_len; // ist len

        if (newaddr.addrH != oldaddr.addrH) {
            cpu.cycle++; 
        }

    }
}

void
cpu_BEQ (void)
{
    // FIXME ALL cycle
    if (cpu.P.Z == 1) {
        // jump relative
        cpu.cycle++; 

        union cpu_addr oldaddr;
        oldaddr.addr = cpu.PC + ISA[cpu.IR].ist_len; // ist len
        
        cpu.PC += REL2ABS (mem[cpu.PC+1]);

        union cpu_addr newaddr;
        newaddr.addr = cpu.PC + ISA[cpu.IR].ist_len; // ist len

        if (newaddr.addrH != oldaddr.addrH) {
            cpu.cycle++; 
        }

    }
}

void
cpu_BIT_ZERO (void)
{
    union cpu_addr addr;
    addr.addrL = mem[cpu.PC+1];
    addr.addrH = 0x00;

    cpu.P.Z = ZFLAG (cpu.A & mem[addr.addr]);
    cpu.P.N = ((mem[addr.addr] & 0b10000000) == 0 ? 0:1);
    cpu.P.V = ((mem[addr.addr] & 0b01000000) == 0 ? 0:1);
                
}

void 
cpu_BNE (void)
{
    if (cpu.P.Z == 0) {
        // jump relative
        cpu.cycle++; 

        union cpu_addr oldaddr;
        oldaddr.addr = cpu.PC + ISA[cpu.IR].ist_len; // ist len

        cpu.PC += REL2ABS (mem[cpu.PC+1]);

        union cpu_addr newaddr;
        newaddr.addr = cpu.PC + ISA[cpu.IR].ist_len; // ist len

        if (newaddr.addrH != oldaddr.addrH) {
            cpu.cycle++; 
        }
    }
}

void
cpu_BPL (void)
{
    if (cpu.P.N == 0 ) {
        // jump relative
        cpu.cycle++; 

        union cpu_addr oldaddr;
        oldaddr.addr = cpu.PC + ISA[cpu.IR].ist_len; // ist len

        cpu.PC += REL2ABS (mem[cpu.PC+1]);

        union cpu_addr newaddr;
        newaddr.addr = cpu.PC + ISA[cpu.IR].ist_len; // ist len

        if (newaddr.addrH != oldaddr.addrH) {
            cpu.cycle++; 
        }
    }
}

void 
cpu_BMI (void) {
    if (cpu.P.N == 1 ) {
        // jump relative
        cpu.cycle++; 

        union cpu_addr oldaddr;
        oldaddr.addr = cpu.PC + ISA[cpu.IR].ist_len; // ist len
        
        cpu.PC += REL2ABS (mem[cpu.PC+1]);

        union cpu_addr newaddr;
        newaddr.addr = cpu.PC + ISA[cpu.IR].ist_len; // ist len
        
        if (newaddr.addrH != oldaddr.addrH) {
            cpu.cycle++; 
        }
    }
}

void
cpu_BRK (void)
{
	cpu_FIXME ("BRK: softirq not implemented"); // softirq pls

	cpu.P.I = 1;	
	cpu.P.B = 1;

	// FIXME: raise softirq here
}

void
cpu_BVC (void)
{
    if (cpu.P.V == 0) {
        // jump relative
        cpu.cycle++;

        union cpu_addr oldaddr;
        oldaddr.addr = cpu.PC + ISA[cpu.IR].ist_len; // ist len

        cpu.PC += REL2ABS (mem[cpu.PC+1]);

        union cpu_addr newaddr;
        newaddr.addr = cpu.PC + ISA[cpu.IR].ist_len; // ist len
        
        if (newaddr.addrH != oldaddr.addrH) {
            cpu.cycle++; 
        }

    }
}

void
cpu_BVS (void)
{
    if (cpu.P.V == 1) {
        // jump relative
        cpu.cycle++;

        union cpu_addr oldaddr;
        oldaddr.addr = cpu.PC + ISA[cpu.IR].ist_len; // ist len

        cpu.PC += REL2ABS (mem[cpu.PC+1]);

        union cpu_addr newaddr;
        newaddr.addr = cpu.PC + ISA[cpu.IR].ist_len; // ist len

        if (newaddr.addrH != oldaddr.addrH) {
            cpu.cycle++; 
        }
    }
}

void
cpu_CLC (void)
{
    cpu.P.C = 0;
}

void
cpu_CLD (void)
{
    cpu.P.D = 0;
}

void
cpu_CLV (void)
{
    cpu.P.V = 0;   
}


void
cpu_CMP_ABS_X (void)
{
    union cpu_addr addr;
    addr.addrL = mem[cpu.PC+1];
    addr.addrH = mem[cpu.PC+2];
    
    uint8_t oldAddrH = addr.addrH;
    addr.addr += cpu.X;

    uint8_t value = mem[addr.addr];

    cpu.P.C = CFLAG (cpu.A , value);
    cpu.P.Z = ZFLAG (cpu.A - value);
    cpu.P.N = NFLAG (cpu.A - value);

    if (addr.addrH != oldAddrH) {
        cpu.cycle++;
    }

}

void
cpu_CMP_IMM (void)
{
    uint8_t value = mem[cpu.PC+1];

    cpu.P.C = CFLAG (cpu.A , value);
    cpu.P.Z = ZFLAG (cpu.A - value);
    cpu.P.N = NFLAG (cpu.A - value);
}

void
cpu_CMP_IND_Y (void)
{
    union cpu_addr addr;
    addr.addrL = mem[cpu.PC+1];
    addr.addrH = 0x00;

    uint8_t value = mem[addr.addr] + cpu.Y;

    cpu.P.C = CFLAG (cpu.A , value);
    cpu.P.Z = ZFLAG (cpu.A - value);
    cpu.P.N = NFLAG (cpu.A - value);

    cpu_FIXME ("CMP_IND_Y: add 1 to cycles if page boundery is crossed");
}

void
cpu_CPX_IMM (void)
{
    uint8_t value = mem[cpu.PC+1];

    cpu.P.C = CFLAG (cpu.X , value);
    cpu.P.Z = ZFLAG (cpu.X - value);
    cpu.P.N = NFLAG (cpu.X - value);
}

void
cpu_CPY_IMM (void)
{
    uint8_t value = mem[cpu.PC+1];

    cpu.P.C = CFLAG (cpu.Y , value);
    cpu.P.Z = ZFLAG (cpu.Y - value);
    cpu.P.N = NFLAG (cpu.Y - value);
}

void
cpu_DEX (void)
{
    cpu.X--;
    cpu.P.Z = ZFLAG (cpu.X);
    cpu.P.N = NFLAG (cpu.X);
}

void
cpu_DEY (void)
{
    cpu.Y--;
    cpu.P.Z = ZFLAG (cpu.Y);
    cpu.P.N = NFLAG (cpu.Y);
}

void
cpu_EOR_IMM (void)
{
    uint8_t value = mem[cpu.PC+1];
    cpu.A = cpu.A ^ value;

    cpu.P.Z = ZFLAG (cpu.A);
    cpu.P.N = NFLAG (cpu.A);
}

void
cpu_INC_ZERO (void)
{
    union cpu_addr addr;
    addr.addrL = mem[cpu.PC+1];
    addr.addrH = 0x00;
    
    ++mem[addr.addr];

    cpu.P.Z = ZFLAG (mem[addr.addr]);
    cpu.P.N = NFLAG (mem[addr.addr]);

}

void
cpu_INX (void)
{
    ++cpu.X;
    cpu.P.Z = ZFLAG (cpu.X);
    cpu.P.N = NFLAG (cpu.X);
}

void
cpu_INY (void)
{
    ++cpu.Y;
    cpu.P.Z = ZFLAG (cpu.Y);
    cpu.P.N = NFLAG (cpu.Y);
}

void
cpu_JSR (void)
{
    uint8_t opl = mem[cpu.PC+1];
    uint8_t oph = mem[cpu.PC+2];

    cpu.PC += 2;
    mem[STACKBASE + cpu.SP] = cpu.PCH;
    cpu.SP--;
    mem[STACKBASE + cpu.SP] = cpu.PCL;
    cpu.SP--;
    
    cpu.PCL = opl;
    cpu.PCH = oph;
}

void
cpu_JMP_ABS (void)
{
    uint8_t opl = mem[cpu.PC+1];
    uint8_t oph = mem[cpu.PC+2];

    cpu.PCL = opl;
    cpu.PCH = oph;
}

void 
cpu_LDA_ABS (void) 
{
    union cpu_addr addr;
    addr.addrL = mem[cpu.PC+1];
    addr.addrH = mem[cpu.PC+2]; 
    //printf ("PC1 %0x PC2 %0x\n", mem[cpu.PC+1], mem[cpu.PC+2]);
    //printf ("PC3 %0x  %0x\n", addr.addr , mem[addr.addr]);

    cpu.A = mem[addr.addr]; 

    cpu.P.Z = ZFLAG (cpu.A);
    cpu.P.N = NFLAG (cpu.A);
}

void
cpu_LDA_ABS_X (void)
{
    union cpu_addr addr;
    
    addr.addrL = mem[cpu.PC+1];
    addr.addrH = mem[cpu.PC+2]; 
    uint8_t oldAddrH = addr.addrH;

    addr.addr += cpu.X;

    cpu.A = mem[addr.addr]; 
    cpu.P.Z = ZFLAG (cpu.A);
    cpu.P.N = NFLAG (cpu.A);
 
    //cpu_FIXME ("LDA_ABS_X: add 1 to cycles if page boundery is crossed");
    if (addr.addrH !=  oldAddrH) {
        cpu.cycle++;
    }
}

void
cpu_LDA_ABS_Y (void)
{
    union cpu_addr addr;
    addr.addrL = mem[cpu.PC+1];
    addr.addrH = mem[cpu.PC+2]; 
    addr.addr += cpu.Y;

    cpu.A = mem[addr.addr]; 
    cpu.P.Z = ZFLAG (cpu.A);
    cpu.P.N = NFLAG (cpu.A);
    // FIXME: compute the exact cpu cycl count (page cross)
    cpu_FIXME ("LDA_ABS_Y: add 1 to cycles if page boundery is crossed");

}

void
cpu_LDA_IMM (void)
{
    cpu.A = mem[cpu.PC+1];
    cpu.P.Z = ZFLAG (cpu.A);
    cpu.P.N = NFLAG (cpu.A);
}

void
cpu_LDA_IND_X (void)
{
    /*
    FA7  A9 00     LDA #$00                        A:00 X:55 Y:69 P:27 SP:FB PPU:267, 21 CYC:2483
    CFA9  85 80     STA $80 = 00                    A:00 X:55 Y:69 P:27 SP:FB PPU:273, 21 CYC:2485
    CFAB  A9 02     LDA #$02                        A:00 X:55 Y:69 P:27 SP:FB PPU:282, 21 CYC:2488
    CFAD  85 81     STA $81 = 00                    A:02 X:55 Y:69 P:25 SP:FB PPU:288, 21 CYC:2490
    CFAF  A9 FF     LDA #$FF                        A:02 X:55 Y:69 P:25 SP:FB PPU:297, 21 CYC:2493
    CFB1  85 01     STA $01 = FF                    A:FF X:55 Y:69 P:A5 SP:FB PPU:303, 21 CYC:2495
    CFB3  A9 00     LDA #$00                        A:FF X:55 Y:69 P:A5 SP:FB PPU:312, 21 CYC:2498
    CFB5  85 82     STA $82 = 00                    A:00 X:55 Y:69 P:27 SP:FB PPU:318, 21 CYC:2500
    CFB7  A9 03     LDA #$03                        A:00 X:55 Y:69 P:27 SP:FB PPU:327, 21 CYC:2503
    CFB9  85 83     STA $83 = 00                    A:03 X:55 Y:69 P:25 SP:FB PPU:333, 21 CYC:2505
    CFBB  85 84     STA $84 = 00                    A:03 X:55 Y:69 P:25 SP:FB PPU:  1, 22 CYC:2508
    CFBD  A9 00     LDA #$00                        A:03 X:55 Y:69 P:25 SP:FB PPU: 10, 22 CYC:2511
    CFBF  85 FF     STA $FF = 00                    A:00 X:55 Y:69 P:27 SP:FB PPU: 16, 22 CYC:2513
    CFC1  A9 04     LDA #$04                        A:00 X:55 Y:69 P:27 SP:FB PPU: 25, 22 CYC:2516
    CFC3  85 00     STA $00 = 00                    A:04 X:55 Y:69 P:25 SP:FB PPU: 31, 22 CYC:2518
    CFC5  A9 5A     LDA #$5A                        A:04 X:55 Y:69 P:25 SP:FB PPU: 40, 22 CYC:2521
    CFC7  8D 00 02  STA $0200 = 00                  A:5A X:55 Y:69 P:25 SP:FB PPU: 46, 22 CYC:2523
    CFCA  A9 5B     LDA #$5B                        A:5A X:55 Y:69 P:25 SP:FB PPU: 58, 22 CYC:2527
    CFCC  8D 00 03  STA $0300 = 00                  A:5B X:55 Y:69 P:25 SP:FB PPU: 64, 22 CYC:2529
    CFCF  A9 5C     LDA #$5C                        A:5B X:55 Y:69 P:25 SP:FB PPU: 76, 22 CYC:2533
    CFD1  8D 03 03  STA $0303 = 00                  A:5C X:55 Y:69 P:25 SP:FB PPU: 82, 22 CYC:2535
    CFD4  A9 5D     LDA #$5D                        A:5C X:55 Y:69 P:25 SP:FB PPU: 94, 22 CYC:2539
    CFD6  8D 00 04  STA $0400 = 00                  A:5D X:55 Y:69 P:25 SP:FB PPU:100, 22 CYC:2541
    CFD9  A2 00     LDX #$00                        A:5D X:55 Y:69 P:25 SP:FB PPU:112, 22 CYC:2545
  > CFDB  A1 80     LDA ($80,X) @ 80 = 0200 = 5A    A:5D X:00 Y:69 P:27 SP:FB PPU:118, 22 CYC:2547
  > CFDD  C9 5A     CMP #$5A                        A:5A X:00 Y:69 P:25 SP:FB PPU:136, 22 CYC:2553    
    */
    union cpu_addr addr;
    addr.addrL = mem[cpu.PC + 1];
    addr.addrH = 0x00;
    addr.addr = (addr.addr + cpu.X) & 0xFF;

    union cpu_addr addr2;
    addr2.addrL = mem[addr.addr];
    addr2.addrH = mem[(addr.addr + 1) & 0xFF]; // wrap around zero page

//CFF2  A1 FF     LDA ($FF,X) @ FF = 0400 = 5D    A:5C X:00 Y:69 P:27 SP:FB PPU:232, 22 CYC:2585
//CFF4  C9 5D     CMP #$5D                        A:5D X:00 Y:69 P:25 SP:FB PPU:250, 22 CYC:2591


//CFF8  A2 81     LDX #$81                        A:5D X:00 Y:69 P:27 SP:FB PPU:262, 22 CYC:2595
//CFFA  A1 FF     LDA ($FF,X) @ 80 = 0200 = 5A    A:5D X:81 Y:69 P:A5 SP:FB PPU:268, 22 CYC:2597
//CFFC  C9 5A     CMP #$5A                        A:5A X:81 Y:69 P:25 SP:FB PPU:286, 22 CYC:2603

//FD: PC:CFFA Nv-bvdIzC A:5D X:81 Y:69 PS:A5 SP:FB IR:A1 (LDA,2) CYCLE:2597

    cpu.A = mem[addr2.addr];

    cpu.P.Z = ZFLAG (cpu.A);
    cpu.P.N = NFLAG (cpu.A);
}

void
cpu_LDA_IND_Y (void)
{
    cpu.A = mem[mem[cpu.PC+1]] + cpu.Y;

    cpu.P.Z = ZFLAG (cpu.A);
    cpu.P.N = NFLAG (cpu.A);

    // FIXME: compute the exact cpu cycl count (page cross)
    cpu_FIXME ("LDA_IND_Y: add 1 to cycles if page boundery is crossed");
}

void
cpu_LDA_ZERO (void)
{
    union cpu_addr addr;
    addr.addrL = mem[cpu.PC+1];
    addr.addrH = 0x00;

    cpu.A = mem[addr.addr];
    cpu.P.Z = ZFLAG (cpu.A);
    cpu.P.N = NFLAG (cpu.A);
}

void
cpu_LDA_ZERO_X (void)
{
    union cpu_addr addr;
    addr.addrL = mem[cpu.PC+1];
    addr.addrH = 0x00;

    cpu.A = mem[addr.addr + cpu.X];
    cpu.P.Z = ZFLAG (cpu.A);
    cpu.P.N = NFLAG (cpu.A);

}

void 
cpu_LDX_ABS (void) 
{
    union cpu_addr addr;
    addr.addrL = mem[cpu.PC+1];
    addr.addrH = mem[cpu.PC+2]; 
    cpu.X = mem[addr.addr]; 

    cpu.P.Z = ZFLAG (cpu.X);
    cpu.P.N = NFLAG (cpu.X);
}

void
cpu_LDX_IMM (void)
{
	cpu.X = mem[cpu.PC+1];
    cpu.P.Z = ZFLAG (cpu.X);
    cpu.P.N = NFLAG (cpu.X);
}

void
cpu_LDX_ZERO (void)
{
    union cpu_addr addr;
    addr.addrL = mem[cpu.PC+1];
    addr.addrH = 0x00;

    cpu.X = mem[addr.addr];
    cpu.P.Z = ZFLAG (cpu.X);
    cpu.P.N = NFLAG (cpu.X);
}

void
cpu_LDY (void)
{
    cpu.Y = mem[cpu.PC+1];
    cpu.P.Z = ZFLAG (cpu.Y);
    cpu.P.N = NFLAG (cpu.Y);
}

void
cpu_LDY_ZERO (void)
{
    union cpu_addr addr;
    addr.addrL = mem[cpu.PC+1];
    addr.addrH = 0x00;

    cpu.Y = mem[addr.addr];
    cpu.P.Z = ZFLAG (cpu.Y);
    cpu.P.N = NFLAG (cpu.Y);
}

void
cpu_LDY_ZERO_X (void)
{
    union cpu_addr addr;
    addr.addrL = mem[cpu.PC+1];
    addr.addrH = 0x00;

    cpu.Y = mem[addr.addr + cpu.X];
    cpu.P.Z = ZFLAG (cpu.Y);
    cpu.P.N = NFLAG (cpu.Y);
}

void 
cpu_LSR (void)
{
    cpu.P.C = cpu.A & 0b00000001;
    cpu.A = cpu.A >> 1;
    cpu.P.Z = ZFLAG (cpu.A);
    cpu.P.N = 0;
}

void
cpu_NOP (void)
{
}

void
cpu_ORA_ABS (void)
{
    union cpu_addr addr;
    addr.addrL = mem[cpu.PC+1];
    addr.addrH = mem[cpu.PC+2];

    cpu.A = cpu.A | mem[addr.addr];
    cpu.P.Z = ZFLAG (cpu.A);
    cpu.P.N = NFLAG (cpu.A);
}

void 
cpu_ORA_IMM (void)
{
    cpu.A = cpu.A | mem[cpu.PC+1];
    cpu.P.Z = ZFLAG (cpu.A);
    cpu.P.N = NFLAG (cpu.A);
}


void
cpu_ROL (void)
{
    uint8_t OldRegA = cpu.A;

    cpu.A = cpu.A << 1;
    cpu.A = cpu.A | cpu.P.C;

    cpu.P.C = NFLAG (OldRegA);
    cpu.P.Z = ZFLAG (cpu.A);
    cpu.P.N = NFLAG (cpu.A);

}


void
cpu_ROR (void)
{
    uint8_t bit0 = cpu.A & 0b00000001;

    cpu.A = cpu.A >> 1;
    cpu.A = cpu.A | (cpu.P.C << 7);

    cpu.P.C = bit0;
    cpu.P.Z = ZFLAG (cpu.A);
    cpu.P.N = NFLAG (cpu.A);
}


void
cpu_RTS (void)
{
    cpu.SP++;
    cpu.PCL = mem[cpu.SP + STACKBASE];
    cpu.SP++;    
    cpu.PCH = mem[cpu.SP + STACKBASE];
}

void
cpu_RTI (void)
{
    cpu.SP++;
    uint8_t brk = cpu.P.B;

    cpu.P.P = mem[cpu.SP + STACKBASE];
    cpu.P.B = brk; // not sure about this... see http://www.oxyron.de/html/opcodes02.html
    cpu.P.X = 1; // unused cant be restored

    cpu.SP++;
    cpu.PCL = mem[cpu.SP + STACKBASE];
    cpu.SP++;    
    cpu.PCH = mem[cpu.SP + STACKBASE];

}

void
cpu_SBC_IMM (void)
{
    uint16_t tot = 0xFF + cpu.A - mem[cpu.PC+1] + cpu.P.C;

    if ((cpu.A ^ mem[cpu.PC+1]) & 0x80) {
        if (tot < 0x80 || tot >= 0x180) {
            cpu.P.V = 0;
        } else {
            cpu.P.V = 1;
        }
    } else {
        cpu.P.V = 0;
    }

    if (tot < 0x100) {
      cpu.P.C = 0;
    } else {
      cpu.P.C = 1;
    }

    cpu.A = tot & 0xFF;

    cpu.P.N = NFLAG (cpu.A);
    cpu.P.Z = ZFLAG (cpu.A);
}


void
cpu_SEC (void)
{
    cpu.P.C = 1;
}

void
cpu_SED (void)
{
    cpu.P.D = 1;
}

void
cpu_SEI (void)
{
    cpu.P.I = 1;
}

void 
cpu_STA_ABS (void)
{
    union cpu_addr addr;
    addr.addrL = mem[cpu.PC+1];
    addr.addrH = mem[cpu.PC+2];

    mem[addr.addr] = cpu.A;
}

void
cpu_STA_ABS_X (void)
{
    union cpu_addr addr;
    addr.addrL = mem[cpu.PC+1];
    addr.addrH = mem[cpu.PC+2];

    mem[addr.addr + cpu.X] = cpu.A;   

}

void
cpu_STA_ABS_Y (void)
{
    union cpu_addr addr;
    addr.addrL = mem[cpu.PC+1];
    addr.addrH = mem[cpu.PC+2];

    mem[addr.addr + cpu.Y] = cpu.A;   
}

void
cpu_STA_IND_X (void)
{
    //2612

    union cpu_addr addr1;
    addr1.addr = mem[cpu.PC+1] + cpu.X;

    union cpu_addr addr2;
    addr2.addrL = mem[addr1.addr];
    addr2.addrH = mem[addr1.addr+1];


//printf("add1 %0x %0x %0x\n", addr1.addr, addr2.addr, mem[addr2.addr]);
    mem[addr2.addr] = cpu.A;
//printf("DUmP  %0x\n", mem[addr2.addr]);


//2674
//FIXME D031  AD 00 04  LDA $0400 = AD                  A:AC X:00 Y:69 P:27 SP:FB PPU:158, 23 CYC:2674    

}

void
cpu_STA_IND_Y (void)
{
    printf("FIXME PLZ  \n"); // come sopra
    mem[mem[cpu.PC+1]+cpu.Y] = cpu.A;
}

void 
cpu_STA_ZERO (void) 
{
    union cpu_addr addr;
    addr.addrL = mem[cpu.PC+1];
    addr.addrH = 0x00;

    mem[addr.addr] = cpu.A;
}

void
cpu_STX_ABS (void)
{
    union cpu_addr addr;
    addr.addrL = mem[cpu.PC+1];
    addr.addrH = mem[cpu.PC+2];

    mem[addr.addr] = cpu.X;
}

void
cpu_STX_ZERO (void)
{
    union cpu_addr addr;
    addr.addrL = mem[cpu.PC+1];
    addr.addrH = 0x00;

    mem[addr.addr] = cpu.X;
}

void
cpu_STY_ABS (void)
{
    union cpu_addr addr;
    addr.addrL = mem[cpu.PC+1];
    addr.addrH = mem[cpu.PC+2];

    mem[addr.addr] = cpu.Y;
}

void
cpu_STA_ZERO_X (void)
{
    union cpu_addr addr;
    addr.addrL = mem[cpu.PC+1];
    addr.addrH = 0x00;

    mem[addr.addr + cpu.X] = cpu.A;
}

void
cpu_STY_ZERO (void)
{
    union cpu_addr addr;
    addr.addrL = mem[cpu.PC+1];
    addr.addrH = 0x00;

    mem[addr.addr] = cpu.Y;
}

void
cpu_STY_ZERO_X (void)
{
    union cpu_addr addr;
    addr.addrL = mem[cpu.PC+1];
    addr.addrH = 0x00;

    mem[addr.addr + cpu.X] = cpu.Y;

}

void
cpu_TAX (void)
{
    cpu.X = cpu.A;
    cpu.P.Z = ZFLAG (cpu.X);
    cpu.P.N = NFLAG (cpu.X);
}

void 
cpu_TAY (void)
{
    cpu.Y = cpu.A;
    cpu.P.Z = ZFLAG (cpu.Y);
    cpu.P.N = NFLAG (cpu.Y);
}

void
cpu_TXA (void)
{
    cpu.A = cpu.X;
    cpu.P.Z = ZFLAG (cpu.A);
    cpu.P.N = NFLAG (cpu.A);
}

void
cpu_TSX (void)
{
    cpu.X = cpu.SP;
    cpu.P.Z = ZFLAG (cpu.X);
    cpu.P.N = NFLAG (cpu.X);

}

void
cpu_TXS (void)
{
    cpu.SP = cpu.X;
}


void
cpu_PHA (void)
{
    mem[STACKBASE + cpu.SP] = cpu.A;
    cpu.SP--;
}

void
cpu_PHP (void)
{
    /* so where 0x10 come from ?
      http://www.zimmers.net/anonftp/pub/cbm/documents/chipdata/64doc
      PHP always pushes the Break (B) flag as a `1' to the stack.
      Jukka Tapanimäki claimed in C=lehti issue 3/89, on page 27 that the processor makes a logical OR between the status register's bit 4 and the bit 8 of the stack pointer register (which is always 1).
      He did not give any reasons for this argument, and has refused to clarify it afterwards. Well, this was not the only error in his article...    
    */
    mem[STACKBASE + cpu.SP] = cpu.P.P | 0x10;
    cpu.SP--;
}

void
cpu_PLA (void)
{
    cpu.SP++;
    cpu.A = mem[STACKBASE + cpu.SP];
    cpu.P.Z = ZFLAG (cpu.A);
    cpu.P.N = NFLAG (cpu.A);
}

void
cpu_PLP (void)
{
    //https://wiki.nesdev.com/w/index.php/Status_flags
    // Two instructions (PLP and RTI) pull a byte from the stack and set all the flags. They ignore bits 5 and 4. 
    // ignore bit 5 and 4
    cpu.SP++;
    gboolean oldB = cpu.P.B;
    gboolean oldX = cpu.P.X;
    cpu.P.P = mem[STACKBASE + cpu.SP];
    cpu.P.B = oldB;
    cpu.P.X = oldX;
}

void
cpu_TYA (void)
{
    cpu.A = cpu.Y;
    cpu.P.Z = ZFLAG (cpu.A);
    cpu.P.N = NFLAG (cpu.A);
}

struct isa_t ISA[] = {
    // +OPCODE
    // |     +IST.LEN
    // |     |  +PC STP   
    // |     |  |
    // |     |  |  +CPU CYCLE
    // |     |  |  | 
	{ "BRK", 1, 2, 7, cpu_BRK  },           // 0x00 BREAKPOINT FIXME:fire an irq. WTF?!? why brk do a PC+2?
	{ "---", 2, 2, 6, cpu_FIXME},           // 0x01
	{ "---", 1, 1, 1, cpu_FIXME},           // 0x02
	{ "---", 1, 1, 1, cpu_FIXME},           // 0x03
	{ "---", 1, 1, 1, cpu_FIXME},           // 0x04
	{ "---", 2, 2, 3, cpu_FIXME},           // 0x05
	{ "---", 2, 2, 5, cpu_FIXME},           // 0x06	
	{ "---", 1, 1, 1, cpu_FIXME},           // 0x07
	{ "PHP", 1, 1, 3, cpu_PHP  },           // 0x08 PHP  Push Processor Status on Stack
	{ "ORA", 2, 2, 2, cpu_ORA_IMM},         // 0x09 ORA  OR Memory with Accumulator
	{ "ASL", 1, 1, 2, cpu_ASL  },           // 0x0A ASL  Shift Left One Bit Accumulator
	{ "---", 1, 1, 1, cpu_FIXME},           // 0x0B
	{ "---", 1, 1, 1, cpu_FIXME},           // 0x0C
	{ "ORA", 3, 3, 4, cpu_ORA_ABS},         // 0x0D ORA  OR Memory with Accumulator
	{ "---", 3, 3, 6, cpu_FIXME},           // 0x0E
	{ "---", 1, 1, 1, cpu_FIXME},           // 0x0F

    { "BPL", 2, 2, 2, cpu_BPL  },           // 0x10 BPL  Branch on Result Plus
    { "---", 2, 2, 5, cpu_FIXME},           // 0x11
    { "---", 1, 1, 1, cpu_FIXME},           // 0x12
	{ "---", 1, 1, 1, cpu_FIXME},           // 0x13
    { "---", 1, 1, 1, cpu_FIXME},           // 0x14
    { "---", 2, 2, 4, cpu_FIXME},           // 0x15
	{ "---", 2, 2, 6, cpu_FIXME},           // 0x16
    { "---", 1, 1, 1, cpu_FIXME},           // 0x17
    { "CLC", 1, 1, 2, cpu_CLC  },           // 0x18 CLC  Clear Carry Flag
    { "---", 3, 3, 4, cpu_FIXME},           // 0x19
    { "---", 1, 1, 1, cpu_FIXME},           // 0x1A
    { "---", 1, 1, 1, cpu_FIXME},           // 0x1B
    { "---", 1, 1, 1, cpu_FIXME},           // 0x1C
    { "---", 3, 3, 4, cpu_FIXME},           // 0x1D
    { "---", 3, 3, 6, cpu_FIXME},           // 0x1E
    { "---", 1, 1, 1, cpu_FIXME},           // 0x1F

    { "JSR", 3, 0, 6, cpu_JSR  },           // 0x20 JSR  Jump to New Location Saving Return Address
    { "---", 1, 1, 1, cpu_FIXME},           // 0x21
    { "---", 1, 1, 1, cpu_FIXME},           // 0x22
	{ "---", 1, 1, 1, cpu_FIXME},           // 0x23
    { "BIT", 2, 2, 3, cpu_BIT_ZERO},        // 0x24 BIT  Test Bits in Memory with Accumulator
    { "---", 1, 1, 1, cpu_FIXME},           // 0x25
	{ "---", 1, 1, 1, cpu_FIXME},           // 0x26
    { "---", 1, 1, 1, cpu_FIXME},           // 0x27
    { "PLP", 1, 1, 4, cpu_PLP  },           // 0x28 PLP  Pull Processor Status from Stack
    { "AND", 2, 2, 2, cpu_AND_IMM},         // 0x29 AND  AND Memory with Accumulator
    { "ROL", 1, 1, 2, cpu_ROL  },           // 0x2A ROL  Rotate One Bit Left (Memory or Accumulator)
    { "---", 1, 1, 1, cpu_FIXME},           // 0x2B
    { "---", 1, 1, 1, cpu_FIXME},           // 0x2C
    { "---", 1, 1, 1, cpu_FIXME},           // 0x2D
    { "---", 1, 1, 1, cpu_FIXME},           // 0x2E
    { "---", 1, 1, 1, cpu_FIXME},           // 0x2F

    { "BMI", 2, 2, 2, cpu_BMI  },           // 0x30 BMI  Branch on Result Minus
    { "---", 1, 1, 1, cpu_FIXME},           // 0x31
    { "---", 1, 1, 1, cpu_FIXME},           // 0x32
	{ "---", 1, 1, 1, cpu_FIXME},           // 0x33
    { "---", 1, 1, 1, cpu_FIXME},           // 0x34
    { "AND", 2, 2, 4, cpu_AND_ZEROX},       // 0x35 AND  AND Memory with Accumulator
	{ "---", 1, 1, 1, cpu_FIXME},           // 0x36
    { "---", 1, 1, 1, cpu_FIXME},           // 0x37
    { "SEC", 1, 1, 2, cpu_SEC  },           // 0x38 SEC  Set Carry Flag
    { "---", 1, 1, 1, cpu_FIXME},           // 0x39
    { "---", 1, 1, 1, cpu_FIXME},           // 0x3A
    { "---", 1, 1, 1, cpu_FIXME},           // 0x3B
    { "---", 1, 1, 1, cpu_FIXME},           // 0x3C
    { "---", 1, 1, 1, cpu_FIXME},           // 0x3D
    { "---", 1, 1, 1, cpu_FIXME},           // 0x3E
    { "---", 1, 1, 1, cpu_FIXME},           // 0x3F

    { "RTI", 1, 0, 6, cpu_RTI  },           // 0x40 RTI  Return from Interrupt
    { "---", 1, 1, 1, cpu_FIXME},           // 0x41
    { "---", 1, 1, 1, cpu_FIXME},           // 0x42
	{ "---", 1, 1, 1, cpu_FIXME},           // 0x43
    { "---", 1, 1, 1, cpu_FIXME},           // 0x44
    { "---", 1, 1, 1, cpu_FIXME},           // 0x45
	{ "---", 1, 1, 1, cpu_FIXME},           // 0x46
    { "---", 1, 1, 1, cpu_FIXME},           // 0x47
    { "PHA", 1, 1, 3, cpu_PHA  },           // 0x48 PHA  Push Accumulator on Stack
    { "EOR", 2, 2, 2, cpu_EOR_IMM},         // 0x49 EOR  Exclusive-OR Memory with Accumulator
    { "LSR", 1, 1, 2, cpu_LSR  },           // 0x4A LSR  Shift One Bit Right Accumulator
    { "---", 1, 1, 1, cpu_FIXME},           // 0x4B
    { "JMP", 3, 0, 3, cpu_JMP_ABS},         // 0x4C JMP  Jump to New Location
    { "---", 1, 1, 1, cpu_FIXME},           // 0x4D
    { "---", 1, 1, 1, cpu_FIXME},           // 0x4E
    { "---", 1, 1, 1, cpu_FIXME},           // 0x4F

    { "BVC", 2, 2, 2, cpu_BVC  },           // 0x50 BVC  Branch on Overflow Clear
    { "---", 1, 1, 1, cpu_FIXME},           // 0x51
    { "---", 1, 1, 1, cpu_FIXME},           // 0x52
	{ "---", 1, 1, 1, cpu_FIXME},           // 0x53
    { "---", 1, 1, 1, cpu_FIXME},           // 0x54
    { "---", 1, 1, 1, cpu_FIXME},           // 0x55
	{ "---", 1, 1, 1, cpu_FIXME},           // 0x56
    { "---", 1, 1, 1, cpu_FIXME},           // 0x57
    { "---", 1, 1, 1, cpu_FIXME},           // 0x58
    { "---", 1, 1, 1, cpu_FIXME},           // 0x59
    { "---", 1, 1, 1, cpu_FIXME},           // 0x5A
    { "---", 1, 1, 1, cpu_FIXME},           // 0x5B
    { "---", 1, 1, 1, cpu_FIXME},           // 0x5C
    { "---", 1, 1, 1, cpu_FIXME},           // 0x5D
    { "---", 1, 1, 1, cpu_FIXME},           // 0x5E
    { "---", 1, 1, 1, cpu_FIXME},           // 0x5F

    { "RTS", 1, 1, 6, cpu_RTS  },           // 0x60 RTS  Return from Subroutine
    { "---", 1, 1, 1, cpu_FIXME},           // 0x61
    { "---", 1, 1, 1, cpu_FIXME},           // 0x62
	{ "---", 1, 1, 1, cpu_FIXME},           // 0x63
    { "---", 1, 1, 1, cpu_FIXME},           // 0x64
    { "---", 1, 1, 1, cpu_FIXME},           // 0x65
	{ "---", 1, 1, 1, cpu_FIXME},           // 0x66
    { "---", 1, 1, 1, cpu_FIXME},           // 0x67
    { "PLA", 1, 1, 4, cpu_PLA  },           // 0x68 PLA  Pull Accumulator from Stack
    { "ADC", 2, 2, 2, cpu_ADC_IMM},         // 0x69 ADC  Add Memory to Accumulator with Carry
    { "ROR", 1, 1, 2, cpu_ROR  },           // 0x6A ROR  Rotate One Bit Right Accumulator
    { "---", 1, 1, 1, cpu_FIXME},           // 0x6B
    { "---", 1, 1, 1, cpu_FIXME},           // 0x6C
    { "---", 1, 1, 1, cpu_FIXME},           // 0x6D
    { "---", 1, 1, 1, cpu_FIXME},           // 0x6E
    { "---", 1, 1, 1, cpu_FIXME},           // 0x6F

    { "BVS", 2, 2, 2, cpu_BVS  },           // 0x70 BVS  Branch on Overflow Set
    { "---", 1, 1, 1, cpu_FIXME},           // 0x71
    { "---", 1, 1, 1, cpu_FIXME},           // 0x72
	{ "---", 1, 1, 1, cpu_FIXME},           // 0x73
    { "---", 1, 1, 1, cpu_FIXME},           // 0x74
    { "---", 1, 1, 1, cpu_FIXME},           // 0x75
	{ "---", 1, 1, 1, cpu_FIXME},           // 0x76
    { "---", 1, 1, 1, cpu_FIXME},           // 0x77
    { "SEI", 1, 1, 2, cpu_SEI  },           // 0x78 SEI  Set Interrupt Disable Status
    { "---", 1, 1, 1, cpu_FIXME},           // 0x79
    { "---", 1, 1, 1, cpu_FIXME},           // 0x7A
    { "---", 1, 1, 1, cpu_FIXME},           // 0x7B
    { "---", 1, 1, 1, cpu_FIXME},           // 0x7C
    { "---", 1, 1, 1, cpu_FIXME},           // 0x7D
    { "---", 1, 1, 1, cpu_FIXME},           // 0x7E
    { "---", 1, 1, 1, cpu_FIXME},           // 0x7F

    { "---", 1, 1, 1, cpu_FIXME},           // 0x80
    { "STA", 2, 2, 6, cpu_STA_IND_X},       // 0x81 STA  Store Accumulator in Memory
    { "---", 1, 1, 1, cpu_FIXME},           // 0x82
	{ "---", 1, 1, 1, cpu_FIXME},           // 0x83
    { "STY", 2, 2, 3, cpu_STY_ZERO},        // 0x84 STY  Store Index Y in Memory
    { "STA", 2, 2, 3, cpu_STA_ZERO},        // 0x85 STA  Store Accumulator in Memory
	{ "STX", 2, 2, 3, cpu_STX_ZERO},        // 0x86 STX  Store Index X in Memory
    { "---", 1, 1, 1, cpu_FIXME},           // 0x87
    { "DEY", 1, 1, 2, cpu_DEY  },           // 0x88 DEY  Decrement Index Y 
    { "---", 1, 1, 1, cpu_FIXME},           // 0x89
    { "TXA", 1, 1, 2, cpu_TXA  },           // 0x8A TXA  Transfer Index X to Accumulator
    { "---", 1, 1, 1, cpu_FIXME},           // 0x8B
    { "STY", 3, 3, 4, cpu_STY_ABS},         // 0x8C STY  Sore Index Y in Memory
    { "STA", 3, 3, 4, cpu_STA_ABS},         // 0x8D STA  Store Accumulator in Memory
    { "STX", 3, 3, 4, cpu_STX_ABS},         // 0x8E STX  Store Index X in Memory
    { "---", 1, 1, 1, cpu_FIXME},           // 0x8F

    { "BCC", 2, 2, 2, cpu_BCC  },           // 0x90 BCC  Branch on Carry Clear
    { "STA", 2, 2, 6, cpu_STA_IND_Y},       // 0x91 STA  Store Accumulator in Memory
    { "---", 1, 1, 1, cpu_FIXME},           // 0x92
	{ "---", 1, 1, 1, cpu_FIXME},           // 0x93
    { "STY", 2, 2, 4, cpu_STY_ZERO_X},      // 0x94 STY  Sore Index Y in Memory
    { "STA", 2, 2, 4, cpu_STA_ZERO_X},      // 0x95 STA  Store Accumulator in Memory
	{ "---", 1, 1, 1, cpu_FIXME},           // 0x96
    { "---", 1, 1, 1, cpu_FIXME},           // 0x97
    { "TYA", 1, 1, 2, cpu_TYA  },           // 0x98 TYA  Transfer Index Y to Accumulator
    { "STA", 3, 3, 5, cpu_STA_ABS_Y},       // 0x99 STA  Store Accumulator in Memory
    { "TXS", 1, 1, 2, cpu_TXS  },           // 0x9A TXS  Transfer Index X to Stack Register
    { "---", 1, 1, 1, cpu_FIXME},           // 0x9B
    { "---", 1, 1, 1, cpu_FIXME},           // 0x9C
    { "STA", 3, 3, 5, cpu_STA_ABS_X},       // 0x9D STA  Store Accumulator in Memory
    { "---", 1, 1, 1, cpu_FIXME},           // 0x9E
    { "---", 1, 1, 1, cpu_FIXME},           // 0x9F

    { "LDY", 2, 2, 2, cpu_LDY  },           // 0xA0 LDY  Load Index Y with Memory
    { "LDA", 2, 2, 6, cpu_LDA_IND_X},       // 0xA1 LDA  Load Accumulator with Memory
    { "LDX", 2, 2, 2, cpu_LDX_IMM},         // 0xA2 LDX #
	{ "---", 1, 1, 1, cpu_FIXME},           // 0xA3
    { "LDY", 2, 2, 3, cpu_LDY_ZERO},        // 0xA4 LDY  Load Index Y with Memory
    { "LDA", 2, 2, 3, cpu_LDA_ZERO},        // 0xA5 LDA  Load Accumulator with Memory
	{ "LDX", 2, 2, 3, cpu_LDX_ZERO},        // 0xA6 LDX  Load Index X with Memory
    { "---", 1, 1, 1, cpu_FIXME},           // 0xA7
    { "TAY", 1, 1, 2, cpu_TAY  },           // 0xA8 TAY  Transfer Accumulator to Index Y
    { "LDA", 2, 2, 2, cpu_LDA_IMM},         // 0xA9 LDA  Load Accumulator with Memory
    { "TAX", 1, 1, 2, cpu_TAX  },           // 0xAA TAX  Transfer Accumulator to Index X
    { "---", 1, 1, 1, cpu_FIXME},           // 0xAB
    { "---", 1, 1, 1, cpu_FIXME},           // 0xAC
    { "LDA", 3, 3, 4, cpu_LDA_ABS},         // 0xAD LDA  Load Accumulator with Memory
    { "LDX", 3, 3, 4, cpu_LDX_ABS},         // 0xAE LDX  Load Index X with Memory
    { "---", 1, 1, 1, cpu_FIXME},           // 0xAF

    { "BCS", 2, 2, 2, cpu_BCS  },           // 0xB0 BCS  Branch on Carry Set
    { "LDA", 2, 2, 5, cpu_LDA_IND_Y},       // 0xB1 LDA  Load Accumulator with Memory
    { "---", 1, 1, 1, cpu_FIXME},           // 0xB2
	{ "---", 1, 1, 1, cpu_FIXME},           // 0xB3
    { "LDY", 2, 2, 4, cpu_LDY_ZERO_X},      // 0xB4 LDY  Load Index Y with Memory
    { "LDA", 2, 2, 4, cpu_LDA_ZERO_X},      // 0xB5 LDA  Load Accumulator with Memory
	{ "---", 1, 1, 1, cpu_FIXME},           // 0xB6
    { "---", 1, 1, 1, cpu_FIXME},           // 0xB7
    { "CLV", 1, 1, 2, cpu_CLV  },           // 0xB8 CLV  Clear Overflow Flag
    { "LDA", 3, 3, 4, cpu_LDA_ABS_Y},       // 0xB9 LDA  Load Accumulator with Memory
    { "TSX", 1, 1, 2, cpu_TSX  },           // 0xBA TSX  Transfer Stack Pointer to Index X
    { "---", 1, 1, 1, cpu_FIXME},           // 0xBB
    { "---", 1, 1, 1, cpu_FIXME},           // 0xBC
    { "LDA", 3, 3, 4, cpu_LDA_ABS_X},       // 0xBD LDA  Load Accumulator with Memory
    { "---", 1, 1, 1, cpu_FIXME},           // 0xBE
    { "---", 1, 1, 1, cpu_FIXME},           // 0xBF

    { "CPY", 2, 2, 2, cpu_CPY_IMM},         // 0xC0 CPY  Compare Memory and Index Y
    { "---", 1, 1, 1, cpu_FIXME},           // 0xC1
    { "---", 1, 1, 1, cpu_FIXME},           // 0xC2
	{ "---", 1, 1, 1, cpu_FIXME},           // 0xC3
    { "---", 1, 1, 1, cpu_FIXME},           // 0xC4
    { "---", 1, 1, 1, cpu_FIXME},           // 0xC5
	{ "---", 1, 1, 1, cpu_FIXME},           // 0xC6
    { "---", 1, 1, 1, cpu_FIXME},           // 0xC7
    { "INY", 1, 1, 2, cpu_INY  },           // 0xC8 INY  Increment Index Y by One
    { "CMP", 2, 2, 2, cpu_CMP_IMM},         // 0xC9 CMP  Compare Memory with Accumulator
    { "DEX", 1, 1, 2, cpu_DEX  },           // 0xCA DEX  Decrement Index X by One
    { "---", 1, 1, 1, cpu_FIXME},           // 0xCB
    { "---", 1, 1, 1, cpu_FIXME},           // 0xCC
    { "---", 1, 1, 1, cpu_FIXME},           // 0xCD
    { "---", 1, 1, 1, cpu_FIXME},           // 0xCE
    { "---", 1, 1, 1, cpu_FIXME},           // 0xCF

    { "BNE", 2, 2, 2, cpu_BNE  },           // 0xD0 Branch on Result not Zero
    { "CMP", 2, 2, 5, cpu_CMP_IND_Y},       // 0xD1 CMP Compare Memory with Accumulator
    { "---", 1, 1, 1, cpu_FIXME},           // 0xD2
	{ "---", 1, 1, 1, cpu_FIXME},           // 0xD3
    { "---", 1, 1, 1, cpu_FIXME},           // 0xD4
    { "---", 1, 1, 1, cpu_FIXME},           // 0xD5
	{ "---", 1, 1, 1, cpu_FIXME},           // 0xD6
    { "---", 1, 1, 1, cpu_FIXME},           // 0xD7
    { "CLD", 1, 1, 2, cpu_CLD  },           // 0xD8 CLD  Clear Decimal Mode
    { "---", 1, 1, 1, cpu_FIXME},           // 0xD9
    { "---", 1, 1, 1, cpu_FIXME},           // 0xDA
    { "---", 1, 1, 1, cpu_FIXME},           // 0xDB
    { "---", 1, 1, 1, cpu_FIXME},           // 0xDC
    { "CMP", 3, 3, 4, cpu_CMP_ABS_X},       // 0xDD CMP  Compare Memory with Accumulator
    { "---", 1, 1, 1, cpu_FIXME},           // 0xDE
    { "---", 1, 1, 1, cpu_FIXME},           // 0xDF

    { "CPX", 2, 2, 2, cpu_CPX_IMM},         // 0xE0 CPX  Compare Memory and Index X
    { "---", 1, 1, 1, cpu_FIXME},           // 0xE1
    { "---", 1, 1, 1, cpu_FIXME},           // 0xE2
	{ "---", 1, 1, 1, cpu_FIXME},           // 0xE3
    { "---", 1, 1, 1, cpu_FIXME},           // 0xE4
    { "---", 1, 1, 1, cpu_FIXME},           // 0xE5
	{ "INC", 2, 2, 5, cpu_INC_ZERO},        // 0xE6 INC  Increment Memory by One
    { "---", 1, 1, 1, cpu_FIXME},           // 0xE7
    { "INX", 1, 1, 2, cpu_INX  },           // 0xE8 INX  Increment Index X by One
    { "SBC", 2, 2, 2, cpu_SBC_IMM},         // 0xE9 SBC  Subtract Memory from Accumulator with Borrow
    { "NOP", 1, 1, 2, cpu_NOP  },           // 0xEA NOP
    { "---", 1, 1, 1, cpu_FIXME},           // 0xEB
    { "---", 1, 1, 1, cpu_FIXME},           // 0xEC
    { "---", 1, 1, 1, cpu_FIXME},           // 0xED
    { "---", 1, 1, 1, cpu_FIXME},           // 0xEE
    { "---", 1, 1, 1, cpu_FIXME},           // 0xEF

    { "BEQ", 2, 2, 2, cpu_BEQ  },           // 0xF0 BEQ  Branch on Result Zero
    { "---", 1, 1, 1, cpu_FIXME},           // 0xF1
    { "---", 1, 1, 1, cpu_FIXME},           // 0xF2
	{ "---", 1, 1, 1, cpu_FIXME},           // 0xF3
    { "---", 1, 1, 1, cpu_FIXME},           // 0xF4
    { "---", 1, 1, 1, cpu_FIXME},           // 0xF5
	{ "---", 1, 1, 1, cpu_FIXME},           // 0xF6
    { "---", 1, 1, 1, cpu_FIXME},           // 0xF7
    { "SED", 1, 1, 2, cpu_SED  },           // 0xF8 SED  Set Decimal Flag
    { "---", 1, 1, 1, cpu_FIXME},           // 0xF9
    { "---", 1, 1, 1, cpu_FIXME},           // 0xFA
    { "---", 1, 1, 1, cpu_FIXME},           // 0xFB
    { "---", 1, 1, 1, cpu_FIXME},           // 0xFC 
    { "---", 1, 1, 1, cpu_FIXME},           // 0xFD
    { "---", 1, 1, 1, cpu_FIXME},           // 0xFE
    { "---", 1, 1, 1, cpu_FIXME}            // 0xFF
 }; 

void
cpu_init  (double freq)
{
    cpu.freq = (freq ? freq : CPU_PAL_HZ);
}

void
cpu_free (void)
{
}

void
cpu_FIXME (char *message)
{
    printf ("<FIX THE OPCODE>\n");
    cpu_dump (message ? message:ISA[cpu.IR].opcode);
    printf ("</FIX THE OPCODE>\n"); 

    if (!message) exit (EXIT_FAILURE);
}

void 
cpu_addRom (uint16_t address, char* romfile, uint16_t offset)
{
    uint16_t count = 0;
	printf ("Adding $%04X %s skipping first $%04X byte", address, romfile, offset);

	if (g_file_test (romfile, G_FILE_TEST_EXISTS)) {
    	FILE *file = fopen (romfile, "rb");
        while (file) {
        	unsigned char chread = fgetc (file);

        	if (feof (file)) break;

            count++;

            if (offset < count) {
        	   mem[address++] = chread;
            }
        }
        fclose (file);

	} else {
		printf (" ERROR! file not found");
	}
	printf ("\n");
}


void
cpu_run (void)
{
    unsigned long int nloop = 1200; // CYCL 2595 ok
    struct timespec now; 

	while (nloop-- >= 1) {
    //while (1) {

        // fetch and decode
		cpu.IR = mem[cpu.PC];

        // DEBUG
        cpu_dump ("FD:");

		// execute
		ISA[cpu.IR].f ();
		cpu.cycle += ISA[cpu.IR].clock;
		cpu.PC += ISA[cpu.IR].pc_step;
		
        // irq
		//if (cpu_pending_irq) {
		//	cpu_irq ();
		//}
/*
        // time to sync
        double trealhw = cpu.cycle * (1.0f / cpu.freq);
        clock_gettime (CLOCK_REALTIME, &now); 
        
        double tspent  = (double)(now.tv_sec - cpu.starttime.tv_sec) + (double)(now.tv_nsec - cpu.starttime.tv_nsec)/(double)1000000000;
        //printf ("time sp:%f rh:%f\n", tspent, trealhw);

        while (tspent < trealhw) {
            printf ("wait \n");
            clock_gettime (CLOCK_REALTIME, &now); 
            tspent  = (double)(now.tv_sec - cpu.starttime.tv_sec) + (double)(now.tv_nsec - cpu.starttime.tv_nsec)/(double)1000000000;
            g_usleep (1);
        }
*/
        // DEBUG EXECUTE
        // cpu_dump ("E_:");

	}
}

void
cpu_reset (void)
{
	//https://www.c64-wiki.com/wiki/Reset_(Process)
	//http://commodore64.se/wiki/index.php/Commodore_64_ROM_Addresses
    cpu.cycle = 7; // was 6 or 7? 

	cpu.PCL = mem[0xFFFC]; // E2
	cpu.PCH = mem[0xFFFD]; // FC
    cpu.A      = 0x00;
    cpu.X      = 0x00;       
    cpu.Y      = 0x00;           
	cpu.IR     = 0x00;

	cpu.SP     = 0xFD;       // implicit on 0x01 page
	cpu.P.P    = 0b00100100; 

    mem[0x0000] = 0x2F;
    mem[0x0001] = 0x37;


    // https://github.com/Klaus2m5/6502_65C02_functional_tests
    // cpu.PCL = 0x00; 
    // cpu.PCH = 0x04; 


    /*
    // FAKE A CARTDRIGE CBM80
     mem[0x8004] = 0xC3;
     mem[0x8005] = 0xC2;
     mem[0x8006] = 0xCD;
     mem[0x8007] = 0x38;
     mem[0x8008] = 0x30;
     */

    clock_gettime (CLOCK_REALTIME, &cpu.starttime); 
}

void
cpu_dump (char *message)
{

    //printf ("%s", message);
	//printf (" PC:%04X ", cpu.PC);
    printf ("%04X", cpu.PC);

    printf ("  %02X ", mem[cpu.PC+0]);    
    if (ISA[cpu.IR].ist_len > 1) {
        printf ("%02X ", mem[cpu.PC+1]);    
    }  else {
        printf ("   ");
    }


    if (ISA[cpu.IR].ist_len > 2) {    
        printf ("%02X ", mem[cpu.PC+2]);    
    } else {
        printf ("   ");
    }

    printf (" IR:%02X (%c%c%c,%01i)", cpu.IR, ISA[cpu.IR].opcode[0], ISA[cpu.IR].opcode[1], ISA[cpu.IR].opcode[2], ISA[cpu.IR].ist_len) ;


	printf (" A:%02X", cpu.A);
	printf (" X:%02X", cpu.X);
	printf (" Y:%02X", cpu.Y);

	printf (" P:%02X", cpu.P.P);

	printf (" SP:%02X", cpu.SP);

    printf (" ");

	printf ("%c", (cpu.P.N == 1 ? 'N':'n'));
	printf ("%c", (cpu.P.V == 1 ? 'V':'v'));
	printf ("%c", (cpu.P.X == 1 ? '-':'_'));
	printf ("%c", (cpu.P.B == 1 ? 'B':'b'));
	printf ("%c", (cpu.P.D == 1 ? 'D':'d'));
	printf ("%c", (cpu.P.I == 1 ? 'I':'i'));
	printf ("%c", (cpu.P.Z == 1 ? 'Z':'z'));
	printf ("%c", (cpu.P.C == 1 ? 'C':'c'));

	printf (" CYC:%lu\n", cpu.cycle);
}

//D01C  AD 00 02  LDA $0200 = AA                  A:AD X:00 Y:69 P:A5 SP:FB PPU: 86, 23 CYC:2650


//2674
//FIXME D031  AD 00 04  LDA $0400 = AD                  A:AC X:00 Y:69 P:27 SP:FB PPU:158, 23 CYC:2674    
