#include <stm32f10x_tim.h>
#include <string.h>

#include "led_digital.h"
#include "gpio_def.h"


static const uint8_t led_num_segment[10] = {
	/* 0     1     2     3     4 */ 
  	0xcf, 0x0c, 0x5b, 0x5d, 0x9c, 
	/* 5     6     7     8     9 */
		0xd5, 0xd7, 0x4c, 0xdf, 0xdd
};

static const uint8_t led_info_segment[6] = {
	/* NULL		C				UP			DOWN		LEFT		RIGTH */
		0x00,		0xc3,		0xc8,		0x98,		0xd0,		0x58
};	

static uint8_t number_buffer[6] = {0, 0, 0, 0, 0, 0};
static uint8_t info_buffer[6] = { INFO_NULL, INFO_NULL, INFO_NULL, 
																	INFO_NULL, INFO_NULL, INFO_NULL };

static int display_content = CONTENT_NUMBER;

static const struct digital_ctrl dig_ctrl[6] = {
	{GPIOC, LED_DIGIT1},
	{GPIOC, LED_DIGIT2},
	{GPIOB, LED_DIGIT3},
	{GPIOB, LED_DIGIT4},
	{GPIOB, LED_DIGIT5},
	{GPIOB, LED_DIGIT6},
};	


void init_timer2(void)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	
	TIM_DeInit(TIM2);
	
	 /* Time base configuration */
  TIM_TimeBaseStructure.TIM_Period = 36000;
  TIM_TimeBaseStructure.TIM_Prescaler = 3;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

  TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
	
	/* TIM IT enable */
  TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
	
	TIM_Cmd(TIM2, ENABLE);
}

/*
static void delay(void)
{
	int i;
	
	for (i = 0; i < 10; i++) {
		;
	}	
}
*/

static void shift_data(uint8_t data)
{
	int i, mask = 1;
	
	GPIOB->BRR = SHIFT_SDA;
	GPIOB->BRR = SHIFT_CLK;
	
	for (i = 0; i < 8; i++) {
		//delay();
		
		if (data&mask)
			GPIOB->BSRR = SHIFT_SDA;
		else
			GPIOB->BRR = SHIFT_SDA;
		
		//delay();
		GPIOB->BSRR = SHIFT_CLK;
		//delay();
		GPIOB->BRR = SHIFT_CLK;
		
		mask <<= 1;
	}	
}

void display_num(uint32_t num)
{
	int i;
	const int pos[6] = {1, 10, 100, 1000, 10000, 100000};
	
	TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);
	
	for (i = 0; i < 6; i++) 
		number_buffer[i] = num/pos[i]%10;
		
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
}

void disable_all_digital(void)
{
	int i;
	
	for (i = 0; i < 6; i++) 
			dig_ctrl[i].gpio->BSRR = dig_ctrl[i].pin;
}

void display_switch(int content)
{
	if (content != CONTENT_NUMBER && content != CONTENT_INFO)
		return;
	
	display_content = content;
}	

int get_display_status(void)
{
	return display_content;
}

void display_info(uint8_t info)
{
	int i;
	
	TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);
	
	for (i = 5; i > 0; i--) 
		info_buffer[i] = info_buffer[i-1];

	info_buffer[0] = info;
	
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
}

void cancel_prev_info(void)
{
	int i;
	
	TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);
	
	for (i = 0; i < 5; i++) 
		info_buffer[i] = info_buffer[i+1];
	
	info_buffer[5] = INFO_NULL;
	
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
}

void set_display_info_buffer(uint8_t *data, int len)
{
	int i, idx = 0;
	
	if (len > 6 || len < 1)
		return;
	
	TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);
	
	if (len == 6) {
		for (i = 0; i < 6; i++)
			info_buffer[5-i] = data[i];
	} else {
		for (i = 5; i >= 0; i--)
			if (i >= len)
				info_buffer[i] = INFO_NULL;
			else 
				info_buffer[i] = data[idx++];
	}		
	
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
}	

void clear_all_info(void)
{ 
	TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);
	memset(info_buffer, INFO_NULL, sizeof(info_buffer));
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
}

__irq void TIM2_IRQHandler(void)
{
	static unsigned int dig_pos = 0;
	unsigned int prev_dig;
	uint8_t seg;
	
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
		prev_dig = dig_pos == 0 ? 5 : dig_pos-1;
		
		dig_ctrl[prev_dig].gpio->BSRR = dig_ctrl[prev_dig].pin;
		
		if (display_content == CONTENT_NUMBER) {
			seg = dig_pos == 1 ? 
							led_num_segment[number_buffer[dig_pos]] | 0x20 :
							led_num_segment[number_buffer[dig_pos]];
		} else {
			seg = info_buffer[dig_pos] >= 0x80 ? 
							led_info_segment[info_buffer[dig_pos]-0x80] :
							led_num_segment[info_buffer[dig_pos]];
		}		
		
		shift_data(seg);
		
		dig_ctrl[dig_pos].gpio->BRR = dig_ctrl[dig_pos].pin;
			
		if (++dig_pos == 6)
			dig_pos = 0;
		
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}	
