#ifndef __INIT_PERIPH
#define __INIT_PERIPH

void NVIC_config(void);
void EXTI_Configuration(void);
void init_gpio(void);
void enbale_periph_clock(void);
void init_iwdg(void);

#endif
