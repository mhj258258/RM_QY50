#include "gd32f30x.h"
#include "systick.h"
#include "uart.h"
#include <stdio.h>
#include "led.h"
#include "timer.h"




void nvic_config(void);
void timer_config(void);
/**
    \brief      user timer init
    \param[in]  none
    \param[out] none
    \retval     none
  */
void user_timer_init(void)
{
	nvic_config();
	timer_config();
}

/**
    \brief      configure the nested vectored interrupt controller
    \param[in]  none
    \param[out] none
    \retval     none
  */
void nvic_config(void)
{
    nvic_irq_enable(TIMER0_UP_IRQn, 0, 1);
}
/**
    \brief      configure the TIMER peripheral
    \param[in]  none
    \param[out] none
    \retval     none
  */
void timer_config(void)
{
    /* TIMER0 configuration: generate PWM signals with different duty cycles:
       TIMER0CLK = SystemCoreClock / 120 = 1MHz */
    timer_parameter_struct timer_initpara;

    rcu_periph_clock_enable(RCU_TIMER0);
    timer_deinit(TIMER0);

    /* TIMER0 configuration */
    timer_initpara.prescaler         = 479;//·ÖÆµ 479 - 200ms        ((1199+1)*50000)/ 120000000 =  500ms 
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = 50000;//ÖÜÆÚ
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER0,&timer_initpara);

		/* TIMER0 channel control update interrupt enable */
    timer_interrupt_enable(TIMER0,TIMER_INT_UP);
    /* TIMER0 break interrupt disable */
    timer_interrupt_disable(TIMER0,TIMER_INT_BRK);
		
    /* auto-reload preload enable */
    timer_auto_reload_shadow_enable(TIMER0);
    timer_enable(TIMER0);
}



/**
    \brief      interrupt deal
    \param[in]  none
    \param[out] none
    \retval     none
  */
 void TIMER0_UP_IRQHandler(void)
{
     if(timer_interrupt_flag_get (TIMER0, TIMER_INT_FLAG_UP)!= RESET)
		 {
			 
			 
		 }
		 
		 
		 timer_interrupt_flag_clear (TIMER0, TIMER_INT_FLAG_UP);
	
}


























