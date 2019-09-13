#include "neslib.h"

//#link "chr_generic.s"

#include "screen_start.h"
#include "screen_game.h"

//System vars
static unsigned char VRAM_BUFFER[32];
static unsigned char oam_buffer;
static unsigned char pad;

//strings
#define STR_LEN_PRESS_START 12
static char* str_pressstart = "PRESS START";
#define STR_LEN_GAME_OVER 9
static char* str_gameover = "GAME OVER";
#define STR_LEN_GAME_PAUSE 11
static char* str_pause = "GAME PAUSED";

//Palettes
const unsigned char sprites_pal[16] = {
    0x20, 0x06, 0x38, 0x27,
    0x0C, 0x19, 0x29, 0x39,
    0x0C, 0x1C, 0x2C, 0x3C,
    0x0C, 0x14, 0x24, 0x34
};

const unsigned char bg_pal[16] = {
    0x0F, 0x0B, 0x21, 0x2A,
    0x0C, 0x19, 0x29, 0x39,
    0x0C, 0x1C, 0x2C, 0x3C,
    0x0C, 0x14, 0x24, 0x34
};

//states
#define STATE_START 	0
#define STATE_GAME  	1
#define STATE_GAME_OVER 2
static unsigned char state = STATE_START;

//game vars
#define STR_LEN_SCORE 4
#define STR_LEN_LEVEL 2

#define LEVEL_X 8
#define LEVEL_Y 6
#define LEVEL_SIZE 15

#define SNAKE_MAX_SIZE 64
#define snake_head snake[0]
#define SNAKE_SPAWN_X 14
#define SNAKE_SPAWN_Y 14
static unsigned char snake[SNAKE_MAX_SIZE][2];
static unsigned char snake_clear_coords[2];
static unsigned char snake_dx, snake_dy, snake_size;

#define LEVEL_MAX 10
#define LEVEL_SPEED  level_speed[level]
#define LEVEL_PILLS  level + 1
#define LEVEL_POINTS level + 1
static unsigned char level_speed[] = {12, 11, 10, 9, 8, 
                                      7 ,  6,  5, 4, 3};

#define GAME_FLAGS_CLEAR 	0x00
#define GAME_FLAGS_GROW  	0x01
#define GAME_FLAGS_SPAWN_PILLS	0x02
#define GAME_FLAGS_PAUSE	0x04
static unsigned char game_flags = 0x00;
static unsigned int score;
static unsigned char pills[16][2];
static unsigned char level;
static unsigned int i, j, k;

#define SPRITE_SNAKE 0xC2
#define SPRITE_PILL  0x18
#define PILLS_PALLETTE 0

//utils
#define LERP(_A, _B, _PCT) (_A + ((_B - _A) * _PCT))
#define INT_TO_CHR(_X) 0x30 + _X
#define BG_X_TO_SPRITE(_X) _X * 8
#define BG_Y_TO_SPRITE(_Y) _Y * 8 - 1


void nmi_handler()
{
  //TODO: update music here
}


void load_screen(const unsigned char* data)
{
  ppu_off();
  
  vram_adr(NAMETABLE_A);
  vram_unrle(data);
  
  /*
  pal_col(0,0x02);
  pal_col(1,0x14);
  pal_col(2,0x20);
  pal_col(3,0x30);
  */
  pal_spr(sprites_pal);
  pal_bg(bg_pal);
  
  ppu_on_all();
}


