ALIENTEK MiniSTM32上rt-thread移植
===
* 使用原子MINISTM32开发板,MCU更换为STM32F103RF
* 使用STM32f1 HAL库
* 使用System Workbench for STM32 IDE

rt-thread配置
-------
* finsh和rt_kprintf映射到uart1,波特率为115200

IO配置 
------- 
      UART:UART1(轮询发送接收,中断发送接收,DMA发送接收)
      SPI:SPI1(轮询发送接收,DMA发送接收)

外设支持
-------
* W25QXX(挂载在SPI1)
* SD卡(挂载在SPI1)