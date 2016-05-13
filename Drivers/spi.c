/**
 * file    spi.c
 * spi驱动
 *
 * Change Logs
 * Data           Author          Notes
 * 2016-4-11      bluebear233     第一版
 * 2016-4-12      bludbear233     优化SPI传输函数
 * 2016-5-10      bludbear233     优化SPI内存使用
 * 2016-5-13      bludbear233     增加SPI的DMA模式
 */

#include <rtdevice.h>
#include "spi.h"
#include "stm32f1xx_hal.h"
#include "drivers/spi.h"
#include <stdio.h>

#define SPI_DMA_MODE

struct stm32_spi_user_data
{
	SPI_HandleTypeDef spi;
	rt_sem_t lock;
};

static struct rt_spi_bus spi1;
static struct stm32_spi_user_data spi1_user_data;

void HAL_SPI_MspInit(SPI_HandleTypeDef* hspi)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	if (hspi->Instance == SPI1)
	{
		/* USER CODE BEGIN SPI1_MspInit 0 */

		/* USER CODE END SPI1_MspInit 0 */
		/* Peripheral clock enable */
		__SPI1_CLK_ENABLE();

		/**SPI1 GPIO Configuration
		 PA5     ------> SPI1_SCK
		 PA6     ------> SPI1_MISO
		 PA7     ------> SPI1_MOSI
		 */
		GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_7;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

		GPIO_InitStruct.Pin = GPIO_PIN_6;
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

		/* USER CODE BEGIN SPI1_MspInit 1 */

		/* USER CODE END SPI1_MspInit 1 */
	}
	else
	{
		rt_kprintf("spi bus not find \n");
		RT_ASSERT(0);
	}
}

rt_err_t stm32_spi_poll_configure(struct rt_spi_device *device,
		struct rt_spi_configuration *configuration)
{
	struct stm32_spi_user_data *user_data = device->bus->parent.user_data;
	RT_ASSERT(user_data != RT_NULL);

	if (user_data->spi.Instance != SPI1)
	{
		rt_kprintf("spi bus not find \n");
		RT_ASSERT(0);
	}

	if(memcmp(&device->bus->owner->config,configuration,sizeof(struct rt_spi_configuration)) == 0)
		return RT_EOK;

	SPI_HandleTypeDef *hspi = &user_data->spi;

	//config mode configuration->mode = rt_spi_device->config->mode
	switch (configuration->mode & RT_SPI_MODE_3)
	{
	case RT_SPI_MODE_0:
		hspi->Init.CLKPolarity = SPI_POLARITY_LOW;
		hspi->Init.CLKPhase = SPI_PHASE_1EDGE;
		break;
	case RT_SPI_MODE_1:
		hspi->Init.CLKPolarity = SPI_POLARITY_LOW;
		hspi->Init.CLKPhase = SPI_PHASE_2EDGE;
		break;
	case RT_SPI_MODE_2:
		hspi->Init.CLKPolarity = SPI_POLARITY_HIGH;
		hspi->Init.CLKPhase = SPI_PHASE_1EDGE;
		break;
	case RT_SPI_MODE_3:
		hspi->Init.CLKPolarity = SPI_POLARITY_HIGH;
		hspi->Init.CLKPhase = SPI_PHASE_2EDGE;
		break;
	default:
		rt_kprintf(
				"rt_spi_device mode error please check rt_spi_device->config->mode \n");
		RT_ASSERT(0);
	}

	//config data width
	switch (configuration->data_width)
	{
	case 8:
		hspi->Init.DataSize = SPI_DATASIZE_8BIT;
		break;
	case 16:
		hspi->Init.DataSize = SPI_DATASIZE_16BIT;
		break;
	default:
		rt_kprintf(
				"rt_spi_device data_width error please check rt_spi_device->config->data_width \n");
		RT_ASSERT(0);
	}

	//config firstBit
	switch (configuration->mode & RT_SPI_MSB)
	{
	case RT_SPI_MSB:
		hspi->Init.FirstBit = SPI_FIRSTBIT_MSB;
		break;
	case RT_SPI_LSB:
		hspi->Init.FirstBit = SPI_FIRSTBIT_LSB;
		break;
	default:
		rt_kprintf(
				"rt_spi_device reserved error please check rt_spi_device->config->reserved \n");
		RT_ASSERT(0);
	}

