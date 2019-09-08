
/*
A simple "hello world" example.
Set the screen background color and palette colors.
Then write a message to the nametable.
Finally, turn on the PPU to display video.
*/

#include "neslib.h"

#define STATE_START 0
#define STATE_GAME  1

// link the pattern table into CHR ROM
//#link "chr_generic.s"

// main function, run after console reset
static unsigned char VRAM_BUFFER[32];

static unsigned char pad;
static unsigned char state = STATE_START;

void nmi_handler()
{
  set_vram_update(VRAM_BUFFER);
  //scroll(x, 0);
}


void state_start_loop()
{
  //State enter
  VRAM_BUFFER[0] = MSB(NTADR_A(2, 2));
  VRAM_BUFFER[1] = LSB(NTADR_A(2, 2));
  VRAM_BUFFER[2] = 'S';
  VRAM_BUFFER[3] = NT_UPD_EOF;
  
  //State update
  while(1) {
    
    //Logic suff
    if (nesclock() % 30 == 0) {
      VRAM_BUFFER[2] = VRAM_BUFFER[2] == 'S' ? ' ' : 'S';
    }
    
    //nmi sync
    ppu_wait_nmi();
    
    //Read pad
    pad = pad_trigger(0);
    if (pad & PAD_START) {
      state = STATE_GAME;
      break;
    }
  }
  
  //State exit
  //TODO...
}


void state_game_loop()
{
  //State enter
  VRAM_BUFFER[0] = MSB(NTADR_A(2, 2));
  VRAM_BUFFER[1] = LSB(NTADR_A(2, 2));
  VRAM_BUFFER[2] = 'G';
  VRAM_BUFFER[3] = NT_UPD_EOF;
  
  while(1) {
    
    //State logic
    //TODO
    
    //nmi sync
    ppu_wait_nmi();
    
    //Read pad
  }
  
  //State exit
  //TODO
}


void main(void) {
  pal_col(0,0x02);	// set screen to dark blue
  pal_col(1,0x14);	// fuchsia
  pal_col(2,0x20);	// grey
  pal_col(3,0x30);	// white

  nmi_set_callback(nmi_handler);
  ppu_on_all();
  
  // infinite loop
  while (1) {
    if (state == STATE_START) {
    	state_start_loop();
    } else if (state == STATE_GAME) {
      	state_game_loop();
    }
  }
}
