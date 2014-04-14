#ifndef __LED_DIGITAL
#define __LED_DIGITAL

#include <stm32f10x.h>
#include "stm32f10x_gpio.h"


enum {
	INFO_NULL = 0x80,
	INFO_C,
	INFO_UP,
	INFO_DOWN,
	INFO_LEFT,
	INFO_RIGHT
};	

enum {
	CONTENT_NUMBER = 0,
	CONTENT_INFO
};	

struct digital_ctrl {
	GPIO_TypeDef *gpio;
	unsigned int pin;
};	

void init_timer2(void);
void display_num(uint32_t num);
void disable_all_digital(void);
void display_switch(int content);
void display_info(uint8_t info);
void cancel_prev_info(void);
void clear_all_info(void);
int get_display_status(void);
void set_display_info_buffer(uint8_t *data, int len);

#endif
