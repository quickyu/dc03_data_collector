#ifndef __TICKTACK
#define __TICKTACK

#include <stdint.h>

typedef void (*send_callback)(void);

void ticktack(void);
void set_send_callback(send_callback cb);

extern uint32_t run_counter;

#endif
