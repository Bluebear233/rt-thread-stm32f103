/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/


#include "stm32f1xx.h"
#include "rtthread.h"
#include <dfs.h>
#include <dfs_fs.h>
#include <dfs_file.h>
			

int main(void)
{
	  if (dfs_mount("flash", "/", "elm", 0, 0) == 0)
	      rt_kprintf("File System initialized!\n");
	  else
	      rt_kprintf("File System init failed!\n");
}
