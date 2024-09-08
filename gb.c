#include "gb.h"
#include "bootrom.h"
#include <SDL.h>
#include <ncurses.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

typedef struct {
    u8 num;
    u8 prefix;
    char name[64];
    u8 bytes;
    u8 cycles;
    char flZ[64];
    char flN[64];
    char flH[64];
    char flC[64];
} opcode;
opcode opcs[512];

void initialize(gb* g) {
    // Initialize values to after bootrom for testing...
    /*_A = 0x01;*/
    /*F = 0xB0;*/
    /*_B = 0x00;*/
    /*C = 0x13;*/
    /*D = 0x00;*/
    /*E = 0xD8;*/
    /*H = 0x01;*/
    /*L = 0x4D;*/
    /*SP = 0xFFFE;*/
    /*PC = 0x0100;*/
}

void init_SDL() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        exit(1);
    }

    window = SDL_CreateWindow("SmallBoy GB Emulator", SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED, DISPLAY_WIDTH * 4,
                              DISPLAY_HEIGHT * 4, // Scaling up the window size
                              SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        exit(1);
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        printf("Renderer could not be created! SDL_Error: %s\n",
               SDL_GetError());
        exit(1);
    }
}

//
void load_rom(gb* g, const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        fprintf(stderr, "Failed to open ROM: %s\n", filename);
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    /*printf("rom size: %ld\n", size);*/
    rewind(file);

    g->rom = (u8*)malloc(size);
    size_t read_size = fread(g->rom, sizeof(uint8_t), size, file);
    if (read_size != size) { printf("Failed to read the entire file\n"); }
    fclose(file);

    for (int i = 0; i < sizeof(bootrom) / sizeof(bootrom[0]); i++) {
        g->rom[i] = bootrom[i];
    }
}

void bitchk(gb* g, u8 reg, u8 b) {
    if (((reg >> b) & 0x1) == 1) fZ = 0;
    else fZ = 1;
    fH = 1;
    fN = 0;
    PC += 2;
}

void ld(gb* g, u8* r1, u8* r2) {
    *r1 = *r2;
    PC++;
}

// u8 read - the way gb memory is setup you need to go to different locations
// based on address
u8 r8(gb* g, u16 a) {
    if (a >= 0x0000 && a <= 0x3fff) {    // Accessing rom bank 1 or bootrom
        if (a < 0x100) return g->rom[a]; // TODO: fix this
        else return g->rom[a];
    } else if (a >= 0x4000 && a <= 0x7fff) { // ROM Bank 01-NN
        return g->rom[a]; // TODO: implement offset for mem bank switching
    } else if (a >= 0x8000 && a <= 0x9fff) { // VRAM
        return g->vram[a - 0x8000];
    } else if (a >= 0xA000 && a <= 0xbfff) { // ERAM
        return g->eram[a - 0xa000];
    } else if (a >= 0xC000 && a <= 0xdfff) { // WRAM
        return g->wram[a - 0xc000];
    } else if (a >= 0xe000 && a <= 0xefff) { // TODO: should be echo ram..
        return g->rom[a];
    } else if (a >= 0xF000 && a <= 0xFFFF) { // oam / I/O
        if (a == 0xFF44) return 0x90;
        else if (a <= 0xFE9F) return g->oam[a - 0xF000];
        else return g->hram[a - 0xFF00];
    } else {
        printf("trying to access memory not implemented or bad: %x\n", a);
        exit(1);
    }
}

u8* r8p(gb* g, u16 a) {
    if (a >= 0x0000 && a <= 0x3fff) {     // Accessing rom bank 1 or bootrom
        if (a < 0x100) return &g->rom[a]; // TODO: fix this
        else return &g->rom[a];
    } else if (a >= 0x4000 && a <= 0x7fff) { // ROM Bank 01-NN
        return &g->rom[a]; // TODO: implement offset for mem bank switching
    } else if (a >= 0x8000 && a <= 0x9fff) { // VRAM
        return &g->vram[a - 0x8000];
    } else if (a >= 0xA000 && a <= 0xbfff) { // ERAM
        return &g->eram[a - 0xa000];
    } else if (a >= 0xC000 && a <= 0xdfff) { // WRAM
        return &g->wram[a - 0xc000];
    } else if (a >= 0xe000 && a <= 0xefff) { // TODO: should be echo ram..
        return &g->rom[a];
    } else if (a >= 0xF000 && a <= 0xFFFF) { // oam / I/O
        if (a <= 0xFE9F) return &g->oam[a - 0xF000];
        else return &g->hram[a - 0xFF00];
    } else {
        printf("trying to access memory not implemented or bad: %x\n", a);
        exit(1);
    }
}

u8 f8(gb* g) {
    u8 v = r8(g, PC + 1);
    PC += 2;
    return v;
}

void lda(gb* g, u8* r1, u16* r2) {
    *r1 = r8(g, *r2);
    PC++;
}

void ldahl(gb* g, u8* r1, s8 i) {
    *r1 = r8(g, HL);
    PC++;
    HL += i;
}

u16 r16(gb* g, u16 a) {
    if (a >= 0x0000 && a <= 0x3fff) { // Accessing rom bank 1 or bootrom
        if (a < 0x100) return g->rom[a + 1] << 8 | g->rom[a]; // TODO: fix this
        else return g->rom[a + 1] << 8 | g->rom[a];
    } else if (a >= 0x4000 && a <= 0x7fff) { // ROM Bank 01-NN
        return g->rom[a + 1] << 8 |
               g->rom[a]; // TODO: implement offset for mem bank switching
    } else if (a >= 0x8000 && a <= 0x9fff) { // VRAM
        return g->vram[a + 1 - 0x8000] << 8 | g->vram[a - 0x8000];
    } else if (a >= 0xA000 && a <= 0xbfff) { // ERAM
        return g->eram[a + 1 - 0xA000] << 8 | g->eram[a - 0xa000];
    } else if (a >= 0xC000 && a <= 0xdfff) { // WRAM
        return g->wram[a + 1 - 0xC000] << 8 | g->wram[a - 0xc000];
    } else if (a >= 0xF000 && a <= 0xFFFF) { // oam / I/O
        if (a <= 0xFE9F)
            return g->oam[a + 1 - 0xF000] << 8 | g->oam[a - 0xF000];
        else return g->hram[a + 1 - 0xFF00] << 8 | g->hram[a - 0xFF00];
    } else {
        printf("trying to access memory r16 not implemented or "
               "bad: %x\n",
               a);
        exit(1);
    }
}

