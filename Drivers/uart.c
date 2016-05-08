/**
 * file    uart.c
 * 底层串口驱动
 *
 * Change Logs
 * Data           Author          Notes
 * 2016-4-3       bluebear233     第一版
 * 2016-4-4       bluebear233     增加中断接收
 * 2016-4-5       bluebear233     增加中断发送
 * 2016-4-7       bluebear233     增加DMA发送
 * 2016-4-8       bluebear233     增加DMA接收
 * 2016-4-10      bluebear233     修改DMA初始化的位置
 * 2016-5-8       bluebear233     修复stm32_uart_configure函数配置串口的Parity的BUG
 */

#include <rtdevice.h>
#include "stm32f1xx_hal.h"

/* 赋值给rt_device的user_data */
struct stm32_uart_user_data
{
	UART_HandleTypeDef uart;
};

/* 创建串口设备指针 */
static struct rt_serial_device uart1;
static struct stm32_uart_user_data uart1_user_data;

/**
 * @brief UART MSP Initialization
 *        This function configures the hardware resources used in this example:
 *           - Peripheral's clock enable
 *           - Peripheral's GPIO Configuration
 *           - NVIC configuration for UART interrupt request enable
 * @param huart: UART handle pointer
 * @retval None
 */
void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{

	GPIO_InitTypeDef GPIO_InitStruct;
	if (huart->Instance == USART1)
	{
		/* Peripheral clock enable */
		__USART1_CLK_ENABLE();

		/**USART1 GPIO Configuration
		 PA9     ------> USART1_TX
		 PA10     ------> USART1_RX
		 */
		GPIO_InitStruct.Pin = GPIO_PIN_9;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

		GPIO_InitStruct.Pin = GPIO_PIN_10;
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

		HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
		HAL_NVIC_EnableIRQ (USART1_IRQn);
	}
}

/**
 * @brief UART MSP De-Initialization
 *        This function frees the hardware resources used in this example:
 *          - Disable the Peripheral's clock
 *          - Revert GPIO and NVIC configuration to their default state
 * @param huart: UART handle pointer
 * @retval None
 */
void HAL_UART_MspDeInit(UART_HandleTypeDef* huart)
{

	if (huart->Instance == USART1)
	{
		/* Peripheral clock disable */
		__USART1_CLK_DISABLE();

		/**USART1 GPIO Configuration
		 PA9     ------> USART1_TX
		 PA10     ------> USART1_RX
		 */
		HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9 | GPIO_PIN_10);

		HAL_NVIC_DisableIRQ (USART1_IRQn);
	}
}

/**
 * @brief stm32串口配置
 */
rt_err_t stm32_uart_configure(struct rt_serial_device *serial,
		struct serial_configure *cfg)
{
	rt_err_t result;
	/* usart */

	/* Check the parameters */
	RT_ASSERT(serial->parent.user_data != RT_NULL);
	RT_ASSERT(cfg != RT_NULL);

	result = RT_EOK;

	/* get config parameters */
	UART_HandleTypeDef* uart =
			&((struct stm32_uart_user_data*) serial->parent.user_data)->uart;

	/* usart periph check */
	if (uart->Instance != USART1)
	{
		result = RT_ERROR;
		goto __exit;
	}

	/* config usart */
	uart->Init.BaudRate = cfg->baud_rate;
	uart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
	uart->Init.Mode = UART_MODE_TX_RX;
	uart->Init.OverSampling = UART_OVERSAMPLING_16;

	switch (cfg->data_bits)
	{
	case DATA_BITS_8:
		uart->Init.WordLength = UART_WORDLENGTH_8B;
		break;
	case DATA_BITS_9:
		uart->Init.WordLength = UART_WORDLENGTH_9B;
		break;
	default:
		result = RT_ERROR;
		goto __exit;
	}

	switch (cfg->stop_bits)
	{
	case STOP_BITS_1:
		uart->Init.StopBits = UART_STOPBITS_1;
		break;
	case STOP_BITS_2:
		uart->Init.StopBits = UART_STOPBITS_2;
		break;
	default:
		result = RT_ERROR;
		goto __exit;
	}

	switch (cfg->parity)
	{
	case PARITY_NONE:
		uart->Init.Parity = UART_PARITY_NONE;
		break;
	case PARITY_ODD:
		uart->Init.Parity = UART_PARITY_ODD;
		break;
	case PARITY_EVEN:
		uart->Init.Parity = UART_PARITY_EVEN;
		break;
	default:
		result = RT_ERROR;
		goto __exit;
	}

	if (HAL_UART_Init(uart) != HAL_OK)
	{
		result = RT_ERROR;
		goto __exit;
	}

	__HAL_UART_CLEAR_FLAG(uart, UART_FLAG_TC);
	__HAL_UART_CLEAR_FLAG(uart, UART_FLAG_RXNE);

	__exit: return result;
}

/**
 * @brief stm32串口中断配置
 */
