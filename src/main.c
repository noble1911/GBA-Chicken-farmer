#include "gba.h"
#include "graphics.h"
#include "input.h"
#include "game.h"
#include "audio.h"

int main() {
    init_video();
    init_audio();
    init_game();
    
    while (1) {
        // Do updates during active period
        update_input();
        update_game();
        // Wait until VBlank begins, then draw during VBlank
        vsync();
        draw_game();
    }
    
    return 0;
}
