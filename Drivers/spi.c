/**
 * file    spi.c
 * spi驱动
 *
 * Change Logs
 * Data           Author          Notes
 * 2016-4-11      bluebear233     第一版
 * 2016-4-12      bluebear233     优化SPI传输函数
 * 2016-5-10      bluebear233     优化SPI内存使用
 * 2016-5-13      bluebear233     增加SPI的DMA模式
 * 2016-5-19      bluebear233     修复SPI的DMA模式BUG
 */

#include <rtdevice.h>
#include "spi.h"
#include "stm32f1xx_hal.h"
#include "drivers/spi.h"
#include <stdio.h>
#include "board.h"

struct stm32_spi_user_data
{
	SPI_HandleTypeDef spi;
	uint8_t error_flag;
	rt_sem_t lock;
};

static struct rt_spi_bus spi1;
static struct stm32_spi_user_data spi1_user_data;

static void spi_transmission_with_dma(struct rt_spi_device *device, const void * send_addr, void * recv_addr, rt_size_t size)
{
	static uint8_t dummy = 0xFF;

	SPI_HandleTypeDef *hspi = &((struct stm32_spi_user_data*) device->bus->parent.user_data)->spi;

	__HAL_DMA_CLEAR_FLAG(hspi->hdmarx, __HAL_DMA_GET_TC_FLAG_INDEX(hspi->hdmarx));
	__HAL_DMA_CLEAR_FLAG(hspi->hdmatx, __HAL_DMA_GET_TC_FLAG_INDEX(hspi->hdmatx));

	hspi->Instance->DR;

    if(recv_addr != RT_NULL)
    {
        /* RX channel configuration */
        __HAL_DMA_DISABLE(hspi->hdmarx);

        hspi->hdmarx->Instance->CNDTR = size;

    	hspi->hdmarx->Instance->CMAR = (uint32_t) recv_addr;
        hspi->hdmarx->Init.MemInc = DMA_MINC_ENABLE;

        HAL_DMA_Init(hspi->hdmarx);
        __HAL_DMA_ENABLE(hspi->hdmarx);

		__HAL_DMA_ENABLE_IT(hspi->hdmarx,DMA_IT_TC);
		__HAL_DMA_DISABLE_IT(hspi->hdmatx,DMA_IT_TC);
    }
    else
    {
		__HAL_DMA_DISABLE_IT(hspi->hdmarx,DMA_IT_TC);
		__HAL_DMA_ENABLE_IT(hspi->hdmatx,DMA_IT_TC);
    }

    /* TX channel configuration */
    __HAL_DMA_DISABLE(hspi->hdmatx);

    hspi->hdmatx->Instance->CNDTR = size;

    if(send_addr != RT_NULL)
    {
    	hspi->hdmatx->Instance->CMAR = (uint32_t) send_addr;
        hspi->hdmatx->Init.MemInc = DMA_MINC_ENABLE;
    }
    else
    {
    	hspi->hdmatx->Instance->CMAR = (uint32_t) (&dummy);
        hspi->hdmatx->Init.MemInc = DMA_MINC_DISABLE;
    }

    HAL_DMA_Init(hspi->hdmatx);
    __HAL_DMA_ENABLE(hspi->hdmatx);

}
static void spi_transmission_with_poll(struct rt_spi_device* device, const uint8_t* send_addr, uint8_t* recv_addr,int length)
{
	SPI_TypeDef *spi =
			((struct stm32_spi_user_data*) device->bus->parent.user_data)->spi.Instance;

	spi->DR;

	if(send_addr != RT_NULL && recv_addr == RT_NULL){
		while(length --){
			while ((spi->SR & SPI_FLAG_TXE) != SPI_FLAG_TXE);

			spi->DR = *send_addr++;
		}
	}
	else if(send_addr != RT_NULL && recv_addr != RT_NULL){
		while(length --){

			spi->DR = *send_addr++;

			while ((spi->SR & SPI_FLAG_RXNE) != SPI_FLAG_RXNE);

			*recv_addr++ = spi->DR;
		}
	}
	else{
		while(length --){

			spi->DR = 0xff;

			while ((spi->SR & SPI_FLAG_RXNE) != SPI_FLAG_RXNE);

			*recv_addr++ = spi->DR;
		}
	}
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef* hspi){

}

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
	if(configuration->max_hz >= 18*1000*1000)
		hspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
	else if(configuration->max_hz >= 9*1000*1000)
		hspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
	else if(configuration->max_hz >= 4.5*1000*1000)
		hspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
	else if(configuration->max_hz >= 2.25*1000*1000)
		hspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
	else if(configuration->max_hz >= 1.125*1000*1000)
		hspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
	else if(configuration->max_hz >= 562.5*1000)
		hspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
	else
		hspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;

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

	HAL_SPI_DeInit(hspi);

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
	if(configuration->max_hz >= 18*1000*1000)
		hspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
	else if(configuration->max_hz >= 9*1000*1000)
		hspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
	else if(configuration->max_hz >= 4.5*1000*1000)
		hspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
	else if(configuration->max_hz >= 2.25*1000*1000)
		hspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
	else if(configuration->max_hz >= 1.125*1000*1000)
		hspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
	else if(configuration->max_hz >= 562.5*1000)
		hspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
	else
		hspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;

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
			HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 0, 0);
			HAL_NVIC_EnableIRQ (DMA1_Channel2_IRQn);

			hdmatx->Init.Priority = DMA_PRIORITY_MEDIUM;
			hdmarx->Init.Priority = DMA_PRIORITY_VERY_HIGH;

			user_data->lock = rt_sem_create("sp1_lock",0,RT_IPC_FLAG_FIFO);
			RT_ASSERT(user_data->lock != RT_NULL);

			break;
		default:
			RT_ASSERT(0);
		}

		hdmatx->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
		hdmatx->Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
		hdmatx->Init.PeriphInc = DMA_PINC_DISABLE;
		hdmatx->Init.MemInc = DMA_MINC_ENABLE;
		hdmatx->Init.Mode = DMA_NORMAL;
		hdmatx->Init.Direction = DMA_MEMORY_TO_PERIPH;
		HAL_DMA_Init(hdmatx);
		hdmatx->Instance->CPAR = (uint32_t)&hspi->Instance->DR;
		__HAL_LINKDMA(&(user_data->spi), hdmatx, *hdmatx);

		hdmarx->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
		hdmarx->Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
		hdmarx->Init.PeriphInc = DMA_PINC_DISABLE;
		hdmarx->Init.MemInc = DMA_MINC_ENABLE;
		hdmarx->Init.Mode = DMA_NORMAL;
		hdmarx->Init.Direction = DMA_PERIPH_TO_MEMORY;
		HAL_DMA_Init(hdmarx);
		hdmarx->Instance->CPAR = (uint32_t)&hspi->Instance->DR;
		__HAL_LINKDMA(&(user_data->spi), hdmarx, *hdmarx);

		__HAL_DMA_ENABLE_IT(hspi->hdmarx,DMA_IT_TC|DMA_IT_TE);
		__HAL_DMA_ENABLE_IT(hspi->hdmatx,DMA_IT_TC|DMA_IT_TE);
	}

	SET_BIT(hspi->Instance->CR2, SPI_CR2_RXDMAEN|SPI_CR2_TXDMAEN);

	return RT_EOK;
}
rt_uint32_t stm32_spi_dma_xfer(struct rt_spi_device *device,
		struct rt_spi_message *message)
{
	void (*cs)(
			GPIO_PinState PinState) = ((struct spi_device_user_data *)device->parent.user_data)->cs;
	RT_ASSERT(cs != RT_NULL);

