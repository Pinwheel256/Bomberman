/*--------------------------------------------------
 * Recipe:  Bomberman
 * Name:    bomb_utils.c
 * Purpose: Bomberman Prototype
 *--------------------------------------------------
 *
 * Modification History
 * 26-03-2016 Created
 * 26-03-2016 Updated (uVision5.17 + DFP2.6.0)
 *
 * Author: Jack Dean
 *--------------------------------------------------*/

#include "bomberman.h"
#include "bitmaps.h"
#include "stm32f7xx_hal.h"
#include "cmsis_os.h"
#include "Board_ADC.h"
#include "GLCD_Config.h"
#include "Board_GLCD.h"
#include "Board_Touch.h"
#include <stdlib.h>
#include <stdio.h> 
#include <time.h>

/* Globals */
TOUCH_STATE  tsc_state;
extern GLCD_FONT     GLCD_Font_16x24;

Game game;

/*--------------------------------------------------
 *      Initialise game - Jack Dean
 *---------------------------------------- ----------*/
void initGame(void) 
{							
	game.player.lives = 2;
	game.bomb.power = 2;
	game.stage = 1;
	showStartScreen();
}

/*--------------------------------------------------
 *      Show start screen - Jack Dean
 *---------------------------------------- ----------*/
void showStartScreen(void) 
{	
	// print game info 
	GLCD_SetBackgroundColor(GLCD_COLOR_BLACK);
	GLCD_ClearScreen();
	GLCD_SetForegroundColor(GLCD_COLOR_WHITE);
	GLCD_DrawString (150, 50, "Bomberman");
	GLCD_DrawString (150, 150, "PRESS to START");
	
	// wait for user input
	while (true)
	{ 
		Touch_GetState (&tsc_state);  // get touch state
		
		if (tsc_state.pressed)
		{
			break;
		}
	}
	
	initStage();	// start first stage
}

/*--------------------------------------------------
 *      Initialise stage - Jack Dean
 *--------------------------------------------------*/
void initStage(void)
{
	int x;								// tile x coord
	int y;								// tile y coord
	int tileNum = 0;			// tile number	
	int randNum;
	
	// display stage info
	showStageScreen();
	
	// set game background
	GLCD_SetBackgroundColor(GLCD_COLOR_BLACK);
  GLCD_ClearScreen(); 
	
	// draw UI and control areas
	drawUI();
	
	// initialise and draw game level	
	for (y = 0; y < ROWS; y++)					// iterate through tiles
	{
		for (x = 0; x < COLS; x++)
		{										
			// initiliase each tile's field values
			Tile* tile = &game.tiles[y][x];
			tile->x = x;
			tile->y = y;
			tile->hasPlayer = false;
			tile->hasEnemy = false;
			tile->hasBomb = false;
			tile->enemy = NULL;								
			
			// place solid tiles
			if (x == 0 || x == COLS-1 || y == 0 || y == ROWS-1 || (y%2 == 0 && x%2 == 0))
			{				
				tile->type = SOLID;												
				drawBitmap(x * TILE_SIZE, y * TILE_SIZE, solid_comp.width, solid_comp.height, solid_comp.rle_pixel_data);
			}
			else if ((y > 2 || x > 2))		// if tile is not adjacent to player start position
			{
				//srand((unsigned)time(NULL));	// needed to be truly random but crashes program..?
				randNum = rand() % 10;		
				if (randNum > 6)		// randomly place weak tiles
				{
					tile->type = WEAK;
					drawBitmap(x * TILE_SIZE, y * TILE_SIZE, weak_comp.width, weak_comp.height, weak_comp.rle_pixel_data);
				}
				else		// place floor tiles
				{
					tile->type = FLOOR;
					tile->hasEnemy = false;
					drawBitmap(x * TILE_SIZE, y * TILE_SIZE, floor_comp.width, floor_comp.height, floor_comp.rle_pixel_data);
				}
			}
			else		// place floor and weak tiles at player starting position
			{
				tile->type = FLOOR;
				tile->hasEnemy = false;
				drawBitmap(x * TILE_SIZE, y * TILE_SIZE, floor_comp.width, floor_comp.height, floor_comp.rle_pixel_data);
			}
			
			// keep track of tile number
			tileNum++;
		}
	}			
	
	// place player
	game.player.x = 1;
	game.player.y = 1;
	game.player.tile = &game.tiles[1][1];
	game.player.tile->hasPlayer = true;
	drawBitmap(1 * TILE_SIZE, 1 * TILE_SIZE, bomberman_comp.width, bomberman_comp.height, bomberman_comp.rle_pixel_data);
	
	placeEnemies();
	placeObjects();
	
	// start enemy movement thread
	game.playing = true;
	startEnemies();
}

