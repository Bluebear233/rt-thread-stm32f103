/**
 * file    sd.c
 * SD卡
 *
 * Change Logs
 * Data           Author          Notes
 * 2016-5-19      bluebear233     第一版
 */

#include <rtdevice.h>
#include "stm32f1xx_hal.h"
#include "spi.h"

static struct rt_spi_device SD;

void sd_cs_control(GPIO_PinState PinState)
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, PinState);
}

const struct spi_device_user_data spi_sd_user_data =
{ sd_cs_control };

int spi_sd_init(void)
{
	//spi总线上注册spi设备
	rt_spi_bus_attach_device(&SD, "sd", "spi1", (void*) &spi_sd_user_data);

	GPIO_InitTypeDef gpio;
	gpio.Mode = GPIO_MODE_OUTPUT_PP;
	gpio.Pin = GPIO_PIN_3;
	gpio.Pull = GPIO_PULLUP;
	gpio.Speed = GPIO_SPEED_HIGH;
	HAL_GPIO_Init(GPIOA, &gpio);
	sd_cs_control (GPIO_PIN_SET);

	return RT_EOK;
}

INIT_DEVICE_EXPORT (spi_sd_init);