	//config speed
	hspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;

	//default config
	hspi->Init.Mode = SPI_MODE_MASTER;
	hspi->Init.Direction = SPI_DIRECTION_2LINES;
	hspi->Init.NSS = SPI_NSS_SOFT;
	hspi->Init.TIMode = SPI_TIMODE_DISABLED;
	hspi->Init.CRCCalculation = SPI_CRCCALCULATION_DISABLED;
	hspi->Init.CRCPolynomial = 10;

	if (HAL_SPI_Init(hspi) != HAL_OK)
		rt_kprintf("hal init spi error");

	__HAL_SPI_ENABLE(hspi);

	return RT_EOK;
}
rt_err_t stm32_spi_dma_configure(struct rt_spi_device *device,
		struct rt_spi_configuration *configuration)
{
	struct stm32_spi_user_data *user_data = device->bus->parent.user_data;
	RT_ASSERT(user_data != RT_NULL);

	if (user_data->spi.Instance != SPI1)
	{
		rt_kprintf("spi bus not find \n");
		RT_ASSERT(0);
	}

	if(memcmp(&device->bus->owner->config,configuration,sizeof(struct rt_spi_configuration)) == 0)
		return RT_EOK;

	SPI_HandleTypeDef *hspi = &user_data->spi;

	//config mode configuration->mode = rt_spi_device->config->mode
	switch (configuration->mode & RT_SPI_MODE_3)
	{
	case RT_SPI_MODE_0:
		hspi->Init.CLKPolarity = SPI_POLARITY_LOW;
		hspi->Init.CLKPhase = SPI_PHASE_1EDGE;
		break;
	case RT_SPI_MODE_1:
		hspi->Init.CLKPolarity = SPI_POLARITY_LOW;
		hspi->Init.CLKPhase = SPI_PHASE_2EDGE;
		break;
	case RT_SPI_MODE_2:
		hspi->Init.CLKPolarity = SPI_POLARITY_HIGH;
		hspi->Init.CLKPhase = SPI_PHASE_1EDGE;
		break;
	case RT_SPI_MODE_3:
		hspi->Init.CLKPolarity = SPI_POLARITY_HIGH;
		hspi->Init.CLKPhase = SPI_PHASE_2EDGE;
		break;
	default:
		rt_kprintf(
				"rt_spi_device mode error please check rt_spi_device->config->mode \n");
		RT_ASSERT(0);
	}

	//config data width
	switch (configuration->data_width)
	{
	case 8:
		hspi->Init.DataSize = SPI_DATASIZE_8BIT;
		break;
	case 16:
		hspi->Init.DataSize = SPI_DATASIZE_16BIT;
		break;
	default:
		rt_kprintf(
				"rt_spi_device data_width error please check rt_spi_device->config->data_width \n");
		RT_ASSERT(0);
	}

	//config firstBit
	switch (configuration->mode & RT_SPI_MSB)
	{
	case RT_SPI_MSB:
		hspi->Init.FirstBit = SPI_FIRSTBIT_MSB;
		break;
	case RT_SPI_LSB:
		hspi->Init.FirstBit = SPI_FIRSTBIT_LSB;
		break;
	default:
		rt_kprintf(
				"rt_spi_device reserved error please check rt_spi_device->config->reserved \n");
		RT_ASSERT(0);
	}

	//config speed
	hspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;

	//default config
	hspi->Init.Mode = SPI_MODE_MASTER;
	hspi->Init.Direction = SPI_DIRECTION_2LINES;
	hspi->Init.NSS = SPI_NSS_SOFT;
	hspi->Init.TIMode = SPI_TIMODE_DISABLED;
	hspi->Init.CRCCalculation = SPI_CRCCALCULATION_DISABLED;
	hspi->Init.CRCPolynomial = 10;

	if (HAL_SPI_Init(hspi) != HAL_OK){
		rt_kprintf("hal init spi error");
		RT_ASSERT(0);
	}

