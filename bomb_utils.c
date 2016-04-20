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
 *      Initialize game - Jack Dean
 *---------------------------------------- ----------*/
void game_init(void) 
{				
	int x;
	int y;
	int randomnumber;		
	
	// draw touch screen control areas
	drawUI();
	
	// iterate through tiles
	for (y = 0; y < ROWS; y++)
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
				//drawChar(x * TILE_SIZE, y * TILE_SIZE, GLCD_COLOR_BLUE);
				drawBitmap(x * TILE_SIZE, y * TILE_SIZE, solid_comp.width, solid_comp.height, solid_comp.rle_pixel_data);
			}
			else if ((y > 2 || x > 2))		// if tile is not adjacent to player start position
			{
				//srand((unsigned)time(NULL));	// needed to be truly random but crashes program..?
				randomnumber = rand() % 10;		
				if (randomnumber > 6)		// randomly place weak tiles
				{
					tile->type = WEAK;
					//drawChar(x * TILE_SIZE, y * TILE_SIZE, GLCD_COLOR_GREEN);
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
		}
	}
	
	// place player
	game.player.x = 1;
	game.player.y = 1;
	game.player.tile = &game.tiles[1][1];
	game.player.tile->hasPlayer = true;
	drawBitmap(1 * TILE_SIZE, 1 * TILE_SIZE, bomberman_comp.width, bomberman_comp.height, bomberman_comp.rle_pixel_data);
	
	// initialise bomb level
	game.bomb.level = 1;
	
	placeEnemies();	
}

/*--------------------------------------------------
 *      Place enemies - Jack Dean
 *--------------------------------------------------*/
void placeEnemies(void)
{
	int i;
	int y;
	int x;
	int randomnumber;
	bool placed;
	
	for (i = 0; i < 6; i++)		// for each enemy
	{
		placed = false;
		for (y = 5; y < ROWS; y++)		// iterate through tiles
		{
			for (x = 0; x < COLS; x++)
			{
				Tile* tile = &game.tiles[y][x];			
				
				if (tile->type == FLOOR && tile->hasEnemy == false)
				{
					randomnumber = rand() % 255;
					if (randomnumber < 25)
					{
						// set enemy fields
						game.enemies[i].alive = true;
						game.enemies[i].tile = tile;						
						tile->enemy = &game.enemies[i];
						tile->hasEnemy = true;
						drawBitmap(x * TILE_SIZE, y * TILE_SIZE, enemy_comp.width, enemy_comp.height, enemy_comp.rle_pixel_data);						
						placed = true;
						break;
					}			
				}		
			}
			if (placed == true)
			{
				break;
			}
		}
		if (placed != true)
		{
			game.enemies[i].alive = false;
			game.enemies[i].tile = NULL;
		}
	}	
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
	
	// check player collision with enemies
	if (game.player.tile->hasEnemy == true)
	{
		// restart game
		GLCD_ClearScreen(); 
		game_init();
	}
	//osDelay(250);	
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
//			drawChar((tile->x * TILE_SIZE) + (i * xChange), 
//										 (tile->y * TILE_SIZE) + (i * yChange), 
//											FLOOR_COL);
			drawBitmap((tile->x * TILE_SIZE) + (i * xChange), 
										(tile->y * TILE_SIZE) + (i * yChange), 
										floor_comp.width, floor_comp.height, floor_comp.rle_pixel_data);
			
			// then redraw bomb
//			drawChar(tile->x * TILE_SIZE, 
//										 tile->y * TILE_SIZE, 
//											BOMB_COL);		
			drawBitmap(tile->x * TILE_SIZE, tile->y * TILE_SIZE, bomb_comp.width, bomb_comp.height, bomb_comp.rle_pixel_data);

		}
		else
		{
			// redraw floor
//			drawChar((tile->x * TILE_SIZE) + (i * xChange), 
//										 (tile->y * TILE_SIZE) + (i * yChange), 
//											FLOOR_COL);	
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
			if (game.enemies[i].alive)
			{			
				Tile* tile = game.enemies[i].tile;		// temp store enemy tile for easier field access
				Enemy* enemy = &game.enemies[i];
				
				// randomly choose movement direction
				randomnumber = rand() % 4;
				
				switch (randomnumber)
				{
					case 0:
						if (game.tiles[tile->y][tile->x+1].type == FLOOR)		// check tile moving to is FLOOR tile
						{
							updateEnemy(tile, enemy, 1, 0);																		
						}												
						break;
					case 1:
						if (game.tiles[tile->y+1][tile->x].type == FLOOR)
						{
							updateEnemy(tile, enemy, 0, 1);
						}
						break;
					case 2:
						if (game.tiles[tile->y][tile->x-1].type == FLOOR)
						{
							updateEnemy(tile, enemy, -1, 0);								
						}
						break;
					case 3:
						if (game.tiles[tile->y-1][tile->x].type == FLOOR)
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
					// restart game
					GLCD_ClearScreen(); 
					game_init();
				}				
			}		
		}
		//osDelay(750);
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
//		drawChar((tile->x * TILE_SIZE) + (i * xChange), 
//		               (tile->y * TILE_SIZE) + (i * yChange), 
//										FLOOR_COL);
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

				// signals bomb thread - startBomb in bomberman.c
				startBomb();				
			}
			else
			{
				clearDisplay();				
				//updateDisplay(&tsc_state);
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
	//drawChar(game.player.x * TILE_SIZE, game.player.y * TILE_SIZE, GLCD_COLOR_BLACK);	
	drawBitmap(game.player.x * TILE_SIZE, game.player.y * TILE_SIZE, bomb_comp.width, bomb_comp.height, bomb_comp.rle_pixel_data);	
}