/*--------------------------------------------------
 *      Show stage screen - Jack Dean
 *---------------------------------------- ----------*/
void showStageScreen(void) 
{	
	char str[] = "Stage ";	
	char stageStr[2];
	int stage;	
	
	// concatenate level number to level info string
	stage = game.stage;
  sprintf(stageStr, "%d", stage);
	strcpy(str, strcat(str, stageStr)); 
		
	// print level info
	GLCD_SetBackgroundColor(GLCD_COLOR_BLACK);
	GLCD_ClearScreen();
	GLCD_SetForegroundColor(GLCD_COLOR_WHITE);
	GLCD_DrawString (150, 50, str);
	GLCD_DrawString (150, 150, "GET READY!");
	
	// delay before starting level
	osDelay(2000);
}

/*--------------------------------------------------
 *      Place enemies - Jack Dean
 *--------------------------------------------------*/
void placeEnemies(void)
{
	int i;
	int y;
	int x;
	int randNum;
	Tile* tile;
	
	// variables for RNG 
	int pool[225];				// pool of possible tile numbers
	int poolSize = 225;		// pool size

	// init pool
	for (i = 0; i < 225; i++)
	{
		pool[i] = i;
	}
	
	// store enemy tile locations (Fisher-Yates Shuffle algorithm)																			
	for (i = 0; i < 6; i++)
	{		
		// get random tile
		do 
		{
			randNum = pool[rand() % poolSize];
			y = randNum / 15;
			x = randNum % 15;
			tile = &game.tiles[y][x];
		}
		while ((tile->type != FLOOR) || (tile->y < 5));		// ensures floor tile and distance from player
		
		game.enemies[i].alive = true;
		game.enemies[i].tile = tile;						
		tile->enemy = &game.enemies[i];
		tile->hasEnemy = true;		
		drawBitmap(x * TILE_SIZE, y * TILE_SIZE, enemy_comp.width, enemy_comp.height, enemy_comp.rle_pixel_data);			
		
		// shuffle pool
		poolSize--;
		pool[randNum] = pool[poolSize];
	}	
}

/*--------------------------------------------------
 *      Place objects - Jack Dean
 *--------------------------------------------------*/
void placeObjects(void)
{
	int y;
	int x;
	int randNum;
	Tile* tile;
	
	// place door
	do 
	{
		randNum = rand() % (ROWS * COLS);
		y = randNum / 15;
		x = randNum % 15;
		tile = &game.tiles[y][x];
	}
	while (tile->type != WEAK);		// ensures weak tile
								
	tile->object = DOOR;			
		
	// place powerup
	do 
	{
		randNum = rand() % (ROWS * COLS);
		y = randNum / 15;
		x = randNum % 15;
		tile = &game.tiles[y][x];
	}
	while (tile->type != WEAK && tile->object != DOOR);	// ensures weak and not door
								
	tile->object = POWERUP;		
}	

/*--------------------------------------------------
 *      Move player - Jack Dean
 *--------------------------------------------------*/
void movePlayer(int i)
{
	Tile* tile = game.player.tile;
	
	switch (i)
	{
		case 0:
			if (game.tiles[tile->y][tile->x+1].type == FLOOR)		// check tile moving to is FLOOR tile
			{				
				updatePlayer(tile, +1, 0);				
			}												
			break;
		case 1:
			if (game.tiles[tile->y+1][tile->x].type == FLOOR)
			{
				updatePlayer(tile, 0, +1);				
			}
			break;
		case 2:
			if (game.tiles[tile->y][tile->x-1].type == FLOOR)
			{
				updatePlayer(tile, -1, 0);				
			}
			break;
		case 3:
			if (game.tiles[tile->y-1][tile->x].type == FLOOR)
			{
				updatePlayer(tile, 0, -1);								
			}
			break;
		default:	// do nothing
			break;
	}
	
	// check for object
	if (game.player.tile->object == DOOR)
	{
		// restart game
		GLCD_ClearScreen(); 
		initGame();
	}
	else if (game.player.tile->object == POWERUP)
	{
		game.bomb.power++;
	}
	
	// check player collision with enemies
	if (game.player.tile->hasEnemy == true)
	{
		loseLife();
	}	
}