void state_start_loop()
{
  //State start
  load_screen(screen_start);
  
  k = 0;
  VRAM_BUFFER[k++] = MSB(NTADR_A(10, 20))|NT_UPD_HORZ;
  VRAM_BUFFER[k++] = LSB(NTADR_A(10, 20));
  VRAM_BUFFER[k++] = STR_LEN_PRESS_START;
  for (i = 0; i < STR_LEN_PRESS_START; i++) {
    VRAM_BUFFER[k++] = str_pressstart[i];
  }
  VRAM_BUFFER[k++] = NT_UPD_EOF;
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
  
  //Game startup
  score = 0;
  level = 0xFF;
  snake_dx = 1;
  snake_dy = 0;
  snake_size = 1;
  snake_head[0] = SNAKE_SPAWN_X;
  snake_head[1] = SNAKE_SPAWN_Y;
  
  game_flags = GAME_FLAGS_CLEAR;
  game_flags |= GAME_FLAGS_SPAWN_PILLS;
  
  
  while(1) {
       
    if (!(game_flags & GAME_FLAGS_PAUSE)) {
      
      /* LOGIC */
      if (game_flags & GAME_FLAGS_SPAWN_PILLS) {
        game_flags &= ~GAME_FLAGS_SPAWN_PILLS;
        level = level != 0xFF ? MIN(level + 1, LEVEL_MAX) : 0;
        for (i = 0; i < LEVEL_PILLS; i++) {
           pills[i][0] = (int)(LERP(LEVEL_X, LEVEL_X + LEVEL_SIZE, rand8() / 255));
           pills[i][1] = (int)(LERP(LEVEL_Y, LEVEL_Y + LEVEL_SIZE, rand8() / 255));
        }
      }

      //Eat pill
      for (i = 0; i < LEVEL_PILLS; i++) {
        if (snake_head[0] == pills[i][0] && snake_head[1] == pills[i][1]) {
          game_flags |= GAME_FLAGS_GROW;
          pills[i][0] = 0xFF;
          pills[i][1] = 0xFF;
          break;
        }
      }

      //Grow snake
      if (game_flags & GAME_FLAGS_GROW) {
        game_flags &= ~GAME_FLAGS_GROW;
        i = snake_size;
        snake_size = MIN(snake_size + 1, SNAKE_MAX_SIZE);
        if (i != snake_size) {
          snake[snake_size - 1][0] = snake[snake_size - 2][0];
          snake[snake_size - 1][1] = snake[snake_size - 2][1];
        }
        score += LEVEL_POINTS;

        game_flags |= GAME_FLAGS_SPAWN_PILLS;
        for (i = 0; i < LEVEL_PILLS; i++) {
          if (pills[i][0] != 0xFF) {
            game_flags &= ~GAME_FLAGS_SPAWN_PILLS;
            break;
          }
        }
      }

      //Move snake
      if (nesclock() % LEVEL_SPEED == 0) {
          snake_clear_coords[0] = snake[snake_size - 1][0];
          snake_clear_coords[1] = snake[snake_size - 1][1];
          for (i = snake_size - 1; i > 0; --i) {
              snake[i][0] = snake[i-1][0];
              snake[i][1] = snake[i-1][1];
          }
          snake_head[0] += snake_dx;
          snake_head[1] += snake_dy;
      }

      //Game over check
      i = 0;
      i = i || (snake_head[0] < LEVEL_X);
      i = i || (snake_head[0] > LEVEL_X + LEVEL_SIZE);
      i = i || (snake_head[1] < LEVEL_Y);
      i = i || (snake_head[1] > LEVEL_Y + LEVEL_SIZE);
      for (j = 2; j < snake_size; j++) {
          i = i || (snake[j][0] == snake_head[0] &&  snake[j][1] == snake_head[1]);
          if (i != 0) {
              break;
          }
      }
      if (i) {
        state = STATE_GAME_OVER;
        break;
      }
    }
    
    /* DRAW */
    
    //VRAM update - clear last coords
    k = 0;
    VRAM_BUFFER[k++] = MSB(NTADR_A(snake_clear_coords[0], snake_clear_coords[1]));
    VRAM_BUFFER[k++] = LSB(NTADR_A(snake_clear_coords[0], snake_clear_coords[1]));
    VRAM_BUFFER[k++] = 0x00;
    
    //VRAM update - snake head
    VRAM_BUFFER[k++] = MSB(NTADR_A(snake_head[0], snake_head[1]));
    VRAM_BUFFER[k++] = LSB(NTADR_A(snake_head[0], snake_head[1]));
    VRAM_BUFFER[k++] = SPRITE_SNAKE;
    
    //VRAM update - score
    VRAM_BUFFER[k++] = MSB(NTADR_A(21, 3))|NT_UPD_HORZ;
    VRAM_BUFFER[k++] = LSB(NTADR_A(21, 3));
    VRAM_BUFFER[k++] = STR_LEN_SCORE;
    i = score;
    for (j = STR_LEN_SCORE; j > 0; j--) {
      VRAM_BUFFER[k + j - 1] = INT_TO_CHR(i % 10);
      i = i / 10;
    }
    k = k + STR_LEN_SCORE;
    
    //VRAM udpate - level
    VRAM_BUFFER[k++] = MSB(NTADR_A(18, 24))|NT_UPD_HORZ;
    VRAM_BUFFER[k++] = LSB(NTADR_A(18, 24));
    VRAM_BUFFER[k++] = STR_LEN_LEVEL;
    VRAM_BUFFER[k++] = INT_TO_CHR((level + 1) / 10);
    VRAM_BUFFER[k++] = INT_TO_CHR((level + 1) % 10);
    
    //VRAM update - pause
    VRAM_BUFFER[k++] = MSB(NTADR_A(11, 26))|NT_UPD_HORZ;
    VRAM_BUFFER[k++] = LSB(NTADR_A(11, 26));
    VRAM_BUFFER[k++] = STR_LEN_GAME_PAUSE;
    for (i = 0; i < STR_LEN_GAME_PAUSE; i++) {
      VRAM_BUFFER[k++] = (game_flags & GAME_FLAGS_PAUSE) ? str_pause[i] : 0x00;
    }
    VRAM_BUFFER[k++] = NT_UPD_EOF;
    ppu_wait_nmi();
    
    //Sprites - pills
    oam_clear();
    oam_buffer = 0;
    for (i = 0; i < LEVEL_PILLS; i++) {
      if (pills[i][0] == 0xFF) continue;
      oam_buffer = oam_spr(BG_X_TO_SPRITE(pills[i][0]), 
                           BG_Y_TO_SPRITE(pills[i][1]), 
                           SPRITE_PILL, 
                           PILLS_PALLETTE, 
                           oam_buffer);
    }
    
    /* INPUT */
    
    //Read pad
    pad = pad_trigger(0);
    if (pad & PAD_A) {
      game_flags = game_flags | GAME_FLAGS_GROW;
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
    } else if (pad & PAD_START) {
      if (game_flags & GAME_FLAGS_PAUSE) {
        game_flags &= ~GAME_FLAGS_PAUSE;
      } else {
        game_flags |= GAME_FLAGS_PAUSE;
      }
    }
  }
  
  //State exit
  set_vram_update(NULL);
}


void state_game_over()
{
  set_vram_update(VRAM_BUFFER);
  oam_clear();
  
  k = 0;
  VRAM_BUFFER[k++] = MSB(NTADR_A(11, 18))|NT_UPD_HORZ;
  VRAM_BUFFER[k++] = LSB(NTADR_A(11, 18));
  VRAM_BUFFER[k++] = STR_LEN_GAME_OVER;
  for (i = 0; i < STR_LEN_GAME_OVER; i++) {
    VRAM_BUFFER[k++] = str_gameover[i];
  }
  VRAM_BUFFER[k++] = NT_UPD_EOF;
  set_vram_update(VRAM_BUFFER);
  j = 0;
  
  //State update
  while(1) {
    
    //Logic
    //TBD
    
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

void main(void) {
  
  nmi_set_callback(nmi_handler);
  ppu_on_all();
  
  state = STATE_START;
  
  // infinite loop
  while (1) {
    if (state == STATE_START) {
    	state_start_loop();
    } else if (state == STATE_GAME) {
      	state_game_loop();
    } else if (state == STATE_GAME_OVER) {
    	state_game_over();
    }
  }
}
