#ifndef __IR_DECODE
#define __IR_DECODE

#include <stm32f10x_tim.h>


#define RB_NUM_1				0xa3
#define RB_NUM_2				0xa9
#define RB_NUM_3				0xad
#define RB_NUM_4				0xa2
#define RB_NUM_5				0xa8
#define RB_NUM_6				0xac
#define RB_NUM_7				0xae
#define RB_NUM_8				0xa4
#define RB_NUM_9				0xaa
#define RB_NUM_0				0x01
#define RB_UP						0x05
#define RB_DOWN					0x89
#define RB_LEFT					0x03
#define RB_RIGHT				0x0d
#define RB_C						0x83
#define RB_BACK					0x0b 
#define RB_OK						0x09

#define IR_TIME_OUT_US                18000 

#define IR_STATUS_HEADER              (1 << 1)
#define IR_STATUS_RX                  (1 << 0)
#define INITIAL_STATUS                IR_STATUS_HEADER 

#define IR_BIT_ERROR                  0xFF
#define IR_HEADER_ERROR               0xFF
#define IR_HEADER_OK                  0x00

#define IR_BITS_COUNT                 32
#define IR_TOTAL_BITS_COUNT           32

#define IR_ONTIME_MIN_US              (560 - 50)
#define IR_ONTIME_MAX_US              (560 + 150)

#define IR_HEADER_LOW_MIN_US          (9000 - 100)
#define IR_HEADER_LOW_MAX_US          (9000 + 100)
#define IR_HEADER_WHOLE_MIN_US        (9000 + 4500 - 500)
#define IR_HEADER_WHOLE_MAX_US        (9000 + 4500 + 500)

#define IR_VALUE_STEP_US              1125
#define IR_VALUE_MARGIN_US            100
#define IR_VALUE_00_US                1125

#define TIM_PRESCALER          				23 


typedef enum { 
	NO = 0, 
	YES = !NO
}StatusYesOrNo;

typedef struct {  
	uint8_t custom_code1;   
	uint8_t custom_code2;  
	uint8_t data_code1; 
	uint8_t data_code2; 
}IR_Frame_TypeDef;

typedef struct {
  uint8_t count;  /*!< Bit count */
  uint8_t status; /*!< Status */
  uint32_t data;  /*!< Data */
}tIR_packet;


void IR_Init(void);
void IR_ResetPacket(void);
void IR_Decode(IR_Frame_TypeDef *ir_frame);

#endif
