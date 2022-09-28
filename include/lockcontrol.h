#include "stdint.h"

void init_lock_control();

enum {LOCK = 0, UNLOCK = 1};
void send_keypad_command(uint8_t state);

uint8_t current_lock_state();

bool lock_new_state_interrupt_triggered();