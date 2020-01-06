/*!
    \file  main.c
    \brief GPIO running led
*/

#include "gd32f30x.h"
//#include "bsp.h"
#include "systick.h"
#include "led.h"




/* private variables */
static uint32_t GPIO_PORT[LEDn] = {LED2_GPIO_PORT, LED3_GPIO_PORT,
                                   LED4_GPIO_PORT, LED5_GPIO_PORT};
static uint32_t GPIO_PIN[LEDn] = {LED2_PIN, LED3_PIN, LED4_PIN, LED5_PIN};

static rcu_periph_enum GPIO_CLK[LEDn] = {LED2_GPIO_CLK, LED3_GPIO_CLK, 
                                         LED4_GPIO_CLK, LED5_GPIO_CLK};



/************************************************
函数名称 ： led_init
功    能 ： 主函数入口
参    数 ： 无
返 回 值 ： void
作    者 ： tomi
*************************************************/
void user_led_init(void)
{
	  //初始化LED
		gd_eval_led_init(LED2);
    gd_eval_led_init(LED3);
    gd_eval_led_init(LED4);		
}
																				 
																				 
/*!
    \brief     led init
    \param[in]  none
    \param[out] none
    \retval     none
*/
void  gd_eval_led_init (led_typedef_enum lednum)
{
    /* enable the led clock */
    rcu_periph_clock_enable(GPIO_CLK[lednum]);
    /* configure led GPIO port */ 
    gpio_init(GPIO_PORT[lednum], GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ,GPIO_PIN[lednum]);

    GPIO_BC(GPIO_PORT[lednum]) = GPIO_PIN[lednum];
}

/*!
    \brief      turn on selected led
    \param[in]  lednum: specify the led to be turned on
      \arg        LED2
      \arg        LED3
      \arg        LED4
      \arg        LED5
    \param[out] none
    \retval     none
*/
void gd_eval_led_on(led_typedef_enum lednum)
{
    GPIO_BOP(GPIO_PORT[lednum]) = GPIO_PIN[lednum];
}

/*!
    \brief      turn off selected led
    \param[in]  lednum: specify the led to be turned off
      \arg        LED2
      \arg        LED3
      \arg        LED4
      \arg        LED5
    \param[out] none
    \retval     none
*/
void gd_eval_led_off(led_typedef_enum lednum)
{
    GPIO_BC(GPIO_PORT[lednum]) = GPIO_PIN[lednum];
}

/*!
    \brief      toggle selected led
    \param[in]  lednum: specify the led to be toggled
      \arg        LED2
      \arg        LED3
      \arg        LED4
      \arg        LED5
    \param[out] none
    \retval     none
*/
void gd_eval_led_toggle(led_typedef_enum lednum)
{
    gpio_bit_write(GPIO_PORT[lednum], GPIO_PIN[lednum], 
                    (bit_status)(1-gpio_input_bit_get(GPIO_PORT[lednum], GPIO_PIN[lednum])));
}



/*!
    \brief      main function
    \param[in]  none
    \param[out] none
    \retval     none
*/
void led_test(void)
{  
     /* turn on LED1 */
     gd_eval_led_on(LED2);
     /* insert 200 ms delay */
     delay_1ms(200);
        
     /* turn on LED2 */
     gd_eval_led_on(LED3);
     /* insert 200 ms delay */
     delay_1ms(200);
        
     /* turn on LED3 */
     gd_eval_led_on(LED4);
     /* insert 200 ms delay */
     delay_1ms(200);        

     /* turn off LEDs */
     gd_eval_led_off(LED2);
     gd_eval_led_off(LED3);
     gd_eval_led_off(LED4);
        
     /* insert 200 ms delay */
     delay_1ms(200);
    
}
