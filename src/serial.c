#include <RTL.h>  

#include "serial.h"


static struct serial_port_ctl serial_port[1] = {
		{USART2, 0}
};


void init_serial_port(void)
{
	int i;
	USART_InitTypeDef usart_init; 

	/* configured as follow:
        - BaudRate = 9600 baud  
        - Word Length = 8 Bits
        - One Stop Bit
        - No parity
        - Hardware flow control disabled (RTS and CTS signals)
        - Receive and transmit enabled
	*/
	usart_init.USART_BaudRate = 115200;
	usart_init.USART_WordLength = USART_WordLength_8b;
	usart_init.USART_StopBits = USART_StopBits_1;
	usart_init.USART_Parity = USART_Parity_No;
	usart_init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	usart_init.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	
	for (i = 0; i < sizeof(serial_port)/sizeof(struct serial_port_ctl); i++) {
		os_mbx_init(serial_port[i].rx_buf, sizeof(serial_port[i].rx_buf));
		os_mbx_init(serial_port[i].tx_buf, sizeof(serial_port[i].tx_buf));

		serial_port[i].sending = 0;

		USART_Init(serial_port[i].uart, &usart_init);
	
		USART_ITConfig(serial_port[i].uart, USART_IT_RXNE, ENABLE);
		USART_ITConfig(serial_port[i].uart, USART_IT_TXE, DISABLE);

		USART_Cmd(serial_port[i].uart, ENABLE);		
	}	
}

int write_serial(int port, uint8_t *buf, int len, int timeout)  
{     
	int i;
	    
	for (i = 0; i < len; i++) {
		USART_ITConfig(serial_port[port].uart, USART_IT_TXE, DISABLE);
		
		if (!serial_port[port].sending) {
			serial_port[port].sending = 1;
			USART_SendData(serial_port[port].uart, buf[i]);
		} else {
			if (os_mbx_send(serial_port[port].tx_buf, (void *)buf[i], 0) != OS_R_OK) { 
				USART_ITConfig(serial_port[port].uart, USART_IT_TXE, ENABLE);
				break;
			}	
		}	

		USART_ITConfig(serial_port[port].uart, USART_IT_TXE, ENABLE);
	}

	return i;
}

int read_serial(int port, uint8_t *buf, int len, int timeout)  
{                 
  void *data;
	int i;

	for (i = 0; i < len; i++) {
		if (os_mbx_wait(serial_port[port].rx_buf, &data, timeout) == OS_R_TMO)
			return i;

 		buf[i] = (uint8_t)data;
	}

	return i;
} 

__irq void USART2_IRQHandler(void)
{
	static uint8_t recv_data;
	static void *send_data;

	if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) {
		recv_data = USART_ReceiveData(USART2);
		if (isr_mbx_check(serial_port[SERIAL1].rx_buf) != 0)
			isr_mbx_send(serial_port[SERIAL1].rx_buf, (void *)recv_data);
	}

	if (USART_GetITStatus(USART2, USART_IT_TXE) != RESET) { 
		if (isr_mbx_receive(serial_port[SERIAL1].tx_buf, &send_data) == OS_R_MBX)
			USART_SendData(USART2, (uint8_t)send_data);
  		else {
		    serial_port[SERIAL1].sending = 0;
				USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
  		}	
	}	
}

