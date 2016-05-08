/* RT-Thread配置文件 */ 
#ifndef __RTTHREAD_CFG_H__ 
#define __RTTHREAD_CFG_H__



///* SECTION: device filesystem */
//#define RT_USING_DFS
//#define RT_USING_DFS_ELMFAT
//#define DFS_USING_WORKDIR
//#define RT_DFS_ELM_REENTRANT
//#define RT_DFS_ELM_WORD_ACCESS
//#define RT_DFS_ELM_DRIVES           1
//#define RT_DFS_ELM_USE_LFN          0 //这里一般设置为0,不使用长文件名,否则需要加入另外的源文件才能编译通过
//#define RT_DFS_ELM_MAX_LFN          255
//#define RT_DFS_ELM_MAX_SECTOR_SIZE  4096     //这里一定要与实际的spi flash一个扇区所包含的字节数相符,太小了会出现内存非法覆盖的情况
//
//
///* the max number of mounted filesystem */
//#define DFS_FILESYSTEMS_MAX         2
///* the max number of opened files       */
//#define DFS_FD_MAX                  4


/* 使用组件初始化 */
#define RT_USING_COMPONENTS_INIT

/* 使用组件的主函数 */
#define RT_USING_USER_MAIN

/* 使用串口驱动框架 */
#define RT_USING_SERIAL
//
///* 使用SPI驱动架构  */
//#define RT_USING_SPI

/* 使用finsh */
#define RT_USING_FINSH

/* 使用制表符 */
#define FINSH_USING_SYMTAB
#define FINSH_USING_DESCRIPTION

#define FINSH_USING_MSH
#define FINSH_USING_MSH_ONLY



/* 内核对象名称最大长度 */ 
#define RT_NAME_MAX 8 

/* 数据对齐长度 */ 
#define RT_ALIGN_SIZE 4 

/* 最大支持的优先级：32 */ 
#define RT_THREAD_PRIORITY_MAX 32 

/* 每秒的节拍数 */ 
#define RT_TICK_PER_SECOND 100

/* SECTION: 调试选项 */ 

/* 打开RT-Thread的ASSERT选项 */ 
#define RT_DEBUG 

/* 打开RT-Thread的线程栈溢出检查 */ 
#define RT_USING_OVERFLOW_CHECK 

/* 使用钩子函数 */ 
#define RT_USING_HOOK

/* SECTION: 线程间通信选项 */ 
/* 支持信号量 */ 
#define RT_USING_SEMAPHORE 

/* 支持互斥锁 */ 
#define RT_USING_MUTEX

/* 支持事件标志 */ 
#define RT_USING_EVENT 

/* 支持邮箱 */ 
#define RT_USING_MAILBOX 

/* 支持消息队列 */ 
#define RT_USING_MESSAGEQUEUE

/* SECTION: 内存管理 */ 
/* 支持静态内存池 */ 
#define RT_USING_MEMPOOL 

/* 支持动态内存堆管理 */ 
#define RT_USING_HEAP

/* 使用小型内存管理算法 */ 
#define RT_USING_SMALL_MEM

/* SECTION: 设备模块选项 */ 
/* 支持设备模块 */ 
#define RT_USING_DEVICE 

/* 支持串口1设备 */ 
#define RT_USING_UART1 

/* SECTION: 控制台选项 */ 
#define RT_USING_CONSOLE

/* 控制台缓冲区大小 */ 
#define RT_CONSOLEBUF_SIZE 128
#endif
