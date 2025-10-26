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
        vsync();
        update_input();
        update_game();
        draw_game();
    }
    
    return 0;
}
