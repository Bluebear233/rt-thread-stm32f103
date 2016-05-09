/**
 * file    spi.h
 * spi驱动
 *
 * Change Logs
 * Data           Author          Notes
 * 2016-5-9       bluebear233     第一版
 */

#ifndef ___SPI_H__
#define ___SPI_H__

#include "stm32f1xx_hal.h"

struct spi_device_user_data
{
	void (*cs)(GPIO_PinState PinState);
};

#endif
