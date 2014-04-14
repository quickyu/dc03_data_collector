#include <RTL.h>
#include <stm32f10x.h>
#include <stm32f10x_iwdg.h>

#include "gpio_def.h"
#include "ticktack.h"
#include "led_digital.h"


uint32_t run_counter = 0;

static send_callback callback = NULL;


void set_send_callback(send_callback cb)
{
	callback = cb;
}

__task void ticktack(void)
{
	unsigned int blink_cnt = 0, timing_cnt = 0, send_cnt = 0; 
	
	os_itv_set(1);

	while (1) {
		if (++blink_cnt == 50) {
			blink_cnt = 0;
			GPIOA->ODR ^= RUN_LED;
		}	
		
		if (++timing_cnt == 100) {
			timing_cnt = 0;
			
			run_counter++;
			display_num(run_counter/360);
		}
		
		if (++send_cnt == 1000) {
			send_cnt = 0;
			
			if (callback)
				callback();
		}	
		
		//IWDG_ReloadCounter(); 
		
		os_itv_wait();
	}    
}
