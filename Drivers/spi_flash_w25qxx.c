/**
 * file    spi_flash_w25qxx.c
 * spi flash驱动
 *
 * Change Logs
 * Data           Author          Notes
 * 2016-4-11      bluebear233     第一版
 * 2016-4-12      bludbear233     修改设备注册方式
 * 2016-5-12      bluebear233     优化内存占用
 */

#include <rtdevice.h>
#include "stm32f1xx_hal.h"
#include "spi.h"



static struct rt_spi_device W25Q64;

void w25qxx_cs_control(GPIO_PinState PinState)
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, PinState);
}

const struct spi_device_user_data spi_cs =
{ w25qxx_cs_control };

int spi_flash_w25qxx_init(void)
{
	//spi总线上注册spi设备
	rt_spi_bus_attach_device(&W25Q64, "w25q64", "spi1", (void*) &spi_cs);

	GPIO_InitTypeDef gpio;
	gpio.Mode = GPIO_MODE_OUTPUT_PP;
	gpio.Pin = GPIO_PIN_2;
	gpio.Pull = GPIO_PULLUP;
	gpio.Speed = GPIO_SPEED_HIGH;
	HAL_GPIO_Init(GPIOA, &gpio);
	w25qxx_cs_control (GPIO_PIN_SET);

	return RT_EOK;
}

INIT_DEVICE_EXPORT (spi_flash_w25qxx_init);
