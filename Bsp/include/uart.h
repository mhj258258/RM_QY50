#ifndef _UART_H
#define _UART_H

#include "gd32f30x.h"
#include <stdio.h>


#define debug  1

//静态接收区的大小
#define BUFFER_SIZE     								 200

#define COMn                             2U

#define EVAL_COM1                        USART0
#define EVAL_COM1_CLK                    RCU_USART0
#define EVAL_COM1_TX_PIN                 GPIO_PIN_9
#define EVAL_COM1_RX_PIN                 GPIO_PIN_10
#define EVAL_COM1_GPIO_PORT              GPIOA
#define EVAL_COM1_GPIO_CLK               RCU_GPIOA

#define EVAL_COM2                        USART1
#define EVAL_COM2_CLK                    RCU_USART1
#define EVAL_COM2_TX_PIN                 GPIO_PIN_2
#define EVAL_COM2_RX_PIN                 GPIO_PIN_3
#define EVAL_COM2_GPIO_PORT              GPIOA
#define EVAL_COM2_GPIO_CLK               RCU_GPIOA

/* configure COM port */
void gd_eval_com_init(uint32_t com);
void user_uart_init(void);
void UART_DMA_TEST(void);
void USART1_DEBUG (char *fmt, ...);

extern uint8_t rx_buffer[];
extern uint16_t rx_counter;
extern uint8_t Rec_Start_Flag;
extern uint8_t Rec_Done_Flag;
#define PutStringToWifi  printf

#endif  


/**** Copyright (C)2019 tomi. All Rights Reserved **** END OF FILE ****/


