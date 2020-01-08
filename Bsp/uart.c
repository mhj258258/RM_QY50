#include "gd32f30x.h"
#include "systick.h"
#include "uart.h"
#include <stdio.h>
#include "led.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "hilink_mcu.h"



static rcu_periph_enum COM_CLK[COMn] = {EVAL_COM1_CLK, EVAL_COM2_CLK};
static uint32_t COM_TX_PIN[COMn] = {EVAL_COM1_TX_PIN, EVAL_COM2_TX_PIN};
static uint32_t COM_RX_PIN[COMn] = {EVAL_COM1_RX_PIN, EVAL_COM2_RX_PIN};
static uint32_t COM_GPIO_PORT[COMn] = {EVAL_COM1_GPIO_PORT, EVAL_COM2_GPIO_PORT};
static rcu_periph_enum COM_GPIO_CLK[COMn] = {EVAL_COM1_GPIO_CLK, EVAL_COM2_GPIO_CLK};
uint8_t Rec_Start_Flag = 1;
uint8_t Rec_Done_Flag = 1;
uint8_t tx_buffer[] = {0,1,2,3,4,5,6,7,8,9};
//#define BUFFER_SIZE   (COUNTOF(tx_buffer))
//#define COUNTOF(a)   (sizeof(a)/sizeof(*(a)))
uint8_t rx_buffer[BUFFER_SIZE];
uint16_t tx_counter = 0, rx_counter = 0;
uint32_t nbr_data_to_read = BUFFER_SIZE, nbr_data_to_send = BUFFER_SIZE;

//DMA��ʼ��
uint8_t tx_DMA_buffer[] = {0,1,2,3,4,5,6,7,8,9};
#define ARRAYNUM(arr_name)     (uint32_t)(sizeof(arr_name)/sizeof(*(arr_name)))
#define USART0_DATA_ADDRESS    ((uint32_t)0x40013804)
//uint8_t rx_DMA_buffer[ARRAYNUM(tx_DMA_buffer)];
void usart_dma_config(void);
ErrStatus memory_compare(uint8_t* src, uint8_t* dst, uint16_t length);
/************************************************
�������� �� user_uart_init
��    �� �� ���������
��    �� �� ��
�� �� ֵ �� int
��    �� �� tomi
*************************************************/
#define USART1_REC_LEN  			200  	//����USART1�������ֽ���
uint8_t USART1_RX_BUF[USART1_REC_LEN];     //���ջ���,���USART_REC_LEN���ֽ�.
//����״̬
//bit15��	������ɱ�־
//bit14��	���յ�0x0d
//bit13~0��	���յ�����Ч�ֽ���Ŀ
uint16_t USART1_RX_STA=0;       //����״̬���	  

void USART1_DEBUG (char *fmt, ...){ 
	char buffer[USART1_REC_LEN+1];  // ���ݳ���
	uint8_t i = 0;	
	va_list arg_ptr;
	va_start(arg_ptr, fmt);  
	vsnprintf(buffer, USART1_REC_LEN+1, fmt, arg_ptr);
	while ((i < USART1_REC_LEN) && (i < strlen(buffer))){
        usart_data_transmit(USART1, (uint8_t) buffer[i++]);
        while ( usart_flag_get(USART1, USART_FLAG_TC) == RESET); 
	}
	va_end(arg_ptr);
}
void user_uart_init(void)
{
			/* USART interrupt configuration */
    nvic_irq_enable(USART0_IRQn, 0, 0);//�ж�����
		//����1��ʼ��
		gd_eval_com_init(EVAL_COM1);
		//����2��ʼ��
		gd_eval_com_init(EVAL_COM2);
	
		/* enable USART0 receive interrupt */
		usart_interrupt_enable(USART0, USART_INT_RBNE);//���ջ������ǿ��жϱ�־
	//	usart_interrupt_disable(USART0, USART_INT_IDLE);//���ջ������ǿ��жϱ�־USART_INT_FLAG_IDLE

}