u16 f16(gb* g) {
    u16 v = r16(g, PC + 1);
    /*u16 v = (u16)g->rom[PC + 2] << 8 | g->rom[PC + 1];*/
    PC += 3;
    return v;
}

void w8(gb* g, u16 a, u8 v) {
    /*if (a == 0xff01) printf("%c", v);*/
    /*if (a == 0xff40) printf("writing to 0xff40: %x", v);*/
    if (a >= 0x0000 && a <= 0x3fff) {
        if (a < 0x100) g->rom[a] = v; // TODO: fix this
        else g->rom[a] = v;
    } else if (a >= 0x4000 && a <= 0x7fff) { // ROM Bank 01-NN
        g->rom[a] = v; // TODO: implement offset for mem bank switching
    } else if (a >= 0x8000 && a <= 0x9fff) { // VRAM
        g->vram[a - 0x8000] = v;
    } else if (a >= 0xA000 && a <= 0xbfff) { // ERAM
        g->eram[a - 0xa000] = v;
    } else if (a >= 0xC000 && a <= 0xdfff) { // WRAM
        g->wram[a - 0xc000] = v;

        /*if (a == 0xD802) printf("trying to write to 0xD802: %x\n", v);*/
    } else if (a >= 0xe000 && a <= 0xefff) {
        g->rom[a] = v;                       // TODO: this should be echo ram?
    } else if (a >= 0xF000 && a <= 0xFFFF) { // oam / I/O
        if (a <= 0xFE9F) g->oam[a - 0xF000] = v;
        else g->hram[a - 0xFF00] = v;
    } else {
        printf("trying to write memory not implemented or bad: "
               "%x\n",
               a);
        exit(1);
    }
}

void w16(gb* g, u16 a, u16 v) {
    w8(g, a, v & 0xFF);
    w8(g, a + 1, v >> 8);
}
void ldtm(gb* g, u16* r1, u8* r2) {
    w8(g, *r1, *r2);
    PC++;
}
void ldtmhl(gb* g, u8* r2, s8 i) {
    w8(g, HL, *r2);
    PC++;
    HL += i;
}

void ldtm16(gb* g, u16* r1, u16* r2) {
    w16(g, *r1, *r2);
    /*PC += 3;*/
}

void ldtmf16(gb* g, u16* r) {
    u16 v = f16(g);
    w16(g, v, *r);
}
void ldtmf8(gb* g, u8* r) {
    u16 v = f16(g);
    w8(g, v, *r);
}
void ldtm16f8(gb* g, u16* r) {
    u16 v = f8(g);
    w8(g, v, *r);
}
void ldan16(gb* g) {
    u16 a = f16(g);
    _A = r8(g, a);
}

void nop(gb* g) { PC++; }

void dec16(gb* g, u16* r) {
    *r -= 1;
    PC++;
}
void inc16(gb* g, u16* r) {
    *r += 1;
    PC++;
}

void inc8(gb* g, u8* r) {
    fH = ((*r & 0x0f) + 1 > 0x0f);
    *r += 1;
    fZ = (*r == 0);
    fN = 0;
    PC++;
}
void dec8(gb* g, u8* r) {
    fH = ((*r & 0x0f) == 0);
    *r -= 1;
    fZ = (*r == 0);
    fN = 1;
    PC++;
}
void inca8(gb* g, u16* r) {
    u8 v = r8(g, *r);
    inc8(g, &v);
    w8(g, *r, v);
}
void deca8(gb* g, u16* r) {
    u8 v = r8(g, *r);
    dec8(g, &v);
    w8(g, *r, v);
}

void _add8(gb* g, u8* r1, u8* r2, u8 c) {
    u8 r = *r1 + *r2 + c;
    fH = (((*r1 & 0xF) + (*r2 & 0xf) + c) > 0xf) ? 1 : 0;
    fN = 0;
    fC = (((u16)*r1) + ((u16)*r2) + (u16)c) > 0x00ff ? 1 : 0;
    *r1 = r;
    fZ = (*r1 == 0);
    PC++;
}
void _sub8(gb* g, u8* r1, u8* r2, u8 c) {
    u8 r = *r1 - *r2 - c; // TODO: minus carry?
    fZ = (r == 0);
    fH = (((*r1 & 0xF) < (*r2 & 0xf) + c)) ? 1 : 0;
    fN = 1;
    fC = (((u16)*r1) < ((u16)*r2) + (u16)c) ? 1 : 0;
    *r1 = r;
    PC++;
}

void add8(gb* g, u8* r1, u8* r2) { _add8(g, r1, r2, 0); }
void adc8(gb* g, u8* r1, u8* r2) { _add8(g, r1, r2, fC); }
void sub8(gb* g, u8* r1, u8* r2) { _sub8(g, r1, r2, 0); }
void sbc8(gb* g, u8* r1, u8* r2) { _sub8(g, r1, r2, fC); }