	__HAL_SPI_ENABLE(hspi);

	if(hspi->hdmatx == RT_NULL){
		DMA_HandleTypeDef *hdmatx = rt_malloc(sizeof(DMA_HandleTypeDef));
		DMA_HandleTypeDef *hdmarx = rt_malloc(sizeof(DMA_HandleTypeDef));

		RT_ASSERT(hdmatx != RT_NULL);
		RT_ASSERT(hdmarx != RT_NULL);

		rt_memset(hdmatx,sizeof(DMA_HandleTypeDef),0);
		rt_memset(hdmarx,sizeof(DMA_HandleTypeDef),0);

		switch((uint32_t)hspi->Instance){
		case (uint32_t)SPI1:
			hdmatx->Instance = DMA1_Channel3;
			hdmarx->Instance = DMA1_Channel2;

			if(__HAL_RCC_DMA1_IS_CLK_DISABLED())
				__HAL_RCC_DMA1_CLK_ENABLE();

			HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 0, 0);
			HAL_NVIC_EnableIRQ (DMA1_Channel3_IRQn);

			hdmatx->Init.Priority = DMA_PRIORITY_MEDIUM;
			hdmarx->Init.Priority = DMA_PRIORITY_MEDIUM;

			user_data->lock = rt_sem_create("sp1_lock",0,RT_IPC_FLAG_FIFO);
			RT_ASSERT(user_data->lock != RT_NULL);

			break;
		default:
			RT_ASSERT(0);
		}

		HAL_DMA_DeInit(hdmatx);
		hdmatx->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
		hdmatx->Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
		hdmatx->Init.PeriphInc = DMA_PINC_DISABLE;
		hdmatx->Init.MemInc = DMA_MINC_ENABLE;
		hdmatx->Init.Mode = DMA_NORMAL;
		hdmatx->Init.Direction = DMA_MEMORY_TO_PERIPH;
		__HAL_LINKDMA(&(user_data->spi), hdmatx, *hdmatx);
		__HAL_DMA_ENABLE_IT(hdmatx, DMA_IT_TC);

		HAL_DMA_DeInit(hdmarx);
		hdmarx->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
		hdmarx->Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
		hdmarx->Init.PeriphInc = DMA_PINC_DISABLE;
		hdmarx->Init.MemInc = DMA_MINC_ENABLE;
		hdmarx->Init.Mode = DMA_NORMAL;
		hdmarx->Init.Direction = DMA_PERIPH_TO_MEMORY;
		__HAL_LINKDMA(&(user_data->spi), hdmarx, *hdmarx);

		HAL_DMA_Init(hdmatx);
		HAL_DMA_Init(hdmarx);

		hdmatx->Instance->CPAR = (rt_uint32_t) & user_data->spi.Instance->DR;
		hdmarx->Instance->CPAR = (rt_uint32_t) & user_data->spi.Instance->DR;
	}

	SET_BIT(user_data->spi.Instance->CR2, SPI_CR2_TXDMAEN);
	SET_BIT(user_data->spi.Instance->CR2, SPI_CR2_RXDMAEN);

	return RT_EOK;
}
rt_uint32_t stm32_spi_dma_xfer(struct rt_spi_device *device,
		struct rt_spi_message *message)
{
	void (*cs)(
			GPIO_PinState PinState) = ((struct spi_device_user_data *)device->parent.user_data)->cs;
	rt_err_t result;
	RT_ASSERT(cs != RT_NULL);


	SPI_TypeDef *spi =
			((struct stm32_spi_user_data*) device->bus->parent.user_data)->spi.Instance;

	rt_size_t length = message->length;
	if (length == 0)
		return 0;

	if (message->cs_take)
	{
		cs(GPIO_PIN_RESET);
	}

