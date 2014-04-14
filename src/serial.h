#ifndef __SERIAL
#define __SERIAL

#include <stm32f10x.h>
#include <stm32f10x_usart.h>

#include <RTL.h> 


#define SERIAL1				0
#define SERIAL2				1
#define SERIAL3				2


struct serial_port_ctl {
	USART_TypeDef *uart;
	int sending;
	os_mbx_declare(rx_buf, 32);
	os_mbx_declare(tx_buf, 32);
};


void init_serial_port(void);
int write_serial(int port, uint8_t *buf, int len, int timeout);
int read_serial(int port, uint8_t *buf, int len, int timeout);

#endif