void and8(gb* g, u8* r1, u8* r2) {
    *r1 &= *r2;
    fZ = (*r1 == 0);
    fN = 0;
    fH = 1;
    fC = 0;
    PC++;
}
void andhl8(gb* g, u8* r1) {
    u8 v = r8(g, HL);
    and8(g, &_A, &v);
}
void subhl8(gb* g, u8* r1) {
    u8 v = r8(g, HL);
    sub8(g, &_A, &v);
}
void adchl8(gb* g, u8* r1) {
    u8 v = r8(g, HL);
    adc8(g, &_A, &v);
}
void sbchl8(gb* g, u8* r1) {
    u8 v = r8(g, HL);
    sbc8(g, &_A, &v);
}
void addhl8(gb* g, u8* r1) {
    u8 v = r8(g, HL);
    add8(g, &_A, &v);
}
void or8(gb* g, u8* r1, u8* r2) {
    *r1 |= *r2;
    fZ = (*r1 == 0);
    fN = 0;
    fH = 0;
    fC = 0;
    PC++;
}
void orhl8(gb* g, u8* r1) {
    u8 v = r8(g, HL);
    or8(g, &_A, &v);
}
void xor8(gb* g, u8* r1, u8* r2) {
    *r1 ^= *r2;
    fZ = (*r1 == 0);
    fN = 0;
    fH = 0;
    fC = 0;
    PC++;
}
void xorhl8(gb* g, u8* r1) {
    u8 v = r8(g, HL);
    xor8(g, &_A, &v);
}
void cp8(gb* g, u8* r1, u8* r2) {
    u8 r = *r1;
    _sub8(g, r1, r2, 0);
    *r1 = r;
}
void cphl8(gb* g, u8* r1) {
    u8 v = r8(g, HL);
    cp8(g, &_A, &v);
}
void addn8(gb* g) {
    u8 v = f8(g);
    add8(g, &_A, &v);
    PC -= 1;
}
void subn8(gb* g) {
    u8 v = f8(g);
    sub8(g, &_A, &v);
    PC -= 1;
}
void adcn8(gb* g) {
    u8 v = f8(g);
    adc8(g, &_A, &v);
    PC -= 1;
}
void sbcn8(gb* g) {
    u8 v = f8(g);
    sbc8(g, &_A, &v);
    PC -= 1;
}
void andn8(gb* g) {
    u8 v = f8(g);
    and8(g, &_A, &v);
    PC -= 1;
}
void orn8(gb* g) {
    u8 v = f8(g);
    or8(g, &_A, &v);
    PC -= 1;
}
void xorn8(gb* g) {
    u8 v = f8(g);
    xor8(g, &_A, &v);
    PC -= 1;
}
void cpan(gb* g) {
    u8 v = f8(g);
    cp8(g, &_A, &v);
    PC -= 1;
}

void add16(gb* g, u16* r1, u16* r2) {
    fH = ((*r1 & 0x0fff) + (*r2 & 0x0fff) > 0x0fff);
    fN = 0;
    fC = (*r1 > (0xffff - *r2));
    *r1 += *r2;
    PC++;
}
void addn8sp(gb* g) {
    u8 v = f8(g);
    /*fH = ((SP & 0x000f) + (v & 0x000F) > 0x000f);*/
    fN = 0;
    /*fC = (SP > (0xffff - (s8)v));*/
    fH = ((SP & 0x000f) + (v & 0x000f) > 0x000f);
    fC = ((SP & 0x00ff) + (((u16)((s16)v)) & 0x00ff) > 0x00ff); //?
    fZ = 0;
    SP += (s8)v;
}
void f8_func(gb* g) {
    u8 v = f8(g);
    /*fH = ((SP & 0x000f) + (v & 0x000F) > 0x000f);*/
    fN = 0;
    /*fC = (SP > (0xffff - (s8)v));*/
    fH = ((SP & 0x000f) + (v & 0x000f) > 0x000f);
    fC = ((SP & 0x00ff) + (((u16)((s16)v)) & 0x00ff) > 0x00ff); //?
    fZ = 0;
    HL = SP + (s8)v;
}
void stop(gb* g) {
    printf("stop\n");
    exit(1);
}
void j16(gb* g) {
    u16 a = f16(g);
    PC = a;
}
void jr(gb* g) {
    s8 v = f8(g);
    PC += v;
}
void jump(gb* g, u16 a) { PC = a; }
void jc(gb* g, u8 f) {
    if (f == 1) {
        j16(g);
        return;
    }
    PC += 3;
}
void jnc(gb* g, u8 f) {
    if (f == 0) {
        j16(g);
        return;
    }
    PC += 3;
}

void jrc(gb* g, u8 f) {
    if (f == 1) {
        jr(g);
        return;
    }
    PC += 2;
}

void jrnc(gb* g, u8 f) {
    if (f == 0) {
        jr(g);
        return;
    }
    PC += 2;
}
void call16(gb* g) {
    u16 a = f16(g);
    SP -= 2; // TODO: might need to do 2
    w16(g, SP, PC);
    PC = a;
}
void call16nc(gb* g, u8 f) {
    if (f == 0) {
        call16(g);
        return;
    }
    PC += 3;
}
void call16c(gb* g, u8 f) {
    if (f == 1) {
        call16(g);
        return;
    }
    PC += 3;
}
void ret(gb* g) {
    PC = r16(g, SP);
    SP += 2;
    /*PC++;*/
}
void retc(gb* g, u8 f) {
    if (f == 1) {
        PC = r16(g, SP);
        SP += 2;
        return;
    }
    /*PC++;*/
    PC++;
}
void retnc(gb* g, u8 f) {
    if (f == 0) {
        PC = r16(g, SP);
        SP += 2;
        return;
    }
    /*PC++;*/
    PC++;
}

void push16(gb* g, u16* r) {
    SP -= 2;
    w16(g, SP, *r);
    PC++;
}
void pop16(gb* g, u16* r) {
    *r = r16(g, SP);
    SP += 2;
    PC++;
}