	if (message->send_buf != RT_NULL)
	{

		uint8_t *buf = (uint8_t *) message->send_buf;

		DMA_HandleTypeDef *hdma = ((struct stm32_spi_user_data*)device->bus->parent.user_data)->spi.hdmatx;

		__HAL_DMA_DISABLE(hdma);

		hdma->Instance->CMAR = (rt_uint32_t) buf;

		hdma->Instance->CNDTR = length;

		__HAL_DMA_ENABLE(hdma);
	}
	else if (message->recv_buf != RT_NULL)
	{

		uint8_t *buf = message->recv_buf;

		*buf = spi->DR;

		DMA_HandleTypeDef *hdmatx = ((struct stm32_spi_user_data*)device->bus->parent.user_data)->spi.hdmatx;

		DMA_HandleTypeDef *hdmarx = ((struct stm32_spi_user_data*)device->bus->parent.user_data)->spi.hdmarx;

		__HAL_DMA_DISABLE(hdmarx);

		hdmarx->Instance->CMAR = (rt_uint32_t) buf;

		hdmarx->Instance->CNDTR = length;

		__HAL_DMA_ENABLE(hdmarx);

		__HAL_DMA_DISABLE(hdmatx);

		hdmatx->Instance->CNDTR = length;

		__HAL_DMA_ENABLE(hdmatx);

	}

	result = rt_sem_take(spi1_user_data.lock,RT_WAITING_FOREVER);
	RT_ASSERT(result == RT_EOK);

	if (message->cs_release)
	{
		cs(GPIO_PIN_SET);
	}

	return 1;
}

rt_uint32_t stm32_spi_poll_xfer(struct rt_spi_device *device,
		struct rt_spi_message *message)
{
	void (*cs)(
			GPIO_PinState PinState) = ((struct spi_device_user_data *)device->parent.user_data)->cs;
	RT_ASSERT(cs != RT_NULL);

	rt_size_t length = message->length;
	if (length == 0)
		return 0;

	SPI_TypeDef *spi =
			((struct stm32_spi_user_data*) device->bus->parent.user_data)->spi.Instance;

	if (message->cs_take)
	{
		cs(GPIO_PIN_RESET);
	}

	if (message->send_buf != RT_NULL)
	{

		uint8_t *buf = (uint8_t *) message->send_buf;

		if (length == 1)
		{
			while ((spi->SR & SPI_FLAG_TXE) != SPI_FLAG_TXE)
				;
			spi->DR = *buf;
		}
		else
		{

			while (length > 0)
			{

				while ((spi->SR & SPI_FLAG_TXE) != SPI_FLAG_TXE)
					;
				spi->DR = *buf++;

				length--;

			}
		}
	}
	else if (message->recv_buf != RT_NULL)
	{

		uint8_t *buf = message->recv_buf;

		*buf = spi->DR;

		spi->DR = 0XFF;

		while (length > 0)
		{
			while ((spi->SR & SPI_FLAG_RXNE) != SPI_FLAG_RXNE)
				;
			*buf++ = spi->DR;
			spi->DR = 0XFF;
			length--;
		}
	}

	if (message->cs_release)
	{
		cs(GPIO_PIN_SET);
	}

	return 1;
}

void DMA1_Channel3_IRQHandler(void){
	/* 进入中断 */
	rt_interrupt_enter();

	DMA_HandleTypeDef* hdma = spi1_user_data.spi.hdmatx;

	if (__HAL_DMA_GET_IT_SOURCE(hdma, DMA_IT_TC))
	{
		rt_sem_release(spi1_user_data.lock);
		__HAL_DMA_CLEAR_FLAG(hdma, __HAL_DMA_GET_TC_FLAG_INDEX(hdma));
	}

	/* 退出中断 */
	rt_interrupt_leave();
}

const struct rt_spi_ops stm32_spi_poll_ops =
{ stm32_spi_poll_configure, stm32_spi_poll_xfer };

const struct rt_spi_ops stm32_spi_dma_ops =
{ stm32_spi_dma_configure, stm32_spi_dma_xfer };

int spi_init(void)
{
	spi1_user_data.spi.Instance = SPI1;

	spi1.parent.user_data = &spi1_user_data;

#ifdef SPI_DMA_MODE
	rt_spi_bus_register(&spi1, "spi1", &stm32_spi_dma_ops);
#else
	rt_spi_bus_register(&spi1, "spi1", &stm32_spi_poll_ops);
#endif

	return RT_EOK;
}
INIT_BOARD_EXPORT (spi_init);
