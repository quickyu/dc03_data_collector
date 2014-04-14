#include <RTL.h>
#include <stm32f10x_tim.h>
#include <stm32f10x_rcc.h>

#include "ir_decode.h"
#include "gpio_def.h"


static __IO StatusYesOrNo IRFrameReceived = NO;   /*!< IR frame status */
static OS_SEM ir_frame_sem;

  
/* IR bit definitions*/
static uint16_t IROnTimeMin = 0; 
static uint16_t IROnTimeMax = 0; 
static uint16_t IRValueStep = 0;
static uint16_t IRValueMargin = 0;
static uint16_t IRValue00 = 0;

/* Header time definitions*/
static uint16_t IRHeaderLowMin = 0;
static uint16_t IRHeaderLowMax = 0;
static uint16_t IRHeaderWholeMin = 0;
static uint16_t IRHeaderWholeMax = 0;
static tIR_packet IRTmpPacket; /*!< IR packet*/

static __IO uint32_t TIMCLKValueKHz = 0;
static uint16_t IRTimeOut = 0;


static uint32_t TIM_GetCounterCLKValue(void)
{
  uint32_t apbprescaler = 0, apbfrequency = 0;
  uint32_t timprescaler = 0;
  __IO RCC_ClocksTypeDef RCC_ClockFreq;
  
  /* This function fills the RCC_ClockFreq structure with the current
  frequencies of different on chip clocks */
  RCC_GetClocksFreq((RCC_ClocksTypeDef*)&RCC_ClockFreq);
  
	/* Get the clock prescaler of APB1 */
	apbprescaler = ((RCC->CFGR >> 8) & 0x7);
	apbfrequency = RCC_ClockFreq.PCLK1_Frequency; 
	timprescaler = TIM_PRESCALER;
  
  /* If APBx clock div >= 4 */
  if (apbprescaler >= 4)
		return ((apbfrequency * 2)/(timprescaler + 1));
  else
    return (apbfrequency/(timprescaler+ 1));
}

void IR_ResetPacket(void)
{
  IRTmpPacket.count = 0;
  IRTmpPacket.status = INITIAL_STATUS;
  IRTmpPacket.data = 0;
}

void IR_Init(void)
{
  TIM_ICInitTypeDef TIM_ICInitStructure;
  
  /* TIMER frequency input */
  TIM_PrescalerConfig(TIM3, TIM_PRESCALER, TIM_PSCReloadMode_Immediate);

  /* TIM configuration */
  TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
  TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Falling;
  TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
  TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
  TIM_ICInitStructure.TIM_ICFilter = 0x0;
  TIM_PWMIConfig(TIM3, &TIM_ICInitStructure); 

  /* Timer Clock */
  TIMCLKValueKHz = TIM_GetCounterCLKValue()/1000; 

  /* Select the TIM3 Input Trigger: TI2FP2 */
  TIM_SelectInputTrigger(TIM3, TIM_TS_TI2FP2);

  /* Select the slave Mode: Reset Mode */
  TIM_SelectSlaveMode(TIM3, TIM_SlaveMode_Reset);

  /* Enable the Master/Slave Mode */
  TIM_SelectMasterSlaveMode(TIM3, TIM_MasterSlaveMode_Enable);

  /* Configures the TIM Update Request Interrupt source: counter overflow */
  TIM_UpdateRequestConfig(TIM3,  TIM_UpdateSource_Regular);
   
  IRTimeOut = TIMCLKValueKHz * IR_TIME_OUT_US/1000;

  /* Set the TIM auto-reload register for each IR protocol */
  TIM3->ARR = IRTimeOut;
  
  /* Clear update flag */
  TIM_ClearFlag(TIM3, TIM_FLAG_Update);

  /* Enable TIM3 Update Event Interrupt Request */
  TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

  /* Enable the CC2/CC1 Interrupt Request */
  TIM_ITConfig(TIM3, TIM_IT_CC2, ENABLE);

  /* Enable the timer */
  TIM_Cmd(TIM3, ENABLE);
  
  /* Header */	
  IRHeaderLowMin = TIMCLKValueKHz * IR_HEADER_LOW_MIN_US/1000;
  IRHeaderLowMax = TIMCLKValueKHz * IR_HEADER_LOW_MAX_US/1000;
  IRHeaderWholeMin = TIMCLKValueKHz * IR_HEADER_WHOLE_MIN_US/1000;
  IRHeaderWholeMax = TIMCLKValueKHz * IR_HEADER_WHOLE_MAX_US/1000;

  /* Bit time range*/
  IROnTimeMax = TIMCLKValueKHz * IR_ONTIME_MAX_US /1000;
  IROnTimeMin = TIMCLKValueKHz * IR_ONTIME_MIN_US/1000; 
  IRValueStep = TIMCLKValueKHz * IR_VALUE_STEP_US/1000;
  IRValueMargin = TIMCLKValueKHz * IR_VALUE_MARGIN_US/1000;
  IRValue00 = TIMCLKValueKHz * IR_VALUE_00_US/1000;

  /* Default state */
  IR_ResetPacket();
	
	os_sem_init (&ir_frame_sem, 0);
}

