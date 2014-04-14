#include <RTL.h> 
#include <string.h>

#include "serial.h"
#include "ticktack.h"
#include "comm.h"

#define FUBBER_LEN		50


static OS_MUT send_mutex;


static int receive_frame(uint8_t *frame)
{
	uint8_t data;
	int ret, idx;
	
	while (1) {
		ret =  read_serial(SERIAL1, &data, 1, 1);
		if (ret != 1 || data != 0x7e) 
			continue;
		
		idx = 0;
		frame[idx++] = data;
		
		while (1) {
			ret = read_serial(SERIAL1, frame+idx, 1, 1);
			if (ret != 1)
				break;
			
			if (frame[idx] == 0x7e)
				return idx+1;
			
			idx++;
			if (idx == FUBBER_LEN)
				break;;
		} 	
	}	
}

static int unpack_frame(uint8_t* src, uint8_t* dst, int len)
{
	int src_idx = 0, dst_idx = 0;
	
	while (src_idx < len) {
		if (src[src_idx] == 0x7d) {
			switch (src[src_idx+1]) {
			case 0x02:
				dst[dst_idx++] = 0x7e;
				break;
			case 0x01:
				dst[dst_idx++] = 0x7d;
				break;
			}	
			
			src_idx += 2;
		}	else
			dst[dst_idx++] = src[src_idx++];
	}	
	
	return dst_idx;
}

static int pack_frame(uint8_t* src, uint8_t* dst, int len)
{
	int src_idx = 1, dst_idx = 1;
	
	dst[0] = src[0];
	
	while (src_idx < (len-1)) {
		if (src[src_idx] == 0x7e) {
			dst[dst_idx++] = 0x7d;
			dst[dst_idx++] = 0x02;
		} else if (src[src_idx] == 0x7d) {
			dst[dst_idx++] = 0x7d;
			dst[dst_idx++] = 0x01;
		} else 
			dst[dst_idx++] = src[src_idx];
 		
		src_idx++;
	}	
	
	dst[dst_idx++] = 0x7e;
	return dst_idx;
}

static void send_frame(uint8_t *frame, int len)
{
	os_mut_wait(&send_mutex, 0xffff);
	write_serial(SERIAL1, frame, len, 2);  
	os_mut_release(&send_mutex);
}

static uint8_t check_sum(uint8_t *data, int len)
{
	int i;
	uint32_t sum = 0;
	
	for (i = 0; i < len; i++) 
		sum += data[i];

	return sum&0xff;
}	

static int construct_frame(uint8_t *buf, uint8_t cmd, uint8_t *data, uint8_t data_len)
{
	__packed struct frame_data *fd;
	int tail_pos;
	
	buf[0] = 0x7e;
	
	fd = (struct frame_data *)(buf+1);
	fd->version = U16_BE(0);
	fd->vendor_id = U16_BE(0);
	fd->device_type = 0x0b;
	fd->cmd = cmd;
	
	memcpy(fd->data, data, data_len);
	
	fd->check_sum = check_sum(buf+4, 4+data_len);
	
	tail_pos = sizeof(struct frame_data)+1+data_len;
	buf[tail_pos] = 0x7e;
	
	return tail_pos+1;
}

static void period_send(void)
{
	static uint8_t frame_buffer[20], send_buffer[40];
	uint32_t user_data;
	int frame_len, packed_len;
		
	user_data = U32_BE(run_counter/360);
	frame_len = construct_frame(frame_buffer, 0x71, 
															(uint8_t *)&user_data, 
															sizeof(uint32_t));		
	
	packed_len = pack_frame(frame_buffer, send_buffer, frame_len);
	send_frame(send_buffer, packed_len);
}

static void protocal_analysis(uint8_t *frame, int len)
{
	static uint8_t frame_buffer[20], send_buffer[40];
	__packed struct frame_data *fd;
	uint32_t user_data, time;
	uint8_t status;
	int frame_len, packed_len;
	
	fd = (struct frame_data *)(frame+1);
		
	switch (fd->cmd) {
	case 0x70:
			user_data = *(uint32_t *)fd->data;
			user_data = U32_BE(user_data);
			run_counter = user_data*360;
			
			status = 0;
			frame_len = construct_frame(frame_buffer, 0x70, &status, 1);		
			packed_len = pack_frame(frame_buffer, send_buffer, frame_len);
			send_frame(send_buffer, packed_len);
			break;
		
	case 0x71:
			time = U32_BE(run_counter/360);
			frame_len = construct_frame(frame_buffer, 0x71, 
																(uint8_t *)&time, 
																sizeof(uint32_t));		
			packed_len = pack_frame(frame_buffer, send_buffer, frame_len);
			send_frame(send_buffer, packed_len);
			break;
		
	default: ;
	}	
}

__task void comm_task(void)
{
	static uint8_t recv_buffer[FUBBER_LEN], unpacked_frame[FUBBER_LEN];
	int recv_len, frame_len;
		
	os_mut_init(&send_mutex);
	
	set_send_callback(period_send);
	
	while (1) {
		recv_len = receive_frame(recv_buffer);
		frame_len = unpack_frame(recv_buffer, unpacked_frame, recv_len);
		
		protocal_analysis(unpacked_frame, frame_len);
	}	
}