/*--------------------------------------------------
 *      Update player - Jack Dean
 *--------------------------------------------------*/
void updatePlayer(Tile* tile, int xChange, int yChange)
{	
	int i;
	
	for (i = 0; i < TILE_SIZE; i++)
	{		
		if (tile->hasBomb)		
		{	
			// redraw floor
			drawBitmap((tile->x * TILE_SIZE) + (i * xChange), 
										(tile->y * TILE_SIZE) + (i * yChange), 
										floor_comp.width, floor_comp.height, floor_comp.rle_pixel_data);
			
			// then redraw bomb
			drawBitmap(tile->x * TILE_SIZE, tile->y * TILE_SIZE, bomb_comp.width, bomb_comp.height, bomb_comp.rle_pixel_data);

		}
		else
		{
			// redraw floor
			drawBitmap((tile->x * TILE_SIZE) + (i * xChange), 
										(tile->y * TILE_SIZE) + (i * yChange), 
										floor_comp.width, floor_comp.height, floor_comp.rle_pixel_data);	
		}
		
		// finally, redraw player
		drawBitmap((tile->x * TILE_SIZE) + ((i+1) * xChange), 
										 (tile->y * TILE_SIZE) + ((i+1) * yChange),
											bomberman_comp.width,
											bomberman_comp.height,
											bomberman_comp.rle_pixel_data);
		
		// movement speed
		osDelay(5);	
	}

	// change tile statuses
	tile->hasPlayer = false;					
	game.player.tile = &game.tiles[tile->y + yChange][tile->x + xChange];							
	game.player.tile->hasPlayer = true;
	
	// update player location
	game.player.x += xChange;
	game.player.y += yChange;	
}

/*--------------------------------------------------
 *      Move enemies - Jack Dean
 *--------------------------------------------------*/
void moveEnemies(void)
{
	int i;
	int randomnumber;
	
	while (true)
	{
		// for each enemy in game
		for (i = 0; i < ENEMY_NUM; i++)
		{	
			if (game.playing == true)
			{
				if (game.enemies[i].alive)
				{			
					Tile* tile = game.enemies[i].tile;		// temp store enemy tile for easier field access
					Enemy* enemy = &game.enemies[i];
					
					// randomly choose movement direction
					randomnumber = rand() % 4;
					
					switch (randomnumber)
					{
						case 0:
							if ((game.tiles[tile->y][tile->x+1].type == FLOOR) && 
								 (!game.tiles[tile->y][tile->x+1].hasBomb) && 
								 (!game.tiles[tile->y][tile->x+1].hasEnemy) &&
								 (game.tiles[tile->y][tile->x+1].object == EMPTY)) // check tile is empty
							{
								updateEnemy(tile, enemy, 1, 0);																		
							}												
							break;
						case 1:
							if ((game.tiles[tile->y+1][tile->x].type == FLOOR) && 
								 (!game.tiles[tile->y+1][tile->x].hasBomb) && 
								 (!game.tiles[tile->y+1][tile->x].hasEnemy) &&
								 (game.tiles[tile->y+1][tile->x].object == EMPTY)) // check tile is empty
							{
								updateEnemy(tile, enemy, 0, 1);
							}
							break;
						case 2:
							if ((game.tiles[tile->y][tile->x-1].type == FLOOR) && 
								 (!game.tiles[tile->y][tile->x-1].hasBomb) && 
								 (!game.tiles[tile->y][tile->x-1].hasEnemy) &&
								 (game.tiles[tile->y][tile->x-1].object == EMPTY)) // check tile is empty
							{
								updateEnemy(tile, enemy, -1, 0);								
							}
							break;
						case 3:
							if ((game.tiles[tile->y-1][tile->x].type == FLOOR) && 
								 (!game.tiles[tile->y-1][tile->x].hasBomb) && 
								 (!game.tiles[tile->y-1][tile->x].hasEnemy) &&
								 (game.tiles[tile->y-1][tile->x].object == EMPTY)) // check tile is empty
							{
								updateEnemy(tile, enemy, 0, -1);
							}
							break;
						default:	// do nothing
							break;
					}
					
					// check enemy collision with player
					if (game.enemies[i].tile->hasPlayer == true)
					{
						loseLife();			
					}				
				}	
			}
		}
	}	
}

