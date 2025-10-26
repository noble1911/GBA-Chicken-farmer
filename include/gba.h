#ifndef GBA_H
#define GBA_H

// GBA Hardware Definitions
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed char s8;  // CRITICAL: signed char for movement (ARM defaults char to unsigned)
typedef signed short s16;
typedef signed int s32;

// Display registers
#define REG_DISPCNT (*(volatile u16*)0x04000000)
#define REG_VCOUNT (*(volatile u16*)0x04000006)

// Display modes
#define MODE_3 0x0003
#define MODE_4 0x0004
#define BG2_ENABLE 0x0400

// Screen dimensions
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 160

// Video memory
#define VRAM 0x06000000

// For Mode 3 (16-bit color bitmap)
#define SCREEN_BUFFER ((volatile u16*)VRAM)

// Input registers
#define REG_KEYINPUT (*(volatile u16*)0x04000130)

// Button defines
#define KEY_A        0x0001
#define KEY_B        0x0002
#define KEY_SELECT   0x0004
#define KEY_START    0x0008
#define KEY_RIGHT    0x0010
#define KEY_LEFT     0x0020
#define KEY_UP       0x0040
#define KEY_DOWN     0x0080
#define KEY_R        0x0100
#define KEY_L        0x0200

// Color macros (for Mode 3 - RGB555 format)
#define RGB(r,g,b) ((r) | ((g)<<5) | ((b)<<10))

// Common colors
#define COLOR_BLACK   RGB(0,0,0)
#define COLOR_WHITE   RGB(31,31,31)
#define COLOR_RED     RGB(31,0,0)
#define COLOR_GREEN   RGB(0,31,0)
#define COLOR_BLUE    RGB(0,0,31)
#define COLOR_YELLOW  RGB(31,31,0)
#define COLOR_ORANGE  RGB(31,15,0)
#define COLOR_BROWN   RGB(15,7,0)
#define COLOR_SKIN    RGB(31,20,15)
#define COLOR_GRAY    RGB(15,15,15)

// VBlank function - wait for start of vblank period
// Wait until we ENTER VBlank, so rendering happens during VBlank
static inline void vsync() {
    // If we're already in VBlank, wait for it to end first
    while (REG_VCOUNT >= 160);
    // Then wait until the next VBlank begins
    while (REG_VCOUNT < 160);
}

#endif // GBA_H
