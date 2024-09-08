#include "typedefs.h"

#define MEM_SIZE 0xFFFF
#define DISPLAY_WIDTH 160
#define DISPLAY_HEIGHT 144
#define CPU_FREQ 4194304

// a struct holding the complete state of one gb core
typedef struct {
  // CPU regs (96 bits)
  // Registers can be access as either 8 or 16b
  // AF Accum & Flags
  // BC
  // DW
  // HL
  // SP Stack Pointer only 16b
  // PC Program Counter only 16b
  union {
    struct {
      u8 C;
      u8 B;
      u8 E;
      u8 D;
      u8 L;
      u8 H;
      union {
        struct {
          u8 unused : 4;
          u8 FC : 1;
          u8 FH : 1;
          u8 FN : 1;
          u8 FZ : 1;
        };
        u8 F;
      };
      u8 A;
      u16 SP;
      u16 PC;
    };
    u16 regs[6];
  };

  // 'cpu' mem
  u8 *rom;         // program      0x0000-0x7fff
  u8 eram[0x2000];  // int ram     0xa000-0xbfff
  u8 wram[0x2000];  // work ram    0xc000-0xdfff
  u8 vram[0x2000]; // video ram    0x8000-0x9fff
  u8 oam[0x1000];  // TODO: fix oam
  u8 hram[0x100];    // i/o+high ram 0xff00-0xffff
  u8 stopped;

  // 'ppu'
  u8 pix[160 * 144]; // screen: 160x144
  u8 ppu_mode;
  u8 enable_ppu;

  // counters
  u32 cpu_instr;
  u32 cpu_ticks;
  u32 ppu_mode_clk;
  u32 frame_no;

  // extra registers for handling register transfers
  u8 src_reg;
  u8 dst_reg;
  u8 unimpl;
  u8 enable_int;
  u8 disable_int;
  u8 irq_en;

} gb;

#define BC (g->regs[0])
#define DE (g->regs[1])
#define HL (g->regs[2])
#define AF (g->regs[3])
#define SP (g->regs[4])
#define PC (g->regs[5])

#define _B (g->B)
#define C (g->C)
#define D (g->D)
#define E (g->E)
#define H (g->H)
#define L (g->L)
#define _A (g->A)
#define F (g->F)

#define fC (g->FC)
#define fN (g->FN)
#define fZ (g->FZ)
#define fH (g->FH)

// REGS
// serial link
#define REG_SERIAL (g->hram[0x01])
#define REG_SERIAL_CNTL (g->hram[0x02])
// timer
#define REG_TIM_DIV (g->hram[0x04])
#define REG_TIM_TIMA (g->hram[0x05])
#define REG_TIM_TMA (g->hram[0x06])
#define REG_TIM_TAC (g->hram[0x07])

// LCD control
#define REG_LCDC (g->hram[0x40])
// LCD status
#define REG_LCDSTAT (g->hram[0x41])
// scroll y
#define REG_SCY (g->hram[0x42])
// scroll y
#define REG_SCX (g->hram[0x43])
// current line
#define REG_SCANLINE (g->hram[0x44])
// LYC
#define REG_LYC (g->hram[0x45])
#define REG_OAMDMA (g->hram[0x46])
// sprite palette
#define REG_OBJPAL0 (g->hram[0x48])
#define REG_OBJPAL1 (g->hram[0x49])
// window coords
#define REG_WINY (g->hram[0x4a])
#define REG_WINX (g->hram[0x4b])
// bootrom off
#define REG_BOOTROM (g->hram[0x50])
// interrupt flags
#define REG_INTF (g->hram[0x0f])
// interrupt master eng->hram[0x
#define REG_INTE (g->hram[0xff])
// backgroud paletter
#define REG_BGRDPAL (g->hram[0x47])
