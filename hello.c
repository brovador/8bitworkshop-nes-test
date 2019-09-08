
/*
A simple "hello world" example.
Set the screen background color and palette colors.
Then write a message to the nametable.
Finally, turn on the PPU to display video.
*/

#include "neslib.h"

// link the pattern table into CHR ROM
//#link "chr_generic.s"

// main function, run after console reset
static unsigned int x = 0;
static unsigned char VRAM_BUFFER[32];

void nmi_handler()
{
  set_vram_update(VRAM_BUFFER);
  scroll(x, 0);
}

void main(void) {

  // set palette colors
  pal_col(0,0x02);	// set screen to dark blue
  pal_col(1,0x14);	// fuchsia
  pal_col(2,0x20);	// grey
  pal_col(3,0x30);	// white

  // write text to name table
  //vram_adr(NTADR_A(2,2));		// set address
  //vram_write("HELLO, WORLD!", 13);	// write bytes to video RAM
  
  VRAM_BUFFER[0] = MSB(NTADR_A(2, 2));
  VRAM_BUFFER[1] = LSB(NTADR_A(2, 2));
  VRAM_BUFFER[2] = '0';
  VRAM_BUFFER[3] = NT_UPD_EOF;
  
  nmi_set_callback(nmi_handler);

  // enable PPU rendering (turn on screen)
  ppu_on_all();

  // infinite loop
  while (1) {
    
    //Do logic stuff...
    
    //Update print area
    x += 1;
    if (x % 30) {
      VRAM_BUFFER[0] = MSB(NTADR_A(0 + x / 30, 1));
      VRAM_BUFFER[1] = LSB(NTADR_A(0 + x / 30, 1));
    }
    
    ppu_wait_nmi();
    //Read pad...
    
  }
}
