#include <RTL.h>
#include <string.h>

#include "remote_control.h"
#include "ir_decode.h"
#include "led_digital.h"
#include "ticktack.h"


#define STATUS_RECV_CMD				1
#define STATUS_IDLE						2

#define TIMEOUT_TIME					5*100  //10ms
#define CMD_BUF_LENGTH				20


typedef void (*cmd_handler)(uint8_t *data, int data_len);

struct keycode_entry {
	uint8_t key_code;
	uint8_t char_code;
};

struct cmd_prefix {
	uint8_t prefix[5];
	uint8_t len;
	cmd_handler handler;
};	


void set_base_cmd(uint8_t *data, int data_len);


static uint8_t cmd_buf[CMD_BUF_LENGTH];
static int status = STATUS_IDLE;

const static struct keycode_entry keycode_lookup_table[] = {
	{RB_NUM_1, 1}, {RB_NUM_2, 2}, {RB_NUM_3, 3}, {RB_NUM_4, 4}, {RB_NUM_5, 5},
	{RB_NUM_6, 6}, {RB_NUM_7, 7}, {RB_NUM_8, 8}, {RB_NUM_9, 9}, {RB_NUM_0, 0},
	{RB_UP, INFO_UP}, {RB_DOWN, INFO_DOWN}, {RB_LEFT, INFO_LEFT}, {RB_RIGHT, INFO_RIGHT},
	{RB_C, INFO_C}, {RB_BACK, 0}, {RB_OK, 0}
};

const static struct cmd_prefix cmd_prefix_table[] = {
	{{INFO_C, INFO_UP, INFO_DOWN}, 3, set_base_cmd}
};


static const struct keycode_entry *lookup_keycode(uint8_t keycode)
{
	int i;
	
	for (i = 0; i < sizeof(keycode_lookup_table)/sizeof(struct keycode_entry); i++) {
		if (keycode == keycode_lookup_table[i].key_code)
			return &keycode_lookup_table[i];
	}	
	
	return NULL;
}

void set_base_cmd(uint8_t *data, int data_len)
{
	static const int multiplier[6] = {1, 10, 100, 1000, 10000, 100000};
	int i, value = 0;

	if (data_len > 5)
		return;
	
	for (i = 0; i < data_len; i++) {
		if (data[i] > 9)
			return;
		
		value += data[i]*multiplier[data_len-i-1];
	}	
	
	value *= 360;
	run_counter = value;
}

static void process_command(uint8_t *cmd, int len)
{
	int i, data_len;
	uint8_t *pdata;
	
	for (i = 0; i < sizeof(cmd_prefix_table)/sizeof(struct cmd_prefix); i++) {
		if (memcmp(cmd,  cmd_prefix_table[i].prefix, 
								cmd_prefix_table[i].len) == 0) {
				pdata = cmd+cmd_prefix_table[i].len;
				data_len = len-cmd_prefix_table[i].len;				
				cmd_prefix_table[i].handler(pdata, data_len);
		}	
	}	
}

__task void remote_control_handler(void)
{
	IR_Frame_TypeDef frame;
	OS_ID timeout_timer;
	int buf_idx;
	const struct keycode_entry *entry;
	
	while (1) {
		IR_Decode(&frame);
		
		if (status == STATUS_IDLE) {
			if (frame.data_code1 == RB_C) {
				clear_all_info();
				display_switch(CONTENT_INFO);
				
				timeout_timer = os_tmr_create(TIMEOUT_TIME, 0); 
				
				status = STATUS_RECV_CMD;
				
				buf_idx = 0;
				cmd_buf[buf_idx++] = INFO_C;
				display_info(INFO_C);
			}	
		}	else {
			os_tmr_kill(timeout_timer);
			timeout_timer = os_tmr_create(TIMEOUT_TIME, 0);
			
			if (frame.data_code1 == RB_OK) {
				status = STATUS_IDLE;
				display_switch(CONTENT_NUMBER);
				os_tmr_kill(timeout_timer);
				process_command(cmd_buf, buf_idx);
			} else if (frame.data_code1 == RB_BACK) {
				if (buf_idx > 1)
					buf_idx--;
				if (buf_idx > 6)
					set_display_info_buffer(&cmd_buf[buf_idx-6], 6);
				else
					set_display_info_buffer(cmd_buf, buf_idx);
			}	else {
				entry = lookup_keycode(frame.data_code1);
				if (entry != NULL) {
					if (buf_idx < CMD_BUF_LENGTH) {
						cmd_buf[buf_idx++] = entry->char_code;
						display_info(entry->char_code);
					}	
				}
			}
		}	
	}	
}

void os_tmr_call (U16 info) {
  /* This function is called when the user timer has expired. Parameter   */
  /* 'info' holds the value, defined when the timer was created.          */

  /* HERE: include optional user code to be executed on timeout. */
	
	display_switch(CONTENT_NUMBER);
	status = STATUS_IDLE;
}

