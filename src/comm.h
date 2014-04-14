#ifndef __COMM
#define __COMM

#include <stdint.h>

__packed struct frame_data {
	uint8_t check_sum;
	uint16_t version;
	uint16_t vendor_id;
	uint8_t device_type;
	uint8_t cmd;
	uint8_t data[];
};

void comm_task(void);

#endif
