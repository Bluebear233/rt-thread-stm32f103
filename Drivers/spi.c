/**
 * file    spi.c
 * spi驱动
 *
 * Change Logs
 * Data           Author          Notes
 * 2016-4-11      bluebear233     第一版
 * 2016-4-12      bludbear233     优化SPI传输函数
 */

#include <rtdevice.h>
#include <spi.h>
#include "stm32f1xx_hal.h"

struct rt_spi_bus *spi1;

struct stm32_spi_user_data
{
	SPI_HandleTypeDef spi;
	SPI_TypeDef *Instance;
};

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

rt_err_t stm32_spi_configure(struct rt_spi_device *device,
		struct rt_spi_configuration *configuration)
{
	struct stm32_spi_user_data *user_data = device->bus->parent.user_data;
	RT_ASSERT(user_data != RT_NULL);

	if (user_data->Instance != SPI1)
	{
		rt_kprintf("spi bus not find \n");
		RT_ASSERT(0);
	}

	SPI_HandleTypeDef *hspi = &user_data->spi;
	hspi->Instance = user_data->Instance;

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
	hspi->Instance = user_data->Instance;
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
rt_uint32_t stm32_spi_xfer(struct rt_spi_device *device,
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

const struct rt_spi_ops stm32_spi_ops =
{ stm32_spi_configure, stm32_spi_xfer };

int spi_init(void)
{
	struct stm32_spi_user_data *spi_user_data;
	spi_user_data = rt_malloc(sizeof(struct stm32_spi_user_data));
	RT_ASSERT(spi_user_data != RT_NULL);
	rt_memset(spi_user_data, 0, sizeof(struct stm32_spi_user_data));

	spi1 = rt_malloc(sizeof(struct rt_spi_bus));
	RT_ASSERT(spi1 != RT_NULL);
	rt_memset(spi1, 0, sizeof(struct rt_spi_bus));

	spi_user_data->Instance = SPI1;

	spi1->parent.user_data = spi_user_data;

	rt_spi_bus_register(spi1, "spi1", &stm32_spi_ops);

	return RT_EOK;
}
INIT_BOARD_EXPORT (spi_init);