// bitwise ops
void rl(gb* g, u8* r) {
    u8 c = ((*r >> 7) & 0x01); // carry 7th bit if needed
    u8 v = (0xff & (*r << 1)) | fC;
    *r = v;
    fZ = (v == 0);
    fH = 0;
    fN = 0;
    fC = c;
    PC += 2;
}
void rlc(gb* g, u8* v) {        // rotate left with carry
    u8 c = ((*v >> 7) == 0x01); // carry if bit 7 set
    u8 r = (*v << 1) | c;       // shift and carry previous bit 7 into 0
    fH = 0;
    fN = 0;
    fZ = (r == 0);
    fC = c;
    *v = r;
    PC += 2;
}
void rrc(gb* g, u8* v) {         // rotate right with carry
    u8 c = (*v & 0x01);          // carry if bit 0 set
    u8 r = (*v >> 1) | (c << 7); // shift and carry previous bit 0 into 7
    fH = 0;
    fN = 0;
    fZ = (r == 0);
    fC = c;
    *v = r;
    PC += 2;
}
void swap(gb* g, u8* v) {
    fZ = (*v == 0);
    fC = 0;
    fN = 0;
    fH = 0;
    *v = ((*v >> 4) | (*v << 4));
    PC += 2;
}
void res(gb* g, u8* v, u8 i) {
    *v &= ~(1 << i);
    PC += 2;
}
void set(gb* g, u8* v, u8 i) {
    *v |= (1 << i);
    PC += 2;
}
void rr(gb* g, u8* r) {
    u8 c = (*r & 0x01);           // carry lsb if there
    u8 v = (*r >> 1) | (fC << 7); // carry shifts on if needed
    *r = v;
    fZ = (v == 0);
    fH = 0;
    fN = 0;
    fC = c;
    PC += 2;
}
void sla(gb* g, u8* r) {
    u8 c = (*r >> 7) & 0x01; // carry 7th bit if needed
    u8 v = (*r << 1);
    *r = v;
    fZ = (v == 0);
    fH = 0;
    fN = 0;
    fC = c;
    PC += 2;
}
void sra(gb* g, u8* r) {
    u8 c = (*r & 0x01);             // carry lsb if there
    u8 v = (*r >> 1) | (*r & 0x80); // TODO: what?
    *r = v;
    fZ = (v == 0);
    fH = 0;
    fN = 0;
    fC = c;
    PC += 2;
}
void srl(gb* g, u8* v) { // shift right logical
    u8 c = (*v & 0x1);   // if bit 0 set
    u8 r = (*v >> 1);    // shift
    fH = 0;
    fN = 0;
    fZ = (r == 0);
    fC = c;
    *v = r;
    PC += 2;
}

void ldhan(gb* g) { _A = r8(g, 0xFF00 + f8(g)); }
void ldhna(gb* g) { w8(g, 0xFF00 + f8(g), _A); }
void execute_cb(gb* g) {
    u8 cb_opcode = r8(g, PC + 1);
    u8 reg = cb_opcode & 0x07; // bottom three bits define reg
    u8* regp;
    switch (reg) {
    case 0x00: regp = &_B; break;
    case 0x01: regp = &C; break;
    case 0x02: regp = &D; break;
    case 0x03: regp = &E; break;
    case 0x04: regp = &H; break;
    case 0x05: regp = &L; break;
    case 0x06: regp = r8p(g, HL); break;
    case 0x07: regp = &_A; break;
    default: printf("not imimasdfhsd\n"); exit(2);
    }

    u8 com = cb_opcode >> 3;
    switch (com) {
    case 0x00: rlc(g, regp); break;
    case 0x01: rrc(g, regp); break;
    case 0x02: rl(g, regp); break;
    case 0x03: rr(g, regp); break;
    case 0x04: sla(g, regp); break;
    case 0x05: sra(g, regp); break;
    case 0x06: swap(g, regp); break;
    case 0x07: srl(g, regp); break;
    case 0x08: bitchk(g, *regp, 0); break;
    case 0x09: bitchk(g, *regp, 1); break;
    case 0x0a: bitchk(g, *regp, 2); break;
    case 0x0b: bitchk(g, *regp, 3); break;
    case 0x0c: bitchk(g, *regp, 4); break;
    case 0x0d: bitchk(g, *regp, 5); break;
    case 0x0e: bitchk(g, *regp, 6); break;
    case 0x0f: bitchk(g, *regp, 7); break;
    case 0x10: res(g, regp, 0); break;
    case 0x11: res(g, regp, 1); break;
    case 0x12: res(g, regp, 2); break;
    case 0x13: res(g, regp, 3); break;
    case 0x14: res(g, regp, 4); break;
    case 0x15: res(g, regp, 5); break;
    case 0x16: res(g, regp, 6); break;
    case 0x17: res(g, regp, 7); break;
    case 0x18: set(g, regp, 0); break;
    case 0x19: set(g, regp, 1); break;
    case 0x1a: set(g, regp, 2); break;
    case 0x1b: set(g, regp, 3); break;
    case 0x1c: set(g, regp, 4); break;
    case 0x1d: set(g, regp, 5); break;
    case 0x1e: set(g, regp, 6); break;
    case 0x1f: set(g, regp, 7); break;
    default: printf("dafah\n"); exit(2);
    }
}
void rlca(gb* g) {
    rlc(g, &_A);
    fZ = 0;
    PC -= 1;
}
void rrca(gb* g) {
    rrc(g, &_A);
    fZ = 0;
    PC -= 1;
}
void rla(gb* g) {
    rl(g, &_A);
    fZ = 0;
    PC -= 1;
}
void rra(gb* g) {
    rr(g, &_A);
    fZ = 0;
    PC -= 1;
}
void display_tile(gb* g, u8 t, u8 y, u8 x) {
    u64 offset = t * 16;
    u8 y_offset = 0;
    for (u64 i = 0x8000 + offset; i < 0x8010 + offset; i += 2) {
        u8 p1 = r8(g, i);
        u8 p2 = r8(g, i + 1);

        for (s8 j = 7; j >= 0; j--) {
            u8 b1 = (p1 >> j) & 0x1;
            u8 b2 = (p2 >> j) & 0x1;
            u8 p = b2 << 1 | b1;

            u8 ps = 4;
            /*SDL_SetRenderDrawColor(renderer, 255, 255, 255,*/
            /*255); // White pixels*/
            if (p == 3) SDL_SetRenderDrawColor(renderer, 15, 56, 15, 255);
            else if (p == 2) SDL_SetRenderDrawColor(renderer, 48, 98, 48, 255);
            else if (p == 1)
                SDL_SetRenderDrawColor(renderer, 139, 172, 15, 255);
            else if (p == 0) SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);
            SDL_Rect pixel = {((x * 8) + 7 - j) * ps, ((y * 8) + y_offset) * ps,
                              ps, ps};
            SDL_RenderFillRect(renderer, &pixel);
            /*printf("trying to write pixel: %x %x %x\n", p, step, y);*/
        }
        y_offset++;

        /*printf("\n");*/
    }
}
void display_tilemap(gb* g) {
    u64 step = 0;
    u8 x = 0;
    // tilemap 9800-9bff or 9c00-9fff
    // each tile map is 32x32 but only part of the screen is displayed at any
    // point and time
    for (u16 i = 0x9800; i < 0x9c00; i += 1) {
        /*printf("%02x", r8(g, i));*/

        if (x > 31) {
            x = 0;
            /*printf("\n");*/
        }
        u8 y = (u64)step / (u64)32;
        /*printf("y: %x\n", y);*/
        display_tile(g, r8(g, i), y, x);
        x++;
        step++;
    }

    /*for (u16 i = 0xFE00; i < 0xFEA0; i += 4) {*/
    /**/
    /*    u16 y_pos = r8(g, i);*/
    /*    u16 x_pos = r8(g, i + 1);*/
    /*    u16 tile_idx = r8(g, i + 2);*/
    /*    u16 attr_flags = r8(g, i + 3);*/
    /*    printf("displaying sprite at x: %x, y: %x, and tile: %x\n", x_pos /
     * 32,*/
    /*           y_pos / 32, tile_idx);*/
    /*    display_tile(g, r8(g, tile_idx), y_pos, x_pos);*/
    /*}*/
    /*printf("\n\n");*/
}

