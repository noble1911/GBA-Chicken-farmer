#include "audio.h"
#include "gba.h"

// GBA Sound Registers
#define REG_SOUNDCNT_X (*(volatile u16*)0x04000084)
#define REG_SOUNDCNT_H (*(volatile u16*)0x04000082)
#define REG_SOUNDCNT_L (*(volatile u16*)0x04000080)

// Sound Channel 1 (Square Wave with Sweep)
#define REG_SOUND1CNT_L (*(volatile u16*)0x04000060)
#define REG_SOUND1CNT_H (*(volatile u16*)0x04000062)
#define REG_SOUND1CNT_X (*(volatile u16*)0x04000064)

// Sound Channel 2 (Square Wave)
#define REG_SOUND2CNT_L (*(volatile u16*)0x04000068)
#define REG_SOUND2CNT_H (*(volatile u16*)0x0400006C)

// Sound Channel 3 (Wave)
#define REG_SOUND3CNT_L (*(volatile u16*)0x04000070)
#define REG_SOUND3CNT_H (*(volatile u16*)0x04000072)
#define REG_SOUND3CNT_X (*(volatile u16*)0x04000074)

// Sound Channel 4 (Noise)
#define REG_SOUND4CNT_L (*(volatile u16*)0x04000078)
#define REG_SOUND4CNT_H (*(volatile u16*)0x0400007C)

void init_audio() {
    // Enable sound
    REG_SOUNDCNT_X = 0x80;  // Master sound enable
    
    // Set up sound control
    REG_SOUNDCNT_L = 0x7777;  // Full volume for all channels
    REG_SOUNDCNT_H = 0x0002;  // Sound output ratio
}

void play_cluck_sound() {
    // Chicken cluck: Quick double-beep pattern
    // Channel 1: First "cluck"
    REG_SOUND1CNT_L = 0x0000;  // No sweep
    REG_SOUND1CNT_H = 0xF310;  // Duty 25%, full volume, short decay
    REG_SOUND1CNT_X = 0xC580;  // Medium-high frequency, reset, play
    
    // Channel 2: Second "cluck" (slightly lower)
    // This plays almost simultaneously for a richer sound
    REG_SOUND2CNT_L = 0xF210;  // Medium-high volume, very short decay
    REG_SOUND2CNT_H = 0xC560;  // Slightly lower frequency
}

void play_eat_sound() {
    // Eating/chomping sound
    // Channel 2: Quick low pop
    REG_SOUND2CNT_L = 0xF720;  // Medium volume, quick decay
    REG_SOUND2CNT_H = 0xC400;  // Low frequency
}

void play_happy_sound() {
    // Happy chirp: Quick ascending tone
    // Channel 1: Sweep up with bright sound
    REG_SOUND1CNT_L = 0x0078;  // Fast sweep up
    REG_SOUND1CNT_H = 0xF200;  // Duty 12.5%, full volume, quick decay
    REG_SOUND1CNT_X = 0xC650;  // High frequency, reset, play
}

void play_hatch_sound() {
    // Egg hatching: Cracking sound
    // Channel 4: Noise channel for crack effect
    REG_SOUND4CNT_L = 0xF740;  // Medium volume, medium decay
    REG_SOUND4CNT_H = 0xC010;  // Short noise burst
}