/*!
    \brief      configure COM port
    \param[in]  com: COM on the board
      \arg        EVAL_COM1: COM1 on the board
      \arg        EVAL_COM2: COM2 on the board
    \param[out] none
    \retval     none
*/
void gd_eval_com_init(uint32_t com)
{
    uint32_t com_id = 0U;
    if(EVAL_COM1 == com){
        com_id = 0U;
    }else if(EVAL_COM2 == com){
        com_id = 1U;
    }
    
    /* enable GPIO clock */
    rcu_periph_clock_enable(COM_GPIO_CLK[com_id]);

    /* enable USART clock */
    rcu_periph_clock_enable(COM_CLK[com_id]);

    /* connect port to USARTx_Tx */
    gpio_init(COM_GPIO_PORT[com_id], GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, COM_TX_PIN[com_id]);

    /* connect port to USARTx_Rx */
    gpio_init(COM_GPIO_PORT[com_id], GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, COM_RX_PIN[com_id]);

    /* USART configure */
    usart_deinit(com);
    usart_baudrate_set(com, 115200U);
    usart_receive_config(com, USART_RECEIVE_ENABLE);
    usart_transmit_config(com, USART_TRANSMIT_ENABLE);
    usart_enable(com);
}


/*!
    \brief      configure USART DMA
    \param[in]  none
    \param[out] none
    \retval     none
*/
void usart_dma_config(void)
{
    dma_parameter_struct dma_init_struct;
    /* enable DMA0 */
    rcu_periph_clock_enable(RCU_DMA0);
    /* deinitialize DMA channel3(USART0 tx) */
    dma_deinit(DMA0, DMA_CH3);//����ͨ����4��
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
    dma_init_struct.memory_addr = (uint32_t)tx_DMA_buffer;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
    dma_init_struct.number = 200;
    dma_init_struct.periph_addr = USART0_DATA_ADDRESS;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
    dma_init(DMA0, DMA_CH3, dma_init_struct);
    /* configure DMA mode */
    dma_circulation_disable(DMA0, DMA_CH3);//DMAѭ��ģʽ����
    dma_memory_to_memory_disable(DMA0, DMA_CH3);
    
    dma_deinit(DMA0, DMA_CH4);
    dma_init_struct.direction = DMA_PERIPHERAL_TO_MEMORY;
    dma_init_struct.memory_addr = (uint32_t)rx_buffer;//(uint32_t)rx_DMA_buffer;
    dma_init(DMA0, DMA_CH4, dma_init_struct);
    /* configure DMA mode */
    dma_circulation_enable(DMA0, DMA_CH4);
    dma_memory_to_memory_disable(DMA0, DMA_CH4);
}


/*!
    \brief      this function handles USART0 exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void USART0_IRQHandler(void)
{
		uint8_t data = 0;
		if(RESET != usart_interrupt_flag_get(USART0, USART_INT_FLAG_RBNE))
		{
			usart_interrupt_flag_clear(USART0, USART_INT_FLAG_RBNE);
			//usart_interrupt_enable(USART0, USART_INT_IDLE);//���ջ������ǿ��жϱ�־
			Rec_Start_Flag = 1;
      data = (uint8_t)usart_data_receive(USART0);
			//rx_buffer[rx_counter++] = data;
			//------------hilink------------
			//USART1_DEBUG("data = %x\r\n",data);
			HiLinkUartRcvOneByte(data);
			//------------------------------	
#if 0			
			if(data == 0x55)
			{
				USART1_DEBUG("here----\r\n");
				HiLinkModuleReboot();	
			}
			else if(data == 0x88)
			{
				USART1_DEBUG("here2----\r\n");
				HiLinkModuleReset();	
			}
			
      if(rx_counter >= nbr_data_to_read)
      {
            /* disable the USART0 receive interrupt */
            //usart_interrupt_disable(USART0, USART_INT_RBNE);
					  rx_counter = 0;
      }
#endif

    }      
//		if(RESET != usart_interrupt_flag_get(USART0, USART_INT_FLAG_IDLE))
//		{
//			usart_interrupt_flag_clear(USART0, USART_INT_FLAG_IDLE);
//			usart_interrupt_disable(USART0, USART_INT_IDLE);//���ջ������ǿ��жϱ�־
//			Rec_Done_Flag = 1;
//		}
}




/*!
    \brief      memory compare function
    \param[in]  src: source data
    \param[in]  dst: destination data
    \param[in]  length: the compare data length
    \param[out] none
    \retval     ErrStatus: ERROR or SUCCESS
*/
ErrStatus memory_compare(uint8_t* src, uint8_t* dst, uint16_t length) 
{
    while(length--){
        if(*src++ != *dst++){
            return ERROR;
        }
    }
    return SUCCESS;
}












