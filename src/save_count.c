#include <stm32f10x_pwr.h>
#include <stm32f10x_exti.h>
#include <stm32f10x_flash.h>
#include <stm32f10x_crc.h>

#include "save_count.h"
#include "led_digital.h"
#include "ticktack.h"
#include "gpio_def.h"


#define FLASH_SAVE_ADDR		0x0801f800


void init_pvd(void)
{
	/* Configure the PVD Level to 2.9V */
  PWR_PVDLevelConfig(PWR_PVDLevel_2V9);
  /* Enable the PVD Output */
  PWR_PVDCmd(ENABLE);
}

static void erase_flash(uint32_t addr)
{
	 /* Unlock the Flash Bank1 Program Erase controller */
  FLASH_UnlockBank1();

  /* Clear All pending flags */
  FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);	

  /* Erase the FLASH pages */
  FLASH_ErasePage(addr);
}

static void write_flash(uint32_t addr, uint32_t data[2])
{
	int i;
	
  for (i = 0; i < 2; i++) {
		FLASH_ProgramWord(addr, data[i]);
		addr += 4;
	}	

  //FLASH_LockBank1();
}

static void save_count_value(void)
{
	uint32_t save_data[2];
	
	save_data[0] = run_counter;
	CRC_ResetDR();
	save_data[1] = CRC_CalcBlockCRC(save_data, 1);
	write_flash(FLASH_SAVE_ADDR, save_data);
}

void load_count_vlaue(void)
{
	__IO uint32_t* addr;
	int i;
	uint32_t data[2];
	
	addr = (__IO uint32_t*)FLASH_SAVE_ADDR;
	for (i = 0; i < 2; i++) 
		data[i] = *(addr+i);
	
	erase_flash(FLASH_SAVE_ADDR);	
	
	CRC_ResetDR();
	if (CRC_CalcBlockCRC(data, 2) != 0) {
			run_counter = 0;
			return;
	}

	run_counter = data[0];
}

__irq void PVD_IRQHandler(void)
{
  if(EXTI_GetITStatus(EXTI_Line16) != RESET) {
		disable_all_digital();
		save_count_value();
		
		while(1);
		
		//EXTI_ClearITPendingBit(EXTI_Line16);
  }
}