void render_gb_display(gb* g) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black background
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White pixels

    display_tilemap(g);
    /*for (int y = 0; y < DISPLAY_HEIGHT; ++y) {*/
    /*    for (int x = 0; x < DISPLAY_WIDTH; ++x) {*/
    /*        if (display[y * DISPLAY_WIDTH + x]) {*/
    /*            SDL_Rect pixel = {x * 10, y * 10, 10,*/
    /*                              10}; // Each CHIP-8 pixel is scaled 10x10*/
    /*            SDL_RenderFillRect(renderer, &pixel);*/
    /*        }*/
    /*    }*/
    /*}*/

    SDL_RenderPresent(renderer);
}

void handle_events(int* quit, gb* g) {
    SDL_Event e;
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) { *quit = 1; }
        // Handle key press events
        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
            case SDLK_ESCAPE: *quit = 1; break;
            default: break;
            }
        }
        // Handle key up events
        if (e.type == SDL_KEYUP) {
            switch (e.key.keysym.sym) {
            default: break;
            }
        }
    }
}
void cpl(gb* g) {
    _A = (_A ^ 0xff) & 0xff;
    fH = 1;
    fN = 1;
    PC += 1;
}
void ccf(gb* g) {
    fC = !fC;
    fH = 0;
    fN = 0;
    PC += 1;
} // complement carry flag
void scf(gb* g) {
    fH = 0;
    fN = 0;
    fC = 1;
    PC += 1;
}
void daa(gb* g) {
    u8 a = _A;
    u8 adj = fC ? 0x60 : 0x00;
    if (fH) adj |= 0x06;
    if (!fN) {
        if ((a & 0x0f) > 0x09) adj |= 0x06;
        if (a > 0x99) adj |= 0x60;
        a += adj;
    } else a -= adj;
    fC = (adj >= 0x60);
    fH = 0;
    fZ = (a == 0);
    _A = a;
    PC += 1;
}
void rst(gb* g, u8 v) {
    PC++; // NOTE: I guess increment?
    push16(g, &PC);
    PC = (u16)v;
}
void emulate_cycle(gb* g) {
    u8 opcode = r8(g, PC);
    g->cpu_instr += 1;
    g->cpu_ticks += opcs[opcode].cycles;

    /*render_gb_display(g);*/
    /*if (REG_SERIAL) printf("%x\n", REG_SERIAL);*/
    /*printf("A:%02X F:%02X B:%02X C:%02X D:%02X E:%02X H:%02X "*/
    /*       "L:%02X SP:%04X PC:%04X PCMEM:%02X,%02X,%02X,%02X\n",*/
    /*       _A, F, _B, C, D, E, H, L, SP, PC, r8(g, PC), r8(g, PC + 1),*/
    /*       r8(g, PC + 2), r8(g, PC + 3));*/

    mvprintw(10, 6,
             "A:%02X F:%02X B:%02X C:%02X D:%02X E:%02X H:%02X "
             "L:%02X SP:%04X PC:%04X PCMEM:%02X,%02X,%02X,%02X",
             _A, F, _B, C, D, E, H, L, SP, PC, r8(g, PC), r8(g, PC + 1),
             r8(g, PC + 2), r8(g, PC + 3));
    /*if (opcode == 0xCB)*/
    /*    printf("op: %02x - %s\n", opcs[g->rom[PC + 1] + 0xFF].num,*/
    /*           opcs[g->rom[PC + 1] + 0xFF].name);*/
    /*else printf("op: %02x - %s\n", opcs[opcode].num, opcs[opcode].name);*/

    switch (opcode & 0xFF) {

    case 0x00: nop(g); break;
    case 0x01: BC = f16(g); break;
    case 0x02: ldtm(g, &BC, &_A); break;
    case 0x03: inc16(g, &BC); break;
    case 0x04: inc8(g, &_B); break;
    case 0x05: dec8(g, &_B); break;
    case 0x06: _B = f8(g); break;
    case 0x07: rlca(g); break;

    case 0x08: ldtmf16(g, &SP); break;
    case 0x09: add16(g, &HL, &BC); break;
    case 0x0a: lda(g, &_A, &BC); break;
    case 0x0b: dec16(g, &BC); break;
    case 0x0c: inc8(g, &C); break;
    case 0x0d: dec8(g, &C); break;
    case 0x0e: C = f8(g); break;
    case 0x0f: rrca(g); break;

    case 0x10: stop(g); break;
    case 0x11: DE = f16(g); break;
    case 0x12: ldtm(g, &DE, &_A); break;
    case 0x13: inc16(g, &DE); break;
    case 0x14: inc8(g, &D); break;
    case 0x15: dec8(g, &D); break;
    case 0x16: D = f8(g); break;
    case 0x17: rla(g); break;

    case 0x18: jr(g); break;
    case 0x19: add16(g, &HL, &DE); break;
    case 0x1a: lda(g, &_A, &DE); break;
    case 0x1b: dec16(g, &DE); break;
    case 0x1c: inc8(g, &E); break;
    case 0x1d: dec8(g, &E); break;
    case 0x1e: E = f8(g); break;
    case 0x1f: rra(g); break;

    case 0x20: jrnc(g, fZ); break;
    case 0x21: HL = f16(g); break;
    case 0x22: ldtmhl(g, &_A, 1); break;
    case 0x23: inc16(g, &HL); break;
    case 0x24: inc8(g, &H); break;
    case 0x25: dec8(g, &H); break;
    case 0x26: H = f8(g); break;
    case 0x27: daa(g); break;

    case 0x28: jrc(g, fZ); break;
    case 0x29: add16(g, &HL, &HL); break;
    case 0x2a: ldahl(g, &_A, 1); break;
    case 0x2b: dec16(g, &HL); break;
    case 0x2c: inc8(g, &L); break;
    case 0x2d: dec8(g, &L); break;
    case 0x2e: L = f8(g); break;
    case 0x2f: cpl(g); break;

    case 0x30: jrnc(g, fC); break;
    case 0x31: SP = f16(g); break;
    case 0x32: ldtmhl(g, &_A, -1); break;
    case 0x33: inc16(g, &SP); break;
    case 0x34: inca8(g, &HL); break;
    case 0x35: deca8(g, &HL); break;
    case 0x36: ldtm16f8(g, &HL); break;
    case 0x37: scf(g); break;

    case 0x38: jrc(g, fC); break;
    case 0x39: add16(g, &HL, &SP); break;
    case 0x3a: ldahl(g, &_A, -1); break;
    case 0x3b: dec16(g, &SP); break;
    case 0x3c: inc8(g, &_A); break;
    case 0x3d: dec8(g, &_A); break;
    case 0x3e: _A = f8(g); break;
    case 0x3f: ccf(g); break;

    // Reg to reg ld
    case 0x40: ld(g, &_B, &_B); break;
    case 0x41: ld(g, &_B, &C); break;
    case 0x42: ld(g, &_B, &D); break;
    case 0x43: ld(g, &_B, &E); break;
    case 0x44: ld(g, &_B, &H); break;
    case 0x45: ld(g, &_B, &L); break;
    case 0x46: lda(g, &_B, &HL); break;
    case 0x47: ld(g, &_B, &_A); break;

    case 0x48: ld(g, &C, &_B); break;
    case 0x49: ld(g, &C, &C); break;
    case 0x4a: ld(g, &C, &D); break;
    case 0x4b: ld(g, &C, &E); break;
    case 0x4c: ld(g, &C, &H); break;
    case 0x4d: ld(g, &C, &L); break;
    case 0x4e: lda(g, &C, &HL); break;
    case 0x4f: ld(g, &C, &_A); break;

    case 0x50: ld(g, &D, &_B); break;
    case 0x51: ld(g, &D, &C); break;
    case 0x52: ld(g, &D, &D); break;
    case 0x53: ld(g, &D, &E); break;
    case 0x54: ld(g, &D, &H); break;
    case 0x55: ld(g, &D, &L); break;
    case 0x56: lda(g, &D, &HL); break;
    case 0x57: ld(g, &D, &_A); break;

    case 0x58: ld(g, &E, &_B); break;
    case 0x59: ld(g, &E, &C); break;
    case 0x5a: ld(g, &E, &D); break;
    case 0x5b: ld(g, &E, &E); break;
    case 0x5c: ld(g, &E, &H); break;
    case 0x5d: ld(g, &E, &L); break;
    case 0x5e: lda(g, &E, &HL); break;
    case 0x5f: ld(g, &E, &_A); break;

    case 0x60: ld(g, &H, &_B); break;
    case 0x61: ld(g, &H, &C); break;
    case 0x62: ld(g, &H, &D); break;
    case 0x63: ld(g, &H, &E); break;
    case 0x64: ld(g, &H, &H); break;
    case 0x65: ld(g, &H, &L); break;
    case 0x66: lda(g, &H, &HL); break;
    case 0x67: ld(g, &H, &_A); break;

    case 0x68: ld(g, &L, &_B); break;
    case 0x69: ld(g, &L, &C); break;
    case 0x6a: ld(g, &L, &D); break;
    case 0x6b: ld(g, &L, &E); break;
    case 0x6c: ld(g, &L, &H); break;
    case 0x6d: ld(g, &L, &L); break;
    case 0x6e: lda(g, &L, &HL); break;
    case 0x6f: ld(g, &L, &_A); break;

    case 0x70: ldtm(g, &HL, &_B); break;
    case 0x71: ldtm(g, &HL, &C); break;
    case 0x72: ldtm(g, &HL, &D); break;
    case 0x73: ldtm(g, &HL, &E); break;
    case 0x74: ldtm(g, &HL, &H); break;
    case 0x75: ldtm(g, &HL, &L); break;
    case 0x76: break; // TODO: halt
    case 0x77: ldtm(g, &HL, &_A); break;

    case 0x78: ld(g, &_A, &_B); break;
    case 0x79: ld(g, &_A, &C); break;
    case 0x7a: ld(g, &_A, &D); break;
    case 0x7b: ld(g, &_A, &E); break;
    case 0x7c: ld(g, &_A, &H); break;
    case 0x7d: ld(g, &_A, &L); break;
    case 0x7e: lda(g, &_A, &HL); break;
    case 0x7f: ld(g, &_A, &_A); break;

    case 0x80: add8(g, &_A, &_B); break;
    case 0x81: add8(g, &_A, &C); break;
    case 0x82: add8(g, &_A, &D); break;
    case 0x83: add8(g, &_A, &E); break;
    case 0x84: add8(g, &_A, &H); break;
    case 0x85: add8(g, &_A, &L); break;
    case 0x86: addhl8(g, &_A); break; // halt
    case 0x87: add8(g, &_A, &_A); break;

    case 0x88: adc8(g, &_A, &_B); break;
    case 0x89: adc8(g, &_A, &C); break;
    case 0x8a: adc8(g, &_A, &D); break;
    case 0x8b: adc8(g, &_A, &E); break;
    case 0x8c: adc8(g, &_A, &H); break;
    case 0x8d: adc8(g, &_A, &L); break;
    case 0x8e: adchl8(g, &_A); break; // halt L); break;
    case 0x8f: adc8(g, &_A, &_A); break;

    case 0x90: sub8(g, &_A, &_B); break;
    case 0x91: sub8(g, &_A, &C); break;
    case 0x92: sub8(g, &_A, &D); break;
    case 0x93: sub8(g, &_A, &E); break;
    case 0x94: sub8(g, &_A, &H); break;
    case 0x95: sub8(g, &_A, &L); break;
    case 0x96: subhl8(g, &_A); break; // halt
    case 0x97: sub8(g, &_A, &_A); break;

    case 0x98: sbc8(g, &_A, &_B); break;
    case 0x99: sbc8(g, &_A, &C); break;
    case 0x9a: sbc8(g, &_A, &D); break;
    case 0x9b: sbc8(g, &_A, &E); break;
    case 0x9c: sbc8(g, &_A, &H); break;
    case 0x9d: sbc8(g, &_A, &L); break;
    case 0x9e: sbchl8(g, &_A); break; // halt L); break;
    case 0x9f: sbc8(g, &_A, &_A); break;

    case 0xa0: and8(g, &_A, &_B); break;
    case 0xa1: and8(g, &_A, &C); break;
    case 0xa2: and8(g, &_A, &D); break;
    case 0xa3: and8(g, &_A, &E); break;
    case 0xa4: and8(g, &_A, &H); break;
    case 0xa5: and8(g, &_A, &L); break;
    case 0xa6: andhl8(g, &_A); break; // halt
    case 0xa7: and8(g, &_A, &_A); break;

    case 0xa8: xor8(g, &_A, &_B); break;
    case 0xa9: xor8(g, &_A, &C); break;
    case 0xaa: xor8(g, &_A, &D); break;
    case 0xab: xor8(g, &_A, &E); break;
    case 0xac: xor8(g, &_A, &H); break;
    case 0xad: xor8(g, &_A, &L); break;
    case 0xae: xorhl8(g, &_A); break; // halt L); break;
    case 0xaf: xor8(g, &_A, &_A); break;

    case 0xb0: or8(g, &_A, &_B); break;
    case 0xb1: or8(g, &_A, &C); break;
    case 0xb2: or8(g, &_A, &D); break;
    case 0xb3: or8(g, &_A, &E); break;
    case 0xb4: or8(g, &_A, &H); break;
    case 0xb5: or8(g, &_A, &L); break;
    case 0xb6: orhl8(g, &_A); break; // halt
    case 0xb7: or8(g, &_A, &_A); break;

    case 0xb8: cp8(g, &_A, &_B); break;
    case 0xb9: cp8(g, &_A, &C); break;
    case 0xba: cp8(g, &_A, &D); break;
    case 0xbb: cp8(g, &_A, &E); break;
    case 0xbc: cp8(g, &_A, &H); break;
    case 0xbd: cp8(g, &_A, &L); break;
    case 0xbe: cphl8(g, &_A); break; // halt L); break;
    case 0xbf: cp8(g, &_A, &_A); break;

    case 0xC0: retnc(g, fZ); break;
    case 0xC1: pop16(g, &BC); break;
    case 0xC2: jnc(g, fZ); break;
    case 0xC3: j16(g); break;
    case 0xC4: call16nc(g, fZ); break;
    case 0xC5: push16(g, &BC); break;
    case 0xC6: addn8(g); break;
    case 0xC7: rst(g, 0); break;
    case 0xC8: retc(g, fZ); break;
    case 0xC9: ret(g); break;
    case 0xCA: jc(g, fZ); break;
    case 0xCB: execute_cb(g); break;
    case 0xCC: call16c(g, fZ); break;
    case 0xCD: call16(g); break;
    case 0xCE: adcn8(g); break;
    case 0xCF: rst(g, 8); break;

    case 0xD0: retnc(g, fC); break;
    case 0xD1: pop16(g, &DE); break;
    case 0xD2: jnc(g, fC); break;
    case 0xD4: call16nc(g, fC); break;
    case 0xD5: push16(g, &DE); break;
    case 0xD6: subn8(g); break;
    case 0xD7: rst(g, 0x10); break;
    case 0xD8: retc(g, fC); break;
    case 0xD9:
        g->enable_int = 1;
        ret(g);
        break;
    case 0xDA: jc(g, fC); break;
    case 0xDC: call16c(g, fC); break;
    case 0xDE: sbcn8(g); break;
    case 0xDF: rst(g, 0x18); break;

    case 0xE0: ldhna(g); break;
    case 0xE1: pop16(g, &HL); break;
    case 0xE2:
        w8(g, C + 0xFF00, _A);
        PC++;
        break;
    case 0xE5: push16(g, &HL); break;
    case 0xE6: andn8(g); break;
    case 0xE7: rst(g, 0x20); break;
    case 0xE8: addn8sp(g); break;
    case 0xE9: jump(g, HL); break;
    case 0xEA: ldtmf8(g, &_A); break;
    case 0xEE: xorn8(g); break;
    case 0xEF: rst(g, 0x28); break;

    case 0xF0: ldhan(g); break;
    case 0xF1:
        pop16(g, &AF);
        F = F & 0xf0; // NOTE: bottom 4 bits of f should always be 0
        break;
    case 0xF2:
        _A = r8(g, C + 0xFF00);
        PC++;
        break;
    case 0xF3:
        g->disable_int = 1;
        PC++;
        break;
    case 0xF5: push16(g, &AF); break;
    case 0xF6: orn8(g); break;
    case 0xF7: rst(g, 0x30); break;
    case 0xF8: f8_func(g); break;
    case 0xF9:
        SP = HL;
        PC += 1;
        break;
    case 0xFA: ldan16(g); break;
    case 0xFB:
        g->enable_int = 1;
        PC++;
        break;
    case 0xFE: cpan(g); break;
    case 0xFF: rst(g, 0x38); break;

    default:
        printf("opcode not implemented: %x\n", opcode);
        exit(1);
        break;
    }
}
void interrupts(gb* g) {

    u8 joydata = (~r8(g, 0xff00)) & 0xf0;
    u8 val = 0;
    if ((joydata >> 4) & 0x2)
        val = (0x20) | ((joydata << 0) | (joydata << 1) | (joydata << 2) |
                        (joydata << 3));
    if ((joydata >> 4) & 0x1)
        val = (0x10) | ((joydata << 0) | (joydata << 1) | (joydata << 2) |
                        (joydata << 3));
    if (val) REG_INTF |= 0x10;
    w8(g, 0xff00, ~(val));

    if (g->irq_en == 1) {
        u8 trig = REG_INTE & REG_INTF;
        if (trig) {
            g->irq_en = 0;
            /*halted = 0;*/
            if (trig & 0x1) // vblank
            {
                g->irq_en = 0;
                REG_INTF &= ~0x1;
                rst(g, 0x40);
            } else if (trig & 0x2) { // lcdstat
                g->irq_en = 0;
                REG_INTF &= ~0x2;
                rst(g, 0x48);
            } else if (trig & 0x4) { // timer
                g->irq_en = 0;
                REG_INTF &= ~0x4;
                rst(g, 0x50);
            } else if (trig & 0x10) { //  joypad
                g->irq_en = 0;
                REG_INTF &= ~0x10;
                rst(g, 0x60);
            }
        }
    }

    if (g->enable_int == 1) {
        g->irq_en = 1;
        g->enable_int = 0;
    }
    if (g->disable_int == 1) {
        g->irq_en = 0;
        g->disable_int = 0;
    }
}