rt_err_t stm32_uart_control(struct rt_serial_device *serial, int cmd, void *arg)
{
	uint32_t UART_IT;

	UART_HandleTypeDef* uart =
			&((struct stm32_uart_user_data*) serial->parent.user_data)->uart;
	/* Check the parameters */
	RT_ASSERT(serial != RT_NULL);

	switch ((rt_uint32_t) arg)
	{
	case RT_DEVICE_FLAG_INT_RX:
		UART_IT = UART_IT_RXNE;
		break;
	case RT_DEVICE_FLAG_INT_TX:
		UART_IT = UART_IT_TC;
		break;
	default:
		RT_ASSERT(0);
	}

	switch (cmd)
	{
	/* disable interrupt */
	case RT_DEVICE_CTRL_CLR_INT:
		__HAL_UART_DISABLE_IT(uart, UART_IT);
		break;
		/* enable interrupt */
	case RT_DEVICE_CTRL_SET_INT:
		__HAL_UART_ENABLE_IT(uart, UART_IT);
		break;
	default:
		RT_ASSERT(0);
	}
        
    return RT_EOK;
}
/**
 * @brief stm32串口发送
 */
int stm32_uart_putc(struct rt_serial_device *serial, char c)
{
	static unsigned char state = 0;
	int result = RT_EOK;

	/* Check the parameters */
	RT_ASSERT(serial->parent.user_data != RT_NULL);

	/* get config parameters */
	USART_TypeDef* uart =
			((UART_HandleTypeDef*) &((struct stm32_uart_user_data*) serial->parent.user_data)->uart)->Instance;

	if (serial->parent.open_flag & RT_DEVICE_FLAG_INT_TX)
	{
		if (!(uart->SR & UART_FLAG_TXE))
		{
			result = -1;
			goto __exit;
		}
		else
		{
			if (state == 1 && c == '\n')
			{
				state = 0;
				c = '\r';
				result = -1;
				/* 不立刻返回，先发送\r再返回-1 */
			}
			else
			{
				state = 1;
			}
		}
	}
	else
	{
		while (!(uart->SR & UART_FLAG_TXE))
			;
	}

	if (result == RT_EOK || c == '\r')
	{
		uart->SR &= ~UART_FLAG_TXE;

		uart->DR = c;
	}

	__exit: return result;
}

/**
 * @brief stm32串口接收
 */
int stm32_uart_getc(struct rt_serial_device *serial)
{
	int ch = -1;

	/* Check the parameters */
	RT_ASSERT(serial->parent.user_data != RT_NULL);

	/* get config parameters */
	USART_TypeDef* uart =
			((UART_HandleTypeDef*) &((struct stm32_uart_user_data*) serial->parent.user_data)->uart)->Instance;

	if (uart->SR & UART_FLAG_RXNE)
	{
		ch = uart->DR & 0xff;
		uart->SR &= ~UART_FLAG_RXNE;
	}

	return ch;
}
rt_size_t stm32_dma_transmit(struct rt_serial_device *serial,
		const rt_uint8_t *buf, rt_size_t size, int direction)
{
	DMA_HandleTypeDef *hdma;

	UART_HandleTypeDef *uart =
			&((struct stm32_uart_user_data*) serial->parent.user_data)->uart;

	RT_ASSERT(uart != RT_NULL);
	if (size > RT_UINT16_MAX)
		RT_ASSERT(0);

	//获取DMA_HandleTypeDef
	switch (direction)
	{
	case RT_SERIAL_DMA_TX:
		hdma = uart->hdmatx;
		break;
	case RT_SERIAL_DMA_RX:
		hdma = uart->hdmarx;
		break;
	default:
		RT_ASSERT(0);
	}

	//如果没有初始化DMA
	if (hdma == RT_NULL)
	{
		hdma = rt_malloc(sizeof(DMA_HandleTypeDef));
		RT_ASSERT(hdma != RT_NULL);
		rt_memset(hdma, 0, sizeof(DMA_HandleTypeDef));

		if (direction == RT_SERIAL_DMA_TX)
		{
			switch ((rt_uint32_t) uart->Instance)
			{
			case (rt_uint32_t) USART1:
				hdma->Instance = DMA1_Channel4;
				__HAL_RCC_DMA1_CLK_ENABLE();
				HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 0, 0);
				HAL_NVIC_EnableIRQ (DMA1_Channel4_IRQn);
				hdma->Init.Priority = DMA_PRIORITY_MEDIUM;
				break;
			default:
				RT_ASSERT(0);
			}
		}
		else
		{
			switch ((rt_uint32_t) uart->Instance)
			{
			case (rt_uint32_t) USART1:
				hdma->Instance = DMA1_Channel5;
				__HAL_RCC_DMA1_CLK_ENABLE();
				HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 0, 0);
				HAL_NVIC_EnableIRQ (DMA1_Channel5_IRQn);
				hdma->Init.Priority = DMA_PRIORITY_MEDIUM;
				break;
			default:
				RT_ASSERT(0);
			}
		}

		HAL_DMA_DeInit(hdma);
		hdma->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
		hdma->Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
		hdma->Init.PeriphInc = DMA_PINC_DISABLE;
		hdma->Init.MemInc = DMA_MINC_ENABLE;
		hdma->Init.Mode = DMA_NORMAL;

		if (direction == RT_SERIAL_DMA_TX)
		{
			hdma->Init.Direction = DMA_MEMORY_TO_PERIPH;
			__HAL_LINKDMA(uart, hdmatx, *hdma);
			SET_BIT(uart->Instance->CR3, USART_CR3_DMAT);
		}
		else
		{
			hdma->Init.Direction = DMA_PERIPH_TO_MEMORY;
			__HAL_LINKDMA(uart, hdmarx, *hdma);
			SET_BIT(uart->Instance->CR3, USART_CR3_DMAR);
		}

		HAL_DMA_Init(hdma);

		/* Enable the transfer complete interrupt */
		__HAL_DMA_ENABLE_IT(hdma, DMA_IT_TC);

	}

	__HAL_DMA_DISABLE(hdma);

	hdma->Instance->CPAR = (rt_uint32_t) & uart->Instance->DR;
	hdma->Instance->CMAR = (rt_uint32_t) buf;

	hdma->Instance->CNDTR = size;

	__HAL_DMA_ENABLE(hdma);

	return RT_EOK;
}

