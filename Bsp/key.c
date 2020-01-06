/*!
    \file  main.c
    \brief GPIO running led
*/

#include "gd32f30x.h"
#include "systick.h"
#include "led.h"
#include "key.h"
#include <stdio.h>

static uint32_t KEY_PORT[KEYn] = {WAKEUP_KEY_GPIO_PORT, 
                                  TAMPER_KEY_GPIO_PORT,
                                  USER_KEY_GPIO_PORT};
static uint32_t KEY_PIN[KEYn] = {WAKEUP_KEY_PIN, 
                                 TAMPER_KEY_PIN,
                                 USER_KEY_PIN};
static rcu_periph_enum KEY_CLK[KEYn] = {WAKEUP_KEY_GPIO_CLK, 
                                        TAMPER_KEY_GPIO_CLK,
                                        USER_KEY_GPIO_CLK};
static exti_line_enum KEY_EXTI_LINE[KEYn] = {WAKEUP_KEY_EXTI_LINE,
                                             TAMPER_KEY_EXTI_LINE,
                                             USER_KEY_EXTI_LINE};
static uint8_t KEY_PORT_SOURCE[KEYn] = {WAKEUP_KEY_EXTI_PORT_SOURCE,
                                        TAMPER_KEY_EXTI_PORT_SOURCE,
                                        USER_KEY_EXTI_PORT_SOURCE};
static uint8_t KEY_PIN_SOURCE[KEYn] = {WAKEUP_KEY_EXTI_PIN_SOURCE,
                                       TAMPER_KEY_EXTI_PIN_SOURCE,
                                       USER_KEY_EXTI_PIN_SOURCE};
static uint8_t KEY_IRQn[KEYn] = {WAKEUP_KEY_EXTI_IRQn, 
                                 TAMPER_KEY_EXTI_IRQn,
                                 USER_KEY_EXTI_IRQn};

/************************************************
函数名称 ： key_init
功    能 ： 主函数入口
参    数 ： 无
返 回 值 ： void
作    者 ： tomi
*************************************************/
void user_key_init(void)
{
	//KEY_TAMPER按键初始化为普通按键
	//gd_eval_key_init(KEY_TAMPER,KEY_MODE_GPIO);
	//KEY_TAMPER按键初始化为中断
	gd_eval_key_init(KEY_TAMPER,KEY_MODE_EXTI);
}	


/************************************************
函数名称 ： user_key_polling_test
功    能 ： 主函数入口
参    数 ： 无
返 回 值 ： void
作    者 ： tomi
*************************************************/
void user_key_polling_test(void)
{
   if(RESET == gd_eval_key_state_get(KEY_TAMPER))
		{
			/* delay 50ms for software removing jitter */
			delay_1ms(50);
			if(RESET == gd_eval_key_state_get(KEY_TAMPER))			
			{
				gpio_bit_write(LED2_GPIO_PORT, LED2_PIN, (bit_status)((1 - gpio_output_bit_get(LED2_GPIO_PORT, LED2_PIN))));
				while(RESET == gd_eval_key_state_get(KEY_TAMPER));
      }		
		}
}
																 
/************************************************
函数名称 ： 按键中断处理函数
功    能 ： 主函数入口
参    数 ： 无
返 回 值 ： void
作    者 ： tomi
*************************************************/
void EXTI10_15_IRQHandler(void)
{
    if(RESET != exti_interrupt_flag_get(EXTI_13))			
		{
        gd_eval_led_toggle(LED2);
    }
    printf("\r\n anjian anxiale  \r\n");
    exti_interrupt_flag_clear(EXTI_13);
}








/*!
    \brief      configure key
    \param[in]  key_num: specify the key to be configured
      \arg        KEY_TAMPER: tamper key
      \arg        KEY_WAKEUP: wakeup key
      \arg        KEY_USER: user key
    \param[in]  key_mode: specify button mode
      \arg        KEY_MODE_GPIO: key will be used as simple IO
      \arg        KEY_MODE_EXTI: key will be connected to EXTI line with interrupt
    \param[out] none
    \retval     none
*/
void gd_eval_key_init(key_typedef_enum key_num, keymode_typedef_enum key_mode)
{
    /* enable the key clock */
    rcu_periph_clock_enable(KEY_CLK[key_num]);
    rcu_periph_clock_enable(RCU_AF);

    /* configure button pin as input */
    gpio_init(KEY_PORT[key_num], GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, KEY_PIN[key_num]);

    if (key_mode == KEY_MODE_EXTI) {
        /* enable and set key EXTI interrupt to the lowest priority */
        nvic_irq_enable(KEY_IRQn[key_num], 2U, 0U);

        /* connect key EXTI line to key GPIO pin */
        gpio_exti_source_select(KEY_PORT_SOURCE[key_num], KEY_PIN_SOURCE[key_num]);

        /* configure key EXTI line */
        exti_init(KEY_EXTI_LINE[key_num], EXTI_INTERRUPT, EXTI_TRIG_FALLING);
        exti_interrupt_flag_clear(KEY_EXTI_LINE[key_num]);
    }
}

/*!
    \brief      return the selected key state
    \param[in]  key: specify the key to be checked
      \arg        KEY_TAMPER: tamper key
      \arg        KEY_WAKEUP: wakeup key
      \arg        KEY_USER: user key
    \param[out] none
    \retval     the key's GPIO pin value
*/
uint8_t gd_eval_key_state_get(key_typedef_enum key)
{
    return gpio_input_bit_get(KEY_PORT[key], KEY_PIN[key]);
}