/*--------------------------------------------------
 *      Bomb Explode - Jack Dean
 *--------------------------------------------------*/
void bombExplode(void)
{
	int i;
	int x = game.bomb.x;
	int y = game.bomb.y;	
	
	// temp store bomb tile and adjacent tiles
	Tile* tiles[5];
	tiles[0] = game.bomb.tile;
	tiles[1] = &game.tiles[y][x+1];		
	tiles[2] = &game.tiles[y+1][x];
	tiles[3] = &game.tiles[y][x]-1;
	tiles[4] = &game.tiles[y-1][x];	
	
	game.bomb.tile->hasBomb = false;
	
	for (i = 0; i < 5; i++)		// process explosions
	{
		if (tiles[i]->type != SOLID)
		{
			//drawChar(tiles[i]->x * TILE_SIZE, tiles[i]->y * TILE_SIZE, GLCD_COLOR_RED);
			drawBitmap(tiles[i]->x * TILE_SIZE, 
										tiles[i]->y * TILE_SIZE, 
										explo_comp.width, explo_comp.height, explo_comp.rle_pixel_data);

			tiles[i]->type = FLOOR;		// change weak tile type to floor
						
			if (tiles[i]->hasPlayer)		// check player collision
			{					
				// restart game
				GLCD_ClearScreen(); 
				game_init();
			}
			else if (tiles[i]->hasEnemy)		// check enemy collision
			{
				tiles[i]->enemy->alive = false;			  // remove enemy from game
				tiles[i]->enemy->tile->hasEnemy = false;
				tiles[i]->enemy->tile->enemy = NULL;
			}
		}
	}		
 
	osDelay(800);
	
	// clear explosions
	for (i = 0; i < 5; i++)		
	{
		if (tiles[i]->type != SOLID)
		{
			//drawChar(tiles[i]->x * TILE_SIZE, tiles[i]->y * TILE_SIZE, GLCD_COLOR_WHITE);
			drawBitmap(tiles[i]->x * TILE_SIZE, 
										tiles[i]->y * TILE_SIZE, 
										floor_comp.width, floor_comp.height, floor_comp.rle_pixel_data);
		}
	}
	
	// reset bomb
	game.bomb.tile = NULL;
	game.bomb.x = NULL;
	game.bomb.y = NULL;
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
	GLCD_SetForegroundColor (GLCD_COLOR_BLACK);	
	GLCD_DrawRectangle (90, 110, 30, 60);		// right
	GLCD_DrawRectangle (30, 160, 60, 30);		// down
	GLCD_DrawRectangle (0, 110, 30, 60);		// left
	GLCD_DrawRectangle (30, 90, 60, 30);		// up
	GLCD_DrawRectangle (390, 110, 40, 40);		// action
}

/*--------------------------------------------------
 *      Updates coords display (for testing) - Jack Dean
 *--------------------------------------------------*/
void updateDisplay(TOUCH_STATE  *tsc_state) 
{
  char buffer[128];
  
  GLCD_SetForegroundColor (GLCD_COLOR_BLACK);
  sprintf(buffer, "%i   ", tsc_state->x);	 	// raw x_coord
  GLCD_DrawString (0, 0, buffer);
  
  sprintf(buffer, "%i   ", tsc_state->y);	  // raw y_coord
  GLCD_DrawString (0, 25, buffer);
}

/*--------------------------------------------------
 *      Clear coords display (for testing) - Jack Dean
 *--------------------------------------------------*/
void clearDisplay() {
  GLCD_SetForegroundColor (GLCD_COLOR_WHITE);
  GLCD_DrawString (0, 0, "   ");
  GLCD_DrawString (0, 25, "   ");
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
