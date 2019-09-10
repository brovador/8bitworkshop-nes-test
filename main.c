#include "neslib.h"

//#link "chr_generic.s"

#include "screen_start.h"
#include "screen_game.h"

//System vars
static unsigned char VRAM_BUFFER[32];
static unsigned char pad;

//strings
#define STR_LEN_PRESS_START 13
static char* str_pressstart = "PRESS START";

//states
#define STATE_START 0
#define STATE_GAME  1
static unsigned char state = STATE_START;

//game vars
#define STR_LEN_SCORE 4
#define STR_LEN_LEVEL 2

#define LEVEL_X 8
#define LEVEL_Y 6
#define LEVEL_SIZE 15

#define SNAKE_MAX_SIZE 35
#define snake_head snake_coords[0]
#define SNAKE_SPAWN_X 14
#define SNAKE_SPAWN_Y 14
static unsigned char snake_coords[SNAKE_MAX_SIZE][2];
static unsigned char snake_dx, snake_dy, snake_size;


static unsigned int score;
static unsigned char level;
static unsigned int i, j, k;

//utils
#define INT_TO_CHR(_X) 0x30 + _X


void nmi_handler()
{
  //TODO: update music here
}


void load_screen(const unsigned char* data)
{
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
  set_vram_update(VRAM_BUFFER);
  j = 0;
  
  //State update
  while(1) {
    
    //Logic suff
    if (nesclock() % 32 == 0) {
      j += 1;
      for (i = 0; i < STR_LEN_PRESS_START; i++) {
        VRAM_BUFFER[3 + i] = (j % 2) ? str_pressstart[i] : ' ';
      }
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
  set_vram_update(NULL);
}


void state_game_loop()
{
  load_screen(screen_game);
  set_vram_update(VRAM_BUFFER);
  
  score = 0000;
  level = 00;
  
  snake_dx = 1;
  snake_dy = 0;
  snake_size = 1;
  snake_head[0] = SNAKE_SPAWN_X;
  snake_head[1] = SNAKE_SPAWN_Y;
  
  
  while(1) {
    
    //State logic
    k = 0;
    VRAM_BUFFER[k] = MSB(NTADR_A(snake_head[0], snake_head[1]));
    VRAM_BUFFER[k + 1] = LSB(NTADR_A(snake_head[0], snake_head[1]));
    VRAM_BUFFER[k + 2] = 0x00;
    k = k + 3;
    
    if (nesclock() % 16 == 0) {
    	snake_head[0] += snake_dx;
	snake_head[1] += snake_dy;
     	if (snake_head[0] < LEVEL_X || snake_head[0] > LEVEL_X + LEVEL_SIZE) {
          snake_head[0] = SNAKE_SPAWN_X;
        }
      	if (snake_head[1] < LEVEL_Y || snake_head[1] > LEVEL_Y + LEVEL_SIZE) {
          snake_head[1] = SNAKE_SPAWN_Y;
        }
    }
    
    /* VRAM BUFFER UPDATE */
    
    //VRAM update - score
    VRAM_BUFFER[k] = MSB(NTADR_A(21, 3))|NT_UPD_HORZ;
    VRAM_BUFFER[k + 1] = LSB(NTADR_A(21, 3));
    VRAM_BUFFER[k + 2] = STR_LEN_SCORE;
    i = score;
    for (j = STR_LEN_SCORE; j > 0; j--) {
      VRAM_BUFFER[k + 3 + j - 1] = INT_TO_CHR(i % 10);
      i = i / 10;
    }
    k = k + 3 + STR_LEN_SCORE;
    
    //VRAM udpate - level
    VRAM_BUFFER[k] = MSB(NTADR_A(18, 24))|NT_UPD_HORZ;
    VRAM_BUFFER[k + 1] = LSB(NTADR_A(18, 24));
    VRAM_BUFFER[k + 2] = STR_LEN_LEVEL;
    VRAM_BUFFER[k + 3] = INT_TO_CHR(level / 10);
    VRAM_BUFFER[k + 4] = INT_TO_CHR(level % 10);
    k = k + 5;
    
    VRAM_BUFFER[k] = MSB(NTADR_A(snake_head[0], snake_head[1]));
    VRAM_BUFFER[k + 1] = LSB(NTADR_A(snake_head[0], snake_head[1]));
    VRAM_BUFFER[k + 2] = 0x01;
    VRAM_BUFFER[k + 3] = NT_UPD_EOF;
    
    //nmi sync
    ppu_wait_nmi();
    
    //Read pad
    pad = pad_trigger(0);
    if (pad & PAD_START) {
      state = STATE_START;
      break;
    } else if (pad & PAD_UP) {
      snake_dy = -1;
      snake_dx = 0;
    } else if (pad & PAD_DOWN) {
      snake_dy = 1;
      snake_dx = 0;
    } else if (pad & PAD_LEFT) {
      snake_dy = 0;
      snake_dx = -1;
    } else if (pad & PAD_RIGHT) {
      snake_dy = 0;
      snake_dx = 1;
    }
  }
  
  //State exit
  set_vram_update(NULL);
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