/**
 * @brief 串口中断函数
 */
void USART1_IRQHandler(void)
{
	/* 进入中断 */
	rt_interrupt_enter();

	/* 接收中断事件 */
	if (__HAL_UART_GET_IT_SOURCE(&(uart1_user_data.uart),
			UART_IT_RXNE))
	{
		rt_hw_serial_isr(&uart1, RT_SERIAL_EVENT_RX_IND);
		__HAL_UART_CLEAR_FLAG(&(uart1_user_data.uart),
				UART_IT_RXNE);
	}

	/* 发送中断事件  */
	if (__HAL_UART_GET_IT_SOURCE(&(uart1_user_data.uart),
			UART_IT_TC))
	{
		rt_hw_serial_isr(&uart1, RT_SERIAL_EVENT_TX_DONE);
		__HAL_UART_CLEAR_FLAG(&(uart1_user_data.uart),
				UART_IT_TC);
	}

	/* 退出中断 */
	rt_interrupt_leave();

}
/**
 * @brief 串口DMA发送中断
 */
void DMA1_Channel4_IRQHandler(void)
{
	/* 进入中断 */
	rt_interrupt_enter();

	DMA_HandleTypeDef* hdma = uart1_user_data.uart.hdmatx;

	if (__HAL_DMA_GET_IT_SOURCE(hdma, DMA_IT_TC))
	{
		rt_hw_serial_isr(&uart1, RT_SERIAL_EVENT_TX_DMADONE);
		__HAL_DMA_CLEAR_FLAG(hdma, __HAL_DMA_GET_TC_FLAG_INDEX(hdma));
	}

	/* 退出中断 */
	rt_interrupt_leave();
}

void DMA1_Channel5_IRQHandler(void)
{
	/* 进入中断 */
	rt_interrupt_enter();

	DMA_HandleTypeDef* hdma = uart1_user_data.uart.hdmatx;

	if (__HAL_DMA_GET_IT_SOURCE(hdma, DMA_IT_TC))
	{
		rt_hw_serial_isr(&uart1, RT_SERIAL_EVENT_RX_DMADONE);
		__HAL_DMA_CLEAR_FLAG(hdma, __HAL_DMA_GET_TC_FLAG_INDEX(hdma));
	}
	/* 退出中断 */
	rt_interrupt_leave();
}

/* 串口底层驱动函数结构体 */
static const struct rt_uart_ops stm32_uart_ops =
{ stm32_uart_configure, stm32_uart_control, stm32_uart_putc, stm32_uart_getc,
		stm32_dma_transmit };

/**
 * @brief 串口注册
 */
int uart_init(void)
{
	struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;

	/* 配置串口参数 */
	uart1.config = config;
	uart1.config.baud_rate = BAUD_RATE_115200;
	uart1.config.data_bits = DATA_BITS_8;
	uart1.config.parity = PARITY_NONE;
	uart1.config.stop_bits = STOP_BITS_1;
	uart1.config.bufsz = 64;

	/* 指定串口底层函数 */
	uart1.ops = &stm32_uart_ops;

	/* 赋值串口的基地址  */
	uart1_user_data.uart.Instance = USART1;

	/* 注册串口 */
	rt_hw_serial_register(&uart1, "uart1",
			RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_INT_TX
					| RT_DEVICE_FLAG_DMA_TX | RT_DEVICE_FLAG_DMA_RX, &uart1_user_data);

	return RT_EOK;
}

/* 板级初始化 */
INIT_BOARD_EXPORT (uart_init);
