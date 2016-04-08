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
#include "stm32f7xx_hal.h"
#include "cmsis_os.h"
#include "Board_ADC.h"
#include "GLCD_Config.h"
#include "Board_GLCD.h"
#include "Board_Touch.h"
#include <stdlib.h>
#include <stdio.h> 
#include <time.h>
#include "Bomberman_Image.c"
#include "Enemy.c"
#include "Bomb.c"

/* Globals */
TOUCH_STATE  tsc_state;
extern GLCD_FONT     GLCD_Font_16x24;
extern GLCD_FONT     GLCD_customFont_16x24;

Game game;

/*--------------------------------------------------
 *      Initialize game - Jack Dean
 *--------------------------------------------------*/
void game_init(void) 
{				
	int x;
	int y;
	int randomnumber;		
	
	// draw touc screen control areas
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
				drawAtCoords(x, y, GLCD_COLOR_BLUE);
			}
			else if ((y > 2 || x > 2))		// if tile is not adjacent to player start position
			{
				//srand((unsigned)time(NULL));	// needed to be truly random but crashes program..?
				randomnumber = rand() % 10;		
				if (randomnumber > 6)		// randomly place weak tiles
				{
					tile->type = WEAK;
					drawAtCoords(x, y, GLCD_COLOR_GREEN);
				}
				else		// place floor tiles
				{
					tile->type = FLOOR;
					tile->hasEnemy = false;
				}
			}
			else		// place floor and weak tiles at player starting position
			{
				tile->type = FLOOR;
				tile->hasEnemy = false;
			}
		}
	}
	
	// place player
	game.player.x = 1;
	game.player.y = 1;
	game.player.tile = &game.tiles[1][1];
	game.player.tile->hasPlayer = true;
	drawIntPlayer(1, 1, Bomberman_Image.pixel_data); // 7/4/16 Player bitmap Chris Hughes
	
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
						drawIntPlayer(x, y, Enemy_Image.pixel_data);
						
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
			drawAtPixels((tile->x * TILE_SIZE) + (i * xChange), 
										 (tile->y * TILE_SIZE) + (i * yChange), 
											FLOOR_COL);
			
			// then redraw bomb
			drawAtPixels(tile->x * TILE_SIZE, 
										 tile->y * TILE_SIZE, 
											BOMB_COL);										
		}
		else
		{
			// redraw floor
			drawAtPixels((tile->x * TILE_SIZE) + (i * xChange), 
										 (tile->y * TILE_SIZE) + (i * yChange), 
											FLOOR_COL);			
		}
		
		// finally, redraw player
		drawPlayer((tile->x * TILE_SIZE) + ((i+1) * xChange), 
										 (tile->y * TILE_SIZE) + ((i+1) * yChange), 
											Bomberman_Image.pixel_data); //7/4/16 Player Bitmap Chris Hughes
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
		drawAtPixels((tile->x * TILE_SIZE) + (i * xChange), 
		               (tile->y * TILE_SIZE) + (i * yChange), 
										FLOOR_COL);
		
		drawPlayer((tile->x * TILE_SIZE) + ((i+1) * xChange), 
		               (tile->y * TILE_SIZE) + ((i+1) * yChange), 
										Enemy_Image.pixel_data); //7/4/16 Enemy Bitmap Chris Hughes
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
	drawIntPlayer(game.player.x, game.player.y, GLCD_COLOR_BLACK);	
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
			drawAtCoords(tiles[i]->x, tiles[i]->y, GLCD_COLOR_RED);
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
	drawAtCoords(game.bomb.x, game.bomb.y, GLCD_COLOR_WHITE);
	for (i = 0; i < 5; i++)		
	{
		if (tiles[i]->type != SOLID)
		{
			drawAtCoords(tiles[i]->x, tiles[i]->y, GLCD_COLOR_WHITE);
		}
	}
	
	// reset bomb
	game.bomb.tile = NULL;
	game.bomb.x = NULL;
	game.bomb.y = NULL;
}

/*--------------------------------------------------
 *      Draw at coords - Jack Dean
 *--------------------------------------------------*/
void drawAtCoords(int x, int y, int color)	
{	
	GLCD_SetForegroundColor(color);
	//GLCD_DrawRectangle((x*TILE_SIZE) + GRID_X, (y*TILE_SIZE) + GRID_Y, TILE_SIZE, TILE_SIZE);		// draws hollow rectangle
	GLCD_DrawChar((x*TILE_SIZE) + GRID_X, (y*TILE_SIZE) + GRID_Y, 0x81); 													// draws GLCD font
	//GLCD_DrawBitmap((x*TILE_SIZE) + GRID_X, (y*TILE_SIZE) + GRID_Y, 20, 20, (unsigned char *) &GLCD_customFont_16x24);												// draws bitmap
}

/*--------------------------------------------------
 *      Same as drawAtCoords but bitmap - Chris Hughes
 *--------------------------------------------------*/
void drawIntPlayer(int x, int y, const unsigned char *image)
{	
  GLCD_DrawBitmap((x*TILE_SIZE) + GRID_X, (y*TILE_SIZE) + GRID_Y, 18, 18, image);
}

/*--------------------------------------------------
 *      Draw at pixels (at pixels) - Jack Dean
 *--------------------------------------------------*/
void drawAtPixels(int x, int y, int color)	
{	
	GLCD_SetForegroundColor(color);
	GLCD_DrawChar(x + GRID_X, y + GRID_Y, 0x81);		// draws GLCD font
}

/*--------------------------------------------------
 *      Same as drawAtPixels but bitmap - Chris Hughes
 *--------------------------------------------------*/
void drawPlayer(int x, int y, const unsigned char *image)
{
	GLCD_DrawBitmap(x + GRID_X, y + GRID_Y, 18, 18, image);
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
