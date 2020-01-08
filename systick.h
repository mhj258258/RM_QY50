/*!
    \file  systick.h
    \brief the header file of systick
*/

/*
    Copyright (C) 2017 GigaDevice

    2017-06-23, V1.0.0, demo for GD32F30x
*/

#ifndef SYS_TICK_H
#define SYS_TICK_H

#include <stdint.h>

/* configure systick */
void systick_config(void);
/* delay a time in milliseconds */
void delay_1ms(uint32_t count);
/* delay decrement */
void delay_decrement(void);
//-------tomi--------
void delaybm_us(uint32_t nus);
void delaybm_ms(uint16_t nms);
void delay10ms(void);
void delay50ms(void);
void delay100ms(void);
//-------hilink------
unsigned long long Get_tick(void);

#endif /* SYS_TICK_H */
