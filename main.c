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
�������� �� System_Initializes
��    �� �� ϵͳ��ʼ��
��    �� �� ��
�� �� ֵ �� ��
��    �� �� tomi
*************************************************/
void System_Initializes(void)
{
  nvic_priority_group_set(NVIC_PRIGROUP_PRE1_SUB3);
  systick_config();
	user_led_init();
	user_key_init();
	user_uart_init();
	user_timer_init();
}

void delay50(void)
{
	int i,j,k;
	for(i=0;i<50;i++)
	{
		for(j=0;j<50;j++)
		{
			for(k=0;k<50;k++)
			{
				;
			}
		}
	}
}
/************************************************
�������� �� main
��    �� �� ���������
��    �� �� ��
�� �� ֵ �� int
��    �� �� tomi
*************************************************/
int main(void)
{  	
	System_Initializes();
	USART1_DEBUG("\r\n �����ʼ�� \r\n");
	//for hilink
	while(1)
	{
		
	}
	
	
	//HiLinkMcuMain();
}
