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
#include <dfs_posix.h>
			

int main(void)
{
	if (dfs_mount("flash", "/", "elm", 0, 0) != 0){
		 rt_kprintf("flash File System init failed!\n");
	     return 0;
	}

	rt_kprintf("flash File System initialized!\n");

 	DIR *dir = opendir("SD");
 	if(dir == RT_NULL){
 		rt_kprintf("mount dir not exis \n");
 		rt_kprintf("creating mount dir　\n");

 		if(mkdir("/SD",0x777) < 0){
 			rt_kprintf("creat mount dir failed \n");
 			return 0;
 		}

 	}else{
 		rt_kprintf("mount dir exis \n");
 		closedir(dir);
 	}

	if (dfs_mount("sd0", "/SD", "elm", 0, 0) == 0)
	     rt_kprintf("SD File System initialized!\n");
	else
	     rt_kprintf("SD File System init failed!\n");

}
