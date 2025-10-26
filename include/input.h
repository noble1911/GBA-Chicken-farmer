#ifndef INPUT_H
#define INPUT_H

#include "gba.h"

// Input state variables
extern u16 keys_held;
extern u16 keys_pressed;
extern u16 keys_released;

// Input functions
void update_input();
int key_is_down(u16 key);
int key_just_pressed(u16 key);
int any_key_pressed();

#endif // INPUT_H
