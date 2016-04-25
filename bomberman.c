/**
  ******************************************************************************
  * @file    bomberman.c 
  * @author  Jack Dean		
  * @version V1.0.1
  * @date    26-03-2016
  * @brief   Simple version of Bomberman game.
  ******************************************************************************
  * @attention
  *
  * Note: The functions:
  *   SystemClock_Config ( )
  *   CPU_CACHE_Enable ( )
  * are copied from the uVision5 examples provided with DFP supporting the board.
  *
  ******************************************************************************
  */

#include "bomberman.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "cmsis_os.h"
#include "stm32f7xx_hal.h"
#include "stm32746g_discovery_sdram.h"
#include "RTE_Components.h"
#include "GLCD_Config.h"
#include "Board_GLCD.h"
#include "Board_Touch.h"

/* Globals */
extern GLCD_FONT     GLCD_Font_16x24;
extern Game game;


#ifdef RTE_CMSIS_RTOS_RTX
extern uint32_t os_time;

uint32_t HAL_GetTick(void) { 
  return os_time; 
}
#endif

/**
  * System Clock Configuration
  *   System Clock source            = PLL (HSE)
  *   SYSCLK(Hz)                     = 200000000
  *   HCLK(Hz)                       = 200000000
  *   AHB Prescaler                  = 1
  *   APB1 Prescaler                 = 4
  *   APB2 Prescaler                 = 2
  *   HSE Frequency(Hz)              = 25000000
  *   PLL_M                          = 25
  *   PLL_N                          = 400
  *   PLL_P                          = 2
  *   PLL_Q                          = 8
  *   VDD(V)                         = 3.3
  *   Main regulator output voltage  = Scale1 mode
  *   Flash Latency(WS)              = 6
  */
static void SystemClock_Config (void) {
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 400;  
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 8;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  /* Activate the OverDrive to reach the 200 MHz Frequency */
  HAL_PWREx_EnableOverDrive();
  
  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;  
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_6);
}

/**
  * CPU L1-Cache enable
  */
static void CPU_CACHE_Enable (void) {

  /* Enable I-Cache */
  SCB_EnableICache();

  /* Enable D-Cache */
  SCB_EnableDCache();
}

/*********************************************************************
*
*       RTX code
*/
// declare mailbox
osMailQDef(mail_box, 16, mail_t);
osMailQId mail_box;

// declare task ids
osThreadId tid_taskA;   
osThreadId tid_taskB;
osThreadId tid_taskC;
osThreadId tid_taskD;
osThreadId tid_taskE;
osThreadId tid_taskF;
osThreadId tid_taskG;

osMutexId mut_GLCD; // decalre mutex to control GLCD access 

// define thread prototypes
void taskA (void const *argument);
void taskB (void const *argument);
void taskC (void const *argument);
void taskD (void const *argument);
void taskE (void const *argument);
void taskF (void const *argument);
void taskG (void const *argument);

// define threads
osThreadDef(taskA, osPriorityNormal, 1, 0);
osThreadDef(taskB, osPriorityNormal, 1, 0);
osThreadDef(taskC, osPriorityNormal, 1, 0);
osThreadDef(taskD, osPriorityNormal, 1, 0);
osThreadDef(taskE, osPriorityNormal, 1, 0);
osThreadDef(taskF, osPriorityNormal, 1, 0);
osThreadDef(taskG, osPriorityNormal, 1, 0);

//define GLCD mutex
osMutexDef(mut_GLCD);


/*********************************************************************
*       Signal Functions (must be in main file) 
*/
/*--------------------------------------------------
 *      Signal bomb thread - Jack Dean
 *--------------------------------------------------*/
void startBomb()
{			
	osSignalSet(tid_taskD, 1U);
}

void startEnemies()
{			
	osSignalSet(tid_taskC, 1U);
}

void startControls()
{			
	osSignalSet(tid_taskB, 1U);
}
	

/*********************************************************************
*       Task functions
*
*/
/*--------------------------------------------------
 *      Thread 1 'taskA': Initialisation task - Jack Dean
 *--------------------------------------------------*/
void taskA (void const *argument) {
  for (;;) {
		osSignalWait(0x0001, osWaitForever);		
			
		initGame();										// start game	
		osSignalSet(tid_taskB, 1U);		// start controls thread	
  }
}

/*--------------------------------------------------
 *      Thread 2 'taskB': Contols - Jack Dean
 *--------------------------------------------------*/
void taskB (void const *argument) {
  for (;;) {
		osSignalWait(0x0001, osWaitForever);
		controls();
  }
}

/*--------------------------------------------------
 *      Thread 3 'taskC': Enemy movement - Jack Dean
 *--------------------------------------------------*/

void taskC (void const *argument) {
  for (;;) {
		osSignalWait(0x0001, osWaitForever);		
		moveEnemies();
  }
}

/*--------------------------------------------------
 *      Thread 4 'taskD': Bomb - Jack Dean
 *--------------------------------------------------*/
void taskD (void const *argument) {
  for (;;) {
		osSignalWait(0x0001, osWaitForever);
		dropBomb();		
		osDelay(2000);													// bomb timer		
		bombExplode();		
		osSignalWait(0x0001, osWaitForever);		// needed otherwise thread executes twice for some reason
  }
}

/*--------------------------------------------------
 *      Thread 5 'taskE': 
 *--------------------------------------------------*/
void taskE (void const *argument) {
  for (;;) {
		
  }
}
	
/*--------------------------------------------------
 *      Thread 6 'taskF': 
 *--------------------------------------------------*/
void taskF (void const *argument) {
	for (;;) {
		
	}
}

/*--------------------------------------------------
 *      Thread 7 'taskG': 
 *--------------------------------------------------*/
void taskG (void const *argument) {
	for (;;) {
		
	}
}


/*********************************************************************
*
*       Main
*/
/*--------------------------------------------------
 *      Initialization code
 *--------------------------------------------------*/
int main (void) {
	CPU_CACHE_Enable();                       /* Enable the CPU Cache           */
  HAL_Init();                               /* Initialize the HAL Library     */
  BSP_SDRAM_Init();                         /* Initialize BSP SDRAM           */
  SystemClock_Config();                     /* Configure the System Clock     */
	//MPU_Config();                             /* Configure the MPU              */
  osKernelInitialize();                     /* initialize CMSIS-RTOS          */	
	Touch_Initialize();                            /* Touchscrn Controller Init */ 
	GLCD_Initialize();
  GLCD_SetFont(&GLCD_Font_16x24);	

	/* Initialize RTX variables */
	mut_GLCD = osMutexCreate(osMutex(mut_GLCD));	
	mail_box = osMailCreate(osMailQ(mail_box), NULL);
	tid_taskA = osThreadCreate(osThread(taskA), NULL);
	tid_taskB = osThreadCreate(osThread(taskB), NULL);
	tid_taskC = osThreadCreate(osThread(taskC), NULL);
	tid_taskD = osThreadCreate(osThread(taskD), NULL);
	tid_taskE = osThreadCreate(osThread(taskE), NULL);
	tid_taskF = osThreadCreate(osThread(taskC), NULL);
	tid_taskG = osThreadCreate(osThread(taskD), NULL);
	
	
/*--------------------------------------------------
 *      Run game
 *--------------------------------------------------*/
	osKernelStart();	// when used clock speed in conf is used so is faster?
	
	osSignalSet(tid_taskA, 1U);    // signal taskA

	osDelay(osWaitForever);
	while(1);
}

/*************************** End of file ****************************/
