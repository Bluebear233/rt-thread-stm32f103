/**
 * file    board.c
 * 板级驱动
 *
 * Change Logs
 * Data           Author          Notes
 * 2016-5-7       bluebear233     第一版
 */
#include "rtthread.h"
#include "stm32f1xx_hal.h"

/**
 * System Clock Configuration
 */
void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct;
	RCC_ClkInitTypeDef RCC_ClkInitStruct;

	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
	HAL_RCC_OscConfig(&RCC_OscInitStruct);

	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
	HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);

	HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq() / RT_TICK_PER_SECOND);

	HAL_SYSTICK_CLKSourceConfig (SYSTICK_CLKSOURCE_HCLK);

	/* SysTick_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}
void SysTick_Handler(void)
{
	/* enter interrupt */
	rt_interrupt_enter();

	rt_tick_increase();

	/* leave interrupt */
	rt_interrupt_leave();

}
/**
 * This function will initial STM32 board.
 */
void rt_hw_board_init()
{
	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	HAL_NVIC_SetPriorityGrouping (NVIC_PRIORITYGROUP_4);

	/* Configure the system clock @ 72 Mhz
	 * Configure the systemtick have interrupt in 10ms time basis
	 */
	SystemClock_Config();

#ifdef RT_USING_HEAP
	extern int __bss_end;
	extern int _estack;
	rt_system_heap_init((void*)&__bss_end, (void*)&_estack);
#endif
}
