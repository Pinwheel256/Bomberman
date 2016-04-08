/*--------------------------------------------------
 * Name:    bomberman.h
 * Purpose: bomberman function prototypes and defs
 *--------------------------------------------------
 *
 * Modification History
 * 26-03-2016 Created
 * 26-03-2016 Updated (uVision5.17 + DFP2.6.0)
 *
 * Author: Jack Dean
 *--------------------------------------------------*/
#ifndef _BOMBERMAN_H
#define _BOMBERMAN_H

#include <stdbool.h>
#include "Board_Touch.h"
#include "cmsis_os.h"
#include "GLCD_Config.h"
#include "Board_GLCD.h"
#include <stdlib.h>
#include <stdio.h> 
#include <time.h>

#define wait_delay HAL_Delay
#define WIDTH		GLCD_WIDTH
#define HEIGHT	GLCD_HEIGHT
#define CHAR_H  GLCD_Font_16x24.height                  /* Character Height (in pixels) */
#define CHAR_W  GLCD_Font_16x24.width                  /* Character Width (in pixels)  */
#define BAR_W   6									  /* Bar Width (in pixels) */
#define BAR_H		24				          /* Bar Height (in pixels) */
#define T_LONG	1000                /* Long delay */
#define T_SHORT 5                   /* Short delay */

/*--------------------------------------------------
 *      Constants - Jack Dean
 *--------------------------------------------------*/
#define GRID_X  130
#define GRID_Y  0
#define TILE_SIZE  18
#define ROWS  15
#define COLS 15
#define ENEMY_NUM 6
#define SOLID_COL GLCD_COLOR_BLUE
#define WEAK_COL GLCD_COLOR_GREEN
#define FLOOR_COL GLCD_COLOR_WHITE
#define PLAYER_COL GLCD_COLOR_YELLOW
#define ENEMY_COL 0x004fff
#define BOMB_COL GLCD_COLOR_BLACK
#define EXPLO_COL GLCD_COLOR_RED

/*--------------------------------------------------
 *      Enum types and struct declarations - Jack Dean
 *--------------------------------------------------*/

typedef enum {SOLID, WEAK, FLOOR} tile_type;
typedef enum {EMPTY, PLAYER, BOMB, ENEMY} object_type;

typedef struct Enemy Enemy;
typedef struct Tile Tile;

struct Tile {
  tile_type type;	
  int x;				
	int y;					
	bool hasPlayer;
	bool hasEnemy;
	bool hasBomb;
	Enemy* enemy;	
};

struct Enemy {
	//int x;					// not used
	//int y;					// not used
	bool alive;
  Tile* tile;
};

typedef struct {
	int x; 
	int y;
	Tile* tile;
} Player;

typedef struct {
	int x;
	int y;
  Tile* tile;
	int level;
} Bomb;

typedef struct Game {
	unsigned int num_ticks;		// could be used to control enemy speed
	Player player;
	Enemy enemies[ENEMY_NUM];
	Bomb bomb;
	Tile tiles[ROWS][COLS];		// 2 dimensional array, represents y and x coordinates within a rectangular map of tiles
} Game;

// mailbox struct
typedef struct{
	int paddleValue;
} mail_t;

/*--------------------------------------------------
 *      Function prototypes - Jack Dean
 *--------------------------------------------------*/
void game_init(void);
void drawUI(void);
void placeEnemies(void);
void movePlayer(int i);
void updatePlayer(Tile* tile, int xChange, int yChange);
void updateEnemy(Tile* tile, Enemy* enemy, int xChange, int yChange);
void playerAI(void);
void moveEnemies(void);
bool checkCollision(void);
void startBomb(void);
void dropBomb(void);
void bombExplode(void);
void controls(void);
void updateDisplay(TOUCH_STATE*);
void clearDisplay(void);
void update_ball (void);
void update_player(int);
void check_collision(void);
void drawChar(int x, int y, int color);
void drawBitmap(int x, int y, int width, int height, const unsigned char *bitmap);
extern unsigned int GLCD_RLE_Bitmap (unsigned int x, unsigned int y, unsigned int width, unsigned int height, const unsigned char *bitmap);

#endif /* _BOMBERMAN_H */