	rt_err_t result;

	rt_size_t length = message->length;

	if (message->cs_take)
	{
		cs(GPIO_PIN_RESET);
	}

	if(length > 32){

		spi_transmission_with_dma(device,message->send_buf, message->recv_buf, message->length);

		result = rt_sem_take(spi1_user_data.lock,RT_WAITING_FOREVER);
		RT_ASSERT(result == RT_EOK);

		if(((struct stm32_spi_user_data*) device->bus->parent.user_data)->error_flag){
			rt_kprintf("spi dma error \n");
			RT_ASSERT(0);
		}

	}else if(length > 0){

		spi_transmission_with_poll(device,message->send_buf, message->recv_buf, message->length);

	}

	if (message->cs_release)
	{
		cs(GPIO_PIN_SET);
	}

	return message->length;
}

rt_uint32_t stm32_spi_poll_xfer(struct rt_spi_device *device,
		struct rt_spi_message *message)
{
	void (*cs)(
			GPIO_PinState PinState) = ((struct spi_device_user_data *)device->parent.user_data)->cs;
	RT_ASSERT(cs != RT_NULL);

	if (message->cs_take)
	{
		cs(GPIO_PIN_RESET);
	}

	if(message->length > 0)
		spi_transmission_with_poll(device,message->send_buf, message->recv_buf, message->length);

