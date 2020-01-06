/*!
    \file  systick.c
    \brief the systick configuration file
*/

/*
    Copyright (C) 2017 GigaDevice

    2017-06-23, V1.0.0, demo for GD32F30x
*/

#include "gd32f30x.h"
#include "systick.h"

volatile static uint32_t delay;

/*!
    \brief      configure systick
    \param[in]  none
    \param[out] none
    \retval     none
*/
void systick_config(void)
{
    /* setup systick timer for 1000Hz interrupts */
    if (SysTick_Config(SystemCoreClock / 1000U)){
        /* capture error */
        while (1){
        }
    }
    /* configure the systick handler priority */
    NVIC_SetPriority(SysTick_IRQn, 0x00U);
		
}

/*!
    \brief      delay a time in milliseconds
    \param[in]  count: count in milliseconds
    \param[out] none
    \retval     none
*/
void delay_1ms(uint32_t count)
{
    delay = count;

    while(0U != delay){
    }
}

/*!
    \brief      delay decrement
    \param[in]  none
    \param[out] none
    \retval     none
*/

void delay_decrement(void)
{
    if (0U != delay){
        delay--;
    }
}




//by tomi
void delaybm_us(uint32_t nus)
{
   uint32_t temp;
   SysTick->LOAD = 15*nus;
   SysTick->VAL=0X00;//��ռ�����
   SysTick->CTRL=0X01;//ʹ�ܣ����������޶����������ⲿʱ��Դ
   
   do
   {
    temp=SysTick->CTRL;//��ȡ��ǰ������ֵ
   }while((temp&0x01)&&(!(temp&(1<<16))));//�ȴ�ʱ�䵽��
       
   SysTick->CTRL=0x00; //�رռ�����
   SysTick->VAL =0X00; //��ռ�����
}

void delaybm_ms(uint16_t nms)
{
 uint32_t temp;
 SysTick->LOAD = 15000*nms;
 SysTick->VAL=0X00;//��ռ�����
 SysTick->CTRL=0X01;//ʹ�ܣ����������޶����������ⲿʱ��Դ
 do
 {
  temp=SysTick->CTRL;//��ȡ��ǰ������ֵ
 }while((temp&0x01)&&(!(temp&(1<<16))));//�ȴ�ʱ�䵽��
    SysTick->CTRL=0x00; //�رռ�����
    SysTick->VAL =0X00; //��ռ�����
}