void read_csv() {
    FILE* file = fopen("opcodes.csv", "r");
    if (!file) {
        printf("Could not open file\n");
        return;
    }

    /*printf("Attempting to read opcodes...\n");*/

    int op_count = 0;
    char line[50];

    // Skip the header line
    /*fgets(line, 50, file);*/

    // Read each line and parse it
    while (fgets(line, 50, file)) {
        char* token;
        token = strtok(line, ",");
        opcs[op_count].num = (u8)strtol(token, NULL, 0);

        if (strpbrk(line, "CB ") != 0) {
            opcs[op_count].prefix = 1;
        } else {
            opcs[op_count].prefix = 0;
        }

        token = strtok(NULL, ",");
        strcpy(opcs[op_count].name, token);

        token = strtok(NULL, ",");
        opcs[op_count].bytes = atoi(token);

        token = strtok(NULL, ",");
        opcs[op_count].cycles = atoi(token);

        token = strtok(NULL, ",");
        strcpy(opcs[op_count].flZ, token);

        token = strtok(NULL, ",");
        strcpy(opcs[op_count].flN, token);

        token = strtok(NULL, ",");
        strcpy(opcs[op_count].flH, token);

        token = strtok(NULL, ",");
        strcpy(opcs[op_count].flC, token);

        op_count++;
    }

    fclose(file);

    // Print out the parsed data
    /*for (int i = 0; i < op_count; i++) {*/
    /*    printf("num: %x, name: %s, bytes: %x\n", opcs[i].num,
     * opcs[i].name,*/
    /*           opcs[i].bytes);*/
    /*}*/
}