/*--------------------------------------------------
 *      Update enemy - Jack Dean
 *--------------------------------------------------*/
void updateEnemy(Tile* tile, Enemy* enemy, int xChange, int yChange)
{
	int i;
	
	for (i = 0; i < TILE_SIZE; i++)
	{		
		drawBitmap((tile->x * TILE_SIZE) + (i * xChange), 
										(tile->y * TILE_SIZE) + (i * yChange), 
										floor_comp.width, floor_comp.height, floor_comp.rle_pixel_data);

		
		drawBitmap((tile->x * TILE_SIZE) + ((i+1) * xChange), 
		               (tile->y * TILE_SIZE) + ((i+1) * yChange), 
										enemy_comp.width,
										enemy_comp.height,
										enemy_comp.rle_pixel_data);		
		osDelay(15);
	}
	
	// change tile statuses
	enemy->tile = &game.tiles[tile->y + yChange][tile->x + xChange];		// set enemy's tile pointer to new tile
	enemy->tile->enemy = enemy;																			// set new tile's enemy pointer to this enemy
	enemy->tile->hasEnemy = true;
	tile->hasEnemy = false;													
	tile->enemy = NULL;																				// set old tile's enemy pointer to null				
}

/*--------------------------------------------------
 *      Controls listener - Jack Dean
 *--------------------------------------------------*/
void controls(void)
{
	while (true)
	{
		Touch_GetState (&tsc_state);  // get touch state
		
		if (tsc_state.pressed)
		{			
			if ((tsc_state.x > 90 && tsc_state.x < 120) &&
				  (tsc_state.y > 110 && tsc_state.y < 170))
			{
				updateDisplay(&tsc_state);
				movePlayer(0);
			}
			else if ((tsc_state.x > 30 && tsc_state.x < 90) &&
							 (tsc_state.y > 160 && tsc_state.y < 190))
			{
				updateDisplay(&tsc_state);
				movePlayer(1);
			}
			else if ((tsc_state.x > 0 && tsc_state.x < 30) &&
							 (tsc_state.y > 110 && tsc_state.y < 170))
			{
				updateDisplay(&tsc_state);
				movePlayer(2);
			}
			else if ((tsc_state.x > 30 && tsc_state.x < 90) &&
							 (tsc_state.y > 90 && tsc_state.y < 120))
			{
				updateDisplay(&tsc_state);
				movePlayer(3);
			}
			else if ((tsc_state.x > 390 && tsc_state.x < 430) &&
							 (tsc_state.y > 110 && tsc_state.y < 150))
			{
				updateDisplay(&tsc_state);

				// start bomb thread
				startBomb();				
			}
			else
			{
				clearDisplay();				
			}			
		}
		else
		{
			clearDisplay();
		}
	}
}

/*--------------------------------------------------
 *      Drop bomb - Jack Dean
 *--------------------------------------------------*/
void dropBomb(void)
{
	// set bomb location
	game.bomb.tile = game.player.tile;
	game.bomb.tile->hasBomb = true;
	game.bomb.x = game.player.x;
	game.bomb.y = game.player.y;
	
	// draw bomb
	drawBitmap(game.player.x * TILE_SIZE, game.player.y * TILE_SIZE, bomb_comp.width, bomb_comp.height, bomb_comp.rle_pixel_data);	
}

/*--------------------------------------------------
 *      Bomb Explode - Jack Dean
 *--------------------------------------------------*/
