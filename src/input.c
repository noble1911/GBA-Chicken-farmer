#include "input.h"

u16 keys_held = 0;
u16 keys_pressed = 0;
u16 keys_released = 0;

static u16 prev_keys = 0;

void update_input() {
    u16 current_keys = ~REG_KEYINPUT & 0x03FF;
    
    keys_pressed = current_keys & (~prev_keys);
    keys_released = prev_keys & (~current_keys);
    keys_held = current_keys;
    
    prev_keys = current_keys;
}

int key_is_down(u16 key) {
    return keys_held & key;
}

int key_just_pressed(u16 key) {
    return keys_pressed & key;
}

int any_key_pressed() {
    return keys_pressed != 0;
}