void IR_Decode(IR_Frame_TypeDef *ir_frame)
{  
	os_sem_wait (&ir_frame_sem, 0xffff);
	
	TIM_ITConfig(TIM3, TIM_IT_CC2, DISABLE);
	
	ir_frame->custom_code1 = __RBIT(IRTmpPacket.data) & 0xff;
	ir_frame->custom_code1 = (__RBIT(IRTmpPacket.data) >> 8) & 0xff;
	ir_frame->data_code1 = (__RBIT(IRTmpPacket.data) >> 16) & 0xff;
	ir_frame->data_code2 = __RBIT(IRTmpPacket.data) >> 24;
    
	IR_ResetPacket();
	
	TIM_ITConfig(TIM3, TIM_IT_CC2, ENABLE);
}

static uint8_t IR_DecodeHeader(uint32_t lowPulseLength, uint32_t wholePulseLength)
{
  /* First check if low pulse time is according to the specs */
  if ((lowPulseLength >= IRHeaderLowMin) && 
					(lowPulseLength <= IRHeaderLowMax)) {
    /* Check if the high pulse is OK */
    if ((wholePulseLength >= IRHeaderWholeMin) && 
					(wholePulseLength <= IRHeaderWholeMax)) 
      return IR_HEADER_OK; /* Valid Header */
  }	
	
  return IR_HEADER_ERROR;  /* Wrong Header */
}

static uint8_t IR_DecodeBit(uint32_t lowPulseLength , uint32_t wholePulseLength)
{
  uint8_t i = 0;
  
  /* First check if low pulse time is according to the specs */
  if ((lowPulseLength >= IROnTimeMin) && (lowPulseLength <= IROnTimeMax)) {
		/* There are data0 to data1 */
    for (i = 0; i < 2; i++) {
      /* Check if the length is in given range */
      if ((wholePulseLength >= IRValue00 + (IRValueStep * i) - IRValueMargin) 
          && (wholePulseLength <= IRValue00 + (IRValueStep * i) + IRValueMargin))
        return i; /* Return bit value */
    }
  }
	
  return IR_BIT_ERROR; /* No correct pulse length was found */
}

static void IR_DataSampling(uint32_t lowPulseLength, uint32_t wholePulseLength)
{
  uint8_t  tmpBit = 0;
  
  /* If the pulse timing is correct */
  if (IRTmpPacket.status == IR_STATUS_RX) {
    /* Convert raw pulse timing to data value */
    tmpBit = IR_DecodeBit(lowPulseLength, wholePulseLength); 
		/* If the pulse timing is correct */
    if (tmpBit != IR_BIT_ERROR) {
      /* This operation fills in the incoming bit to the correct position in
      32 bit word*/
      IRTmpPacket.data |= tmpBit;
      IRTmpPacket.data <<= 1;
      /* Increment the bit count  */
      IRTmpPacket.count++;
    }
    /* If all bits identified */
    if (IRTmpPacket.count == IR_TOTAL_BITS_COUNT) {
      /* Frame received*/
      //IRFrameReceived = YES;
			 isr_sem_send(&ir_frame_sem);
    } else if (IRTmpPacket.count == IR_BITS_COUNT) {    /* Bit 15:the idle time between IR message and the inverted one */
      IRTmpPacket.status = IR_STATUS_HEADER; 
    }
  } else if (IRTmpPacket.status == IR_STATUS_HEADER) {
    /* Convert raw pulse timing to data value */
    tmpBit = IR_DecodeHeader(lowPulseLength, wholePulseLength);
    
    /* If the Header timing is correct */
    if (tmpBit != IR_HEADER_ERROR) { 
      /* Reception Status for the inverted message */
      IRTmpPacket.status = IR_STATUS_RX; 
    } else {
      /* Wrong header time */
      IR_ResetPacket();  
    }
  }
}

void TIM3_IRQHandler(void)
{
  static uint32_t ICValue1;
  static uint32_t ICValue2;
  
  if (TIM_GetFlagStatus(TIM3, TIM_FLAG_CC2) != RESET) {   /* IC2 Interrupt */ 
    TIM_ClearFlag(TIM3, TIM_FLAG_CC2);
    /* Get the Input Capture value */
		ICValue1 = TIM_GetCapture1(TIM3);
		ICValue2 = TIM_GetCapture2(TIM3);
		IR_DataSampling(ICValue1, ICValue2); 
  } else if (TIM_GetFlagStatus(TIM3, TIM_FLAG_Update) != RESET) {  /* Checks whether the IR_TIM flag is set or not. */
    /* Clears the IR_TIM's pending flags*/
    TIM_ClearFlag(TIM3, TIM_FLAG_Update);
    IR_ResetPacket();
  }
}
