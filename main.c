#include "neslib.h"

//#link "chr_generic.s"

#include "screen_start.h"
#include "screen_game.h"

//states
#define STATE_START 0
#define STATE_GAME  1
static unsigned char state = STATE_START;

//strings
#define STR_LEN_PRESS_START 13
static char* str_pressstart = "PRESS START";

//game vars
#define STR_LEN_SCORE 4
#define STR_LEN_LEVEL 2

#define VRAM_FLAGS_DRAW 0x01
#define VRAM_FLAGS_CLEAR 0x00
static unsigned char vram_buffer_flags = VRAM_FLAGS_CLEAR;
static unsigned char VRAM_BUFFER[32];
static unsigned char pad;
static unsigned int i, j, k;
static unsigned int score;
static unsigned char level;

//utils
#define INT_TO_CHR(_X) 0x30 + _X

void nmi_handler()
{
  if (vram_buffer_flags == VRAM_FLAGS_DRAW) {
    vram_buffer_flags = VRAM_FLAGS_CLEAR;
    set_vram_update(VRAM_BUFFER);
  } else {
    set_vram_update(NULL);
  }
}


void load_screen(const unsigned char* data)
{
  //Disable vram NMI update
  //vram_buffer_flags = VRAM_FLAGS_CLEAR;
  //ppu_wait_nmi();

  ppu_off();
  
  vram_adr(NAMETABLE_A);
  vram_unrle(data);
  
  pal_col(0,0x02);
  pal_col(1,0x14);
  pal_col(2,0x20);
  pal_col(3,0x30);
  
  ppu_on_all();
}


void state_start_loop()
{
  //State start
  load_screen(screen_start);
  VRAM_BUFFER[0] = MSB(NTADR_A(10, 20))|NT_UPD_HORZ;
  VRAM_BUFFER[1] = LSB(NTADR_A(10, 20));
  VRAM_BUFFER[2] = STR_LEN_PRESS_START;
  for (i = 0; i < STR_LEN_PRESS_START; i++) {
    VRAM_BUFFER[3 + i] = str_pressstart[i];
  }
  VRAM_BUFFER[3 + STR_LEN_PRESS_START] = NT_UPD_EOF;
  vram_buffer_flags = VRAM_FLAGS_DRAW;
  j = 0;
  
  //State update
  while(1) {
    
    //Logic suff
    if (nesclock() % 32 == 0) {
      j += 1;
      for (i = 0; i < STR_LEN_PRESS_START; i++) {
        VRAM_BUFFER[3 + i] = (j % 2) ? str_pressstart[i] : ' ';
      }
      vram_buffer_flags = VRAM_FLAGS_DRAW;
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
  load_screen(screen_game);
  score = 0000;
  level = 00;
  
  while(1) {
    
    //State logic
    
    
    /* VRAM BUFFER UPDATE */
    k = 0; //VRAM buffer idx
    
    //TODO: check flag score updated
    if (TRUE) {
      VRAM_BUFFER[k] = MSB(NTADR_A(21, 3))|NT_UPD_HORZ;
      VRAM_BUFFER[k + 1] = LSB(NTADR_A(21, 3));
      VRAM_BUFFER[k + 2] = STR_LEN_SCORE;
      i = score;
      for (j = STR_LEN_SCORE; j > 0; j--) {
        VRAM_BUFFER[k + 3 + j - 1] = INT_TO_CHR(i % 10);
        i = i / 10;
      }
      k = k + 3 + STR_LEN_SCORE;
    }
    
    //TODO: check flag level udpated
    if (TRUE) {
       //VRAM udpate - level
      VRAM_BUFFER[k] = MSB(NTADR_A(18, 24))|NT_UPD_HORZ;
      VRAM_BUFFER[k + 1] = LSB(NTADR_A(18, 24));
      VRAM_BUFFER[k + 2] = STR_LEN_LEVEL;
      VRAM_BUFFER[k + 3] = INT_TO_CHR(level / 10);
      VRAM_BUFFER[k + 4] = INT_TO_CHR(level % 10);
      VRAM_BUFFER[k + 5] = NT_UPD_EOF;
      vram_buffer_flags = VRAM_FLAGS_DRAW;
    }
    /* VRAM BUFFER UPDATE END */
    
    //nmi sync
    ppu_wait_nmi();
    
    //Read pad
    pad = pad_trigger(0);
    if (pad & PAD_START) {
      state = STATE_START;
      break;
    } else if (pad & PAD_UP) {
      score++;
    } else if (pad & PAD_DOWN) {
      score--;
    }
  }
  
  //State exit
  //TODO
}

void main(void) {
  
  nmi_set_callback(nmi_handler);
  ppu_on_all();
  
  state = STATE_GAME;
  
  // infinite loop
  while (1) {
    if (state == STATE_START) {
    	state_start_loop();
    } else if (state == STATE_GAME) {
      	state_game_loop();
    }
  }
}
