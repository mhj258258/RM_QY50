//@program version: 001
//@project:QY50
//@update brief&time:20200104 init version



#include "gd32f30x.h"
#include "systick.h"
#include "led.h"
#include "key.h"
#include "uart.h"
#include "timer.h"
#include <stdio.h>
#include "hilink_mcu.h"

/* retarget the C library printf function to the USART */
int fputc(int ch, FILE *f)
{
    usart_data_transmit(EVAL_COM1, (uint8_t)ch);
    while(RESET == usart_flag_get(EVAL_COM1, USART_FLAG_TBE));
    return ch;
}
/************************************************S
函数名称 ： System_Initializes
功    能 ： 系统初始化
参    数 ： 无
返 回 值 ： 无
作    者 ： tomi
*************************************************/
void System_Initializes(void)
{
 // nvic_priority_group_set(NVIC_PRIGROUP_PRE1_SUB3);
  systick_config();
	user_led_init();
	user_key_init();
	user_uart_init();
	//user_timer_init();
}


/************************************************
函数名称 ： main
功    能 ： 主函数入口
参    数 ： 无
返 回 值 ： int
作    者 ： tomi
*************************************************/
int main(void)
{  	
	System_Initializes();
	USART1_DEBUG("\r\n 软件开始了 \r\n");
	
	USART1_DEBUG("\r\n ---------- \r\n");
#if 0
	USART1_DEBUG("\r\n start :%lld\r\n",Get_tick());
	delay50ms();
	USART1_DEBUG("\r\n stop :%lld\r\n",Get_tick());
#endif	
	//for hilink
	HiLinkMcuMain();
}