void bombExplode(void)
{
	int i;
	int bombX = game.bomb.x;
	int bombY = game.bomb.y;	
	Tile* tiles[20];												   // store vulnerable tiles in array
	int directions[4][2] = {{0, +1}, {+1, 0},  // hardcode explosion directions
												{0, -1}, {-1, 0}};
	int dirIndex = 0;
	int offset = 1;			// distance from centre tile
	int numTiles = 0;
	
	// add centre tile
	tiles[0] = game.bomb.tile;
	numTiles++;
	
	// add vulnerable tiles for each direction
	for (i = 1; dirIndex < 4; i++)
	{			
		// temp store tile
		Tile* tile = &game.tiles[bombY + (offset * directions[dirIndex][0])]
														[bombX + (offset * directions[dirIndex][1])];
		
		// check tile is vulnerable and within reach
		if (tile->type != SOLID && offset <= game.bomb.power)
		{			
			// add tile
			tiles[i] = tile;
			offset++;
			numTiles++;
		}
		else
		{
			offset = 1;		// reset offset 
			dirIndex++;		// use next set of coords
			i--;					// tile not initialised so reuse same index
		}		
	}
	
	// remove bomb
	game.bomb.tile->hasBomb = false;	
	
	if (game.playing == true)
	{
		// process explosions
		for (i = 0; i < numTiles; i++)
		{
			// draw explosion
			drawBitmap(tiles[i]->x * TILE_SIZE, 
										tiles[i]->y * TILE_SIZE, 
										explo_comp.width, explo_comp.height, explo_comp.rle_pixel_data);								
						
			if (tiles[i]->hasPlayer)		// check player collision
			{					
				loseLife();
				break;
			}
			else if (tiles[i]->hasEnemy)		// check enemy collision
			{
				// kill enemy
				tiles[i]->enemy->alive = false;			  
				tiles[i]->enemy->tile->hasEnemy = false;
				tiles[i]->enemy->tile->enemy = NULL;
			}
		}
		
		// let explosions linger
		osDelay(800);
	}
	
	if (game.playing == true)
	{
		// clear explosions
		for (i = 0; i < numTiles; i++)		
		{
			if (tiles[i]->type == FLOOR)
			{
				drawBitmap(tiles[i]->x * TILE_SIZE, 
											tiles[i]->y * TILE_SIZE, 
											floor_comp.width, floor_comp.height, floor_comp.rle_pixel_data);
			}
			else if (tiles[i]->type == WEAK)
			{
				tiles[i]->type = FLOOR;		// change type to floor				
				
				// draw hidden objects
				if (tiles[i]->object == DOOR)
				{
					drawBitmap(tiles[i]->x * TILE_SIZE, 
										tiles[i]->y * TILE_SIZE, 
										bomberman_comp.width, bomberman_comp.height, door.rle_pixel_data);										
				}
				else if (tiles[i]->object == POWERUP)
				{
					drawBitmap(tiles[i]->x * TILE_SIZE, 
										tiles[i]->y * TILE_SIZE, 										
										bomb_comp.width, bomb_comp.height, power_up.rle_pixel_data);
				}
				else
				{
					drawBitmap(tiles[i]->x * TILE_SIZE, 
										tiles[i]->y * TILE_SIZE, 
										floor_comp.width, floor_comp.height, floor_comp.rle_pixel_data);
				}
			}												
		}
	}	
	
	// reset bomb
	game.bomb.tile = NULL;
	game.bomb.x = NULL;
	game.bomb.y = NULL;
}

/*--------------------------------------------------
 *      Lose life - Jack Dean
 *--------------------------------------------------*/
void loseLife(void)
{
	// check for lives left then decrement
	if (game.player.lives-- <= 0)
	{ 
		
		// stop enemy movement thread
		game.playing = false;
		osDelay(500);			
		
		// show game over screen
		GLCD_SetBackgroundColor(GLCD_COLOR_BLACK);
		GLCD_ClearScreen();
		GLCD_SetForegroundColor(GLCD_COLOR_WHITE);
		GLCD_DrawString (150, 50, "GAME OVER!");
		
		// delay before restarting game
		osDelay(3000);
		
		initGame();			
	}
	else
	{		
		// stop enemy movement thread
		game.playing = false;
		osDelay(500);
		initStage();
	}
}

/*--------------------------------------------------
 *      Draw char - Jack Dean
 *--------------------------------------------------*/
void drawChar(int x, int y, int color)	
{	
	GLCD_SetForegroundColor(color);
	GLCD_DrawChar(x + GRID_X, y + GRID_Y, 0x81);		// draws GLCD ball font
}

/*--------------------------------------------------
 *     Draw bitmap at pixels - Chris Hughes, Jack Dean
 *--------------------------------------------------*/
void drawBitmap(int x, int y, int width, int height, const unsigned char *bitmap)
{
	GLCD_RLE_Bitmap(x + GRID_X, y + GRID_Y, width, height, bitmap);
}

/*--------------------------------------------------
 *      RLE draw bitmap - Dr. Mark Fisher
 *--------------------------------------------------*/
/**
  \fn          int32_t GLCD_RLE_DrawBitmap (uint32_t x, uint32_t y, uint32_t width, uint32_t height, const uint8_t *bitmap)
  \brief       RLE bitmap ( RUN LENGTH ENCODED bitmap without header 16 bits per pixel format)
  \param[in]   x      Start x position in pixels (0 = left corner)
  \param[in]   y      Start y position in pixels (0 = upper corner)
  \param[in]   width  Bitmap width in pixels
  \param[in]   height Bitmap height in pixels
  \param[in]   bitmap Bitmap data
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
unsigned int GLCD_RLE_Bitmap (unsigned int x, unsigned int y, unsigned int width, unsigned int height, const unsigned char *bitmap) {
  
	int32_t npix = width * height;
	int32_t i=0, j;
  uint8_t *ptr_bmp, *ptr_buff;
	uint8_t count;
  
  static uint8_t buff[GLCD_WIDTH * GLCD_HEIGHT];
  ptr_buff = buff;
	 
 	while (i<npix) {
		count = *bitmap++;
		ptr_bmp = (unsigned char *) bitmap;
   

		if (count >= 128) {
			count = count-128;
			for (j = 0; j<(count); j++) { /* repeated pixels */
        ptr_buff[j*2] = *ptr_bmp;
        ptr_buff[(j*2)+1] = *(ptr_bmp+1);         
			}
			bitmap+=2; /* adjust the pointer */
		}
		else {
			for (j=0; j<(count*2); j++) {
        ptr_buff[j] = ptr_bmp[j];
      }
      bitmap+=(count*2); /* adjust the pointer */			
		}
    ptr_buff+=(count*2);
		i+=count;
	} /* while */
  
  GLCD_DrawBitmap(x, y, width, height, buff);
  return 0;
}

/*--------------------------------------------------
 *      Draw user interface - Jack Dean
 *--------------------------------------------------*/
void drawUI(void)
{
	int lives;	
	char str[] = "Lives: ";	
	char livesStr[2];	
	
	GLCD_SetForegroundColor (GLCD_COLOR_WHITE);	
	GLCD_DrawRectangle (90, 110, 30, 60);		// right
	GLCD_DrawRectangle (30, 160, 60, 30);		// down
	GLCD_DrawRectangle (0, 110, 30, 60);		// left
	GLCD_DrawRectangle (30, 90, 60, 30);		// up
	GLCD_DrawRectangle (410, 110, 40, 40);		// action
	
	// concatenate level number to level info string
	lives = game.player.lives;
	sprintf(livesStr, "%d", lives);
	strcpy(str, strcat(str, livesStr)); 
		
	// print lives info
	GLCD_SetForegroundColor(GLCD_COLOR_WHITE);
	GLCD_DrawString (0, 0, str);		;	
}

/*--------------------------------------------------
 *      Updates coords display (for testing) - Jack Dean
 *--------------------------------------------------*/
void updateDisplay(TOUCH_STATE  *tsc_state) 
{
//  char buffer[128];
//  
//  GLCD_SetForegroundColor (GLCD_COLOR_BLACK);
//  sprintf(buffer, "%i   ", tsc_state->x);	 	// raw x_coord
//  GLCD_DrawString (0, 0, buffer);
//  
//  sprintf(buffer, "%i   ", tsc_state->y);	  // raw y_coord
//  GLCD_DrawString (0, 25, buffer);
}

/*--------------------------------------------------
 *      Clear coords display (for testing) - Jack Dean
 *--------------------------------------------------*/
void clearDisplay() {
//  GLCD_SetForegroundColor (GLCD_COLOR_WHITE);
//  GLCD_DrawString (0, 0, "   ");
//  GLCD_DrawString (0, 25, "   ");
}

/*--------------------------------------------------
 *      Player movement AI (for testing) - Jack Dean
 *--------------------------------------------------*/
void playerAI(void)
{
	int randomnumber;
	
	while (true)
	{			
		// randomly choose player movement direction
		randomnumber = rand() % 4;					
		movePlayer(randomnumber);				
		osDelay(300);
	}	
}

/*--------------------------------------------------
 *      Check collision - Jack Dean
 *--------------------------------------------------*/
//bool checkCollision(void)		// not currently used
//{
//	// if tile is occupied there must be an enemy on it
//	if (game.player.tile->occupied)
//	{
//		return true;			
//	}	
//	return false;
//}
