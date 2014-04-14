#include <RTL.h>   

#include "gpio_def.h"
#include "led_digital.h"
#include "init_periph.h"
#include "save_count.h"
#include "serial.h"
#include "comm.h"
#include "ticktack.h"
#include "ir_decode.h"
#include "remote_control.h"


__task void initialize(void) 
{
	load_count_vlaue();
	
	init_gpio();
	init_serial_port();
	init_timer2();
	init_pvd();
	IR_Init();
	//init_iwdg();
	
	os_tsk_create(remote_control_handler, 1);
	os_tsk_create(comm_task, 2);
	os_tsk_create(ticktack, 1);
	
	os_tsk_delete_self();     
}

int main(void) 
{ 
	enbale_periph_clock();  
	EXTI_Configuration();
	NVIC_config();
		
	os_sys_init_prio(initialize, 1);   

	for (;;);

	return 0;
}


