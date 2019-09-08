
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

#include "level.h"

static unsigned char VRAM_BUFFER[32];

static unsigned char pad;
static unsigned char state = STATE_START;

void nmi_handler()
{
  if (VRAM_BUFFER[0] != 0) {
    set_vram_update(VRAM_BUFFER);
  }
}


void load_screen()
{  
  unsigned char brightness = 0;
  ppu_off();
  
  vram_adr(NAMETABLE_A);
  vram_write(level_nam, 1024);
  
  //Disable nmi drawing
  VRAM_BUFFER[0] = 0;
  
  pal_col(0,0x02);
  pal_col(1,0x14);
  pal_col(2,0x20);
  pal_col(3,0x30);
  pal_bg_bright(0);
  
  ppu_on_all();
  
  while(brightness < 8) {
    if (nesclock() % 2 == 0) {
      brightness++;
    }
    pal_bg_bright(brightness);
    ppu_wait_nmi();
  }
	  
  while(brightness > 4) {
    if (nesclock() % 2 == 0) {
      brightness--;
    }
    pal_bg_bright(brightness);
    ppu_wait_nmi();
  }
}


void state_start_loop()
{
  load_screen();
  
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
}


void state_game_loop()
{
  load_screen();
  
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