int main(int argc, char** argv) {

    if (argc != 2) {
        printf("Usage: %s <ROM file>\n", argv[0]);
        return 1;
    }

    read_csv();

    gb g;
    /*printf("initializing...\n");*/
    initialize(&g);

    /*printf("loading bootrom...\n");*/
    load_rom(&g, argv[1]);
    init_SDL();

    char title[16];
    memcpy(title, &g.rom[0x134], sizeof(title));
    /*printf("title: %s\n", title);*/
    /*printf("cartridge type: %x\n", g.rom[0x147]);*/
    /*printf("rom size: %x\n", g.rom[0x148]);*/
    /*printf("ram size: %x\n", g.rom[0x149]);*/
    /*printf("header checksum: %x\n", g.rom[0x14D]);*/

    uint8_t checksum = 0;
    for (uint16_t address = 0x0134; address <= 0x014C; address++) {
        checksum = checksum - g.rom[address] - 1;
    }
    /*printf("calculated header checksum: %x\n", checksum);*/
    /*for (int i = 0; i < 16; i++) {*/
    /*  printf("%c\n", (char)g.rom[0x134 + i]);*/
    /*}*/
    int row, col;
    initscr();
    cbreak();
    noecho();
    getmaxyx(stdscr, row, col);
    refresh();

    WINDOW* win = newwin(row - 4, col - 4, 2, 2);
    box(win, 0, 0);
    int k;

    char* quit_str = "q - quit";
    mvprintw(row - 2, col - strlen(quit_str) - 4, "%s", quit_str);

    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);
    attron(COLOR_PAIR(1));

    u64 count = 0;
    int quit = 0;

    while (!quit) {
        emulate_cycle(&g);
        interrupts(&g);
        /*mvprintw(row / 2, (col - strlen("Hello world")) / 2, "%s",*/
        /*         "Hello world");*/

        /*mvprintw(10, 6, "PC:%04x", g.regs[5]);*/
        mvprintw(7, 6, "step:%08x", g.cpu_instr);
        mvprintw(8, 6, "cycl:%08x", g.cpu_ticks);

        u8 x = 0;
        u8 y = 0;
        for (u8 i = 0; i < 0xFF; i++) {
            if (x == 0) mvprintw(16 + y, 6, "%08x", i - y);

            mvprintw(16 + y, 16 + x, "%02x", g.hram[i]);
            x += 3;
            if (x > 48) {
                x = 0;
                y++;
            }
        }

        wrefresh(win);
        /*refresh();*/
        k = getch();
        if (k == 113) break;

        count++;
        if (count % 100000 == 0) {
            handle_events(&quit, &g);
            render_gb_display(&g);
        }
        usleep(10);
        /*SDL_Delay(16); // Delay to control speed, roughly 60 frames per*/
    }
    /*for (;;) {*/
    /*    emulate_cycle(&g);*/
    /*    usleep(10);*/
    /*}*/
    delwin(win);
    endwin();

    return 0;
}