	if (message->cs_release)
	{
		cs(GPIO_PIN_SET);
	}

	return 1;
}
void DMA1_Channel2_IRQHandler(void){
	/* 进入中断 */
	rt_interrupt_enter();

	DMA_HandleTypeDef* hdmarx = spi1_user_data.spi.hdmarx;

	if (__HAL_DMA_GET_FLAG(hdmarx, __HAL_DMA_GET_TE_FLAG_INDEX(hdmarx)) != RESET)
	{
		if(__HAL_DMA_GET_IT_SOURCE(hdmarx, DMA_IT_TE) != RESET){
			__HAL_DMA_CLEAR_FLAG(hdmarx, __HAL_DMA_GET_TE_FLAG_INDEX(hdmarx));
			__HAL_DMA_CLEAR_FLAG(hdmarx, __HAL_DMA_GET_TC_FLAG_INDEX(hdmarx));
			spi1_user_data.error_flag = 1;
		}
	}

	else if (__HAL_DMA_GET_FLAG(hdmarx, __HAL_DMA_GET_TC_FLAG_INDEX(hdmarx)) != RESET)
	{
		if(__HAL_DMA_GET_IT_SOURCE(hdmarx, DMA_IT_TC) != RESET){
			__HAL_DMA_CLEAR_FLAG(hdmarx, __HAL_DMA_GET_TC_FLAG_INDEX(hdmarx));
			rt_sem_release(spi1_user_data.lock);
		}
	}

	HAL_DMA_IRQHandler(spi1_user_data.spi.hdmarx);

	/* 退出中断 */
	rt_interrupt_leave();
}

void DMA1_Channel3_IRQHandler(void){
	/* 进入中断 */
	rt_interrupt_enter();

	DMA_HandleTypeDef* hdmatx = spi1_user_data.spi.hdmatx;

	if (__HAL_DMA_GET_FLAG(hdmatx, __HAL_DMA_GET_TE_FLAG_INDEX(hdmatx)) != RESET)
	{
		if(__HAL_DMA_GET_IT_SOURCE(hdmatx, DMA_IT_TE) != RESET){
			__HAL_DMA_CLEAR_FLAG(hdmatx, __HAL_DMA_GET_TE_FLAG_INDEX(hdmatx));
			__HAL_DMA_CLEAR_FLAG(hdmatx, __HAL_DMA_GET_TC_FLAG_INDEX(hdmatx));
			spi1_user_data.error_flag = 1;
		}
	}
	else if (__HAL_DMA_GET_FLAG(hdmatx, __HAL_DMA_GET_TC_FLAG_INDEX(hdmatx)) != RESET)
	{
		if(__HAL_DMA_GET_IT_SOURCE(hdmatx, DMA_IT_TC) != RESET){

			__HAL_DMA_CLEAR_FLAG(hdmatx, __HAL_DMA_GET_TC_FLAG_INDEX(hdmatx));

			rt_sem_release(spi1_user_data.lock);
		}
	}

	HAL_DMA_IRQHandler(spi1_user_data.spi.hdmatx);

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
