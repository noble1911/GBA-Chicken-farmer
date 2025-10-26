#include "game.h"
#include "graphics.h"
#include "input.h"
#include "audio.h"

// Forward declarations
void chicken_eat_food(Chicken* c, Food* f);
void chicken_lay_egg(Chicken* c);
void create_corpse(s16 x, s16 y);
void update_corpses();

// Game state
Chicken chickens[MAX_CHICKENS];
Food foods[MAX_FOOD];
Egg eggs[MAX_EGGS];
Corpse corpses[MAX_CORPSES];
int chicken_count = 0;
int food_count = 0;
int egg_count = 0;
int corpse_count = 0;
int generation_count = 1;
u32 frames = 0;
// Global speed multiplier (1 = normal). Increase to speed up timers for testing.
static u8 game_speed = 3;
void set_game_speed(u8 speed) { game_speed = speed ? speed : 1; }
// Lower chicken update/draw load by updating chickens in stripes (buckets) each frame
// 1 = update all every frame (full 60fps); 2 = ~30fps per chicken; 3 = ~20fps per chicken
#ifndef CHICKEN_UPDATE_STRIDE
#define CHICKEN_UPDATE_STRIDE 3
#endif

// Track chicken active state across frames to handle immediate cleanup on death
static u8 prev_chicken_active[MAX_CHICKENS];

// Cursor
s16 cursor_x = SCREEN_WIDTH / 2;
s16 cursor_y = SCREEN_HEIGHT / 2;
FoodType selected_food = FOOD_SEEDS;

// Simple pseudo-random number generator
u32 rand_seed = 12345;
u32 simple_rand() {
    rand_seed = (rand_seed * 1103515245 + 12345) & 0x7fffffff;
    return rand_seed;
}

u8 rand_range(u8 min, u8 max) {
    return min + (simple_rand() % (max - min + 1));
}

// Genetics functions
u8 mutate_gene(u8 value) {
    // 30% chance to mutate
    if (simple_rand() % 100 < 30) {
        s8 change = (simple_rand() % 3) - 1; // -1, 0, or 1
        s16 new_val = value + change;
        if (new_val < 1) new_val = 1;
        if (new_val > 10) new_val = 10;
        return new_val;
    }
    return value;
}

ChickenGenes create_genes(ChickenGenes* parent) {
    ChickenGenes genes;
    
    if (parent == 0) {
        // Random genes for first generation
        genes.hunger_rate = rand_range(3, 7);
        genes.aggression = rand_range(3, 7);
        genes.metabolism = rand_range(3, 7);
        genes.fertility = rand_range(3, 7);
        genes.food_preference = simple_rand() % 3;  // Random preference: 0=SEEDS, 1=CORN, 2=WORMS
        
        // Random initial color (various shades of brown/yellow/white)
        u8 color_choice = simple_rand() % 5;
        switch(color_choice) {
            case 0: genes.color = RGB(31, 31, 31); break; // White
            case 1: genes.color = RGB(31, 25, 15); break; // Light brown
            case 2: genes.color = RGB(25, 15, 5); break;  // Brown
            case 3: genes.color = RGB(31, 31, 10); break; // Yellow
            case 4: genes.color = RGB(20, 10, 5); break;  // Dark brown
        }
    } else {
        // Inherit with mutation
        genes.hunger_rate = mutate_gene(parent->hunger_rate);
        genes.aggression = mutate_gene(parent->aggression);
        genes.metabolism = mutate_gene(parent->metabolism);
        genes.fertility = mutate_gene(parent->fertility);
        
        // Inherit food preference with occasional mutation
        if (simple_rand() % 5 == 0) {
            genes.food_preference = simple_rand() % 3;  // 20% chance to get different preference
        } else {
            genes.food_preference = parent->food_preference;  // Inherit parent's preference
        }
        
        // Inherit color with slight mutation
        u16 r = (parent->color & 0x1F);
        u16 g = ((parent->color >> 5) & 0x1F);
        u16 b = ((parent->color >> 10) & 0x1F);
        
        // Mutate each color component slightly
        if (simple_rand() % 3 == 0) {
            s8 change = (simple_rand() % 5) - 2; // -2 to +2
            r += change;
            if (r > 31) r = 31;
            if (r < 10) r = 10; // Keep it visible
        }
        if (simple_rand() % 3 == 0) {
            s8 change = (simple_rand() % 5) - 2;
            g += change;
            if (g > 31) g = 31;
            if (g < 5) g = 5;
        }
        if (simple_rand() % 3 == 0) {
            s8 change = (simple_rand() % 5) - 2;
            b += change;
            if (b > 31) b = 31;
            if (b < 3) b = 3;
        }
        
        genes.color = RGB(r, g, b);
    }
    
    return genes;
}

void init_game() {
    // Clear everything
    for (int i = 0; i < MAX_CHICKENS; i++) {
        chickens[i].active = 0;
        prev_chicken_active[i] = 0;
        chickens[i].hunger_tick_accum = 0;
        chickens[i].satiation_tick_accum = 0;
    }
    for (int i = 0; i < MAX_FOOD; i++) {
        foods[i].active = 0;
    }
    for (int i = 0; i < MAX_EGGS; i++) {
        eggs[i].active = 0;
    }
    for (int i = 0; i < MAX_CORPSES; i++) {
        corpses[i].active = 0;
    }
    
    chicken_count = 0;
    food_count = 0;
    egg_count = 0;
    corpse_count = 0;
    frames = 0;
    
    // Spawn 2 initial chickens at different positions
    spawn_chicken(50, 60, 0);
    spawn_chicken(180, 100, 0);
    
    // Place some initial food for testing
    place_food(120, 70, FOOD_CORN);
    place_food(80, 90, FOOD_SEEDS);
    place_food(150, 80, FOOD_WORMS);
    
    // Draw initial screen
    fill_screen(RGB(15, 25, 15));
}

void spawn_chicken(s16 x, s16 y, ChickenGenes* parent_genes) {
    if (chicken_count >= MAX_CHICKENS) return;
    
    // Find empty slot
    for (int i = 0; i < MAX_CHICKENS; i++) {
        if (!chickens[i].active) {
            chickens[i].x = x;
            chickens[i].y = y;
            chickens[i].prev_x = x;
            chickens[i].prev_y = y;
            // Give each chicken a clear starting direction
            // First chicken moves right and up, second moves left and down
            if (chicken_count == 0) {
                chickens[i].dx = 1;
                chickens[i].dy = -1;
            } else if (chicken_count == 1) {
                chickens[i].dx = -1;
                chickens[i].dy = 1;
            } else {
                // For offspring, give random direction
                chickens[i].dx = ((simple_rand() % 3) - 1);
                chickens[i].dy = ((simple_rand() % 3) - 1);
                // Make sure not both zero
                if (chickens[i].dx == 0 && chickens[i].dy == 0) {
                    chickens[i].dx = 1;
                }
            }
            chickens[i].hunger = 350;  // Start hungrier so they seek food
            chickens[i].satiation = 200;
            chickens[i].age = i * 17;  // Different starting age for different behavior
            chickens[i].active = 1;
            chickens[i].frame = 0;
            chickens[i].anim_counter = 0;
            chickens[i].can_lay_egg = 0;
            chickens[i].egg_cooldown = 0;
            chickens[i].has_target = 0;
            chickens[i].just_got_hungry = 0;
            chickens[i].happiness_timer = 0;
            chickens[i].genes = create_genes(parent_genes);
            chickens[i].hunger_tick_accum = 0;
            chickens[i].satiation_tick_accum = 0;
            chicken_count++;
            
            if (parent_genes != 0) {
                generation_count++;
            }
            break;
        }
    }
}

void update_chicken(Chicken* c) {
    if (!c->active) return;
    
    // Time delta for this update (scaled)
    int dt = game_speed;

    // Age and animation advance with dt
    c->age += dt;
    c->anim_counter += dt;
    while (c->anim_counter > 15) {
        c->anim_counter -= 16;
        c->frame = (c->frame + 1) % 2;
    }
    
    // Update happiness timer
    if (c->happiness_timer > 0) {
        if (c->happiness_timer > dt) c->happiness_timer -= dt; else c->happiness_timer = 0;
    }
    
    // Decrease hunger based on hunger_rate gene (scaled using accumulator)
    {
        u16 period = (u16)(12 - c->genes.hunger_rate / 2);
        c->hunger_tick_accum += dt;
        while (c->hunger_tick_accum >= period) {
            if (c->hunger > 0) c->hunger--;
            c->hunger_tick_accum -= period;
            // Early out if already empty
            if (c->hunger == 0) break;
        }
    }
    
    // Decrease satiation (scaled using accumulator)
    if (c->satiation > 0) {
        c->satiation_tick_accum += dt;
        while (c->satiation_tick_accum >= 30 && c->satiation > 0) {
            c->satiation--;
            c->satiation_tick_accum -= 30;
        }
    }
    
    // Check if chicken dies from hunger
    if (c->hunger == 0) {
        create_corpse(c->x, c->y);
        c->active = 0;
        chicken_count--;
        return;
    }
    
    // Check if chicken dies from old age (90 seconds = 5400 frames at 60fps)
    if (c->age > 5400) {
        create_corpse(c->x, c->y);
        c->active = 0;
        chicken_count--;
        return;
    }
    
    // Check if chicken can lay egg (well fed AND not hungry AND adult age)
    // Baby: 0-1800 frames, Adult: 1800-3600, Old: 3600-5400
    if (c->satiation > 700 && c->hunger > 400 && c->egg_cooldown == 0 
        && c->age >= 1800 && c->age < 3600) {
        c->can_lay_egg = 1;
    }
    
    if (c->egg_cooldown > 0) {
        if (c->egg_cooldown > dt) c->egg_cooldown -= dt; else c->egg_cooldown = 0;
    }
    
    // Lay egg randomly when well-fed (higher chance than before)
    if (c->can_lay_egg && (simple_rand() % (50 + (10 - c->genes.fertility) * 5)) < 2) {
        chicken_lay_egg(c);
    }
    
    // AI: Find food when hungry
    if (c->hunger < 400) {
        // Play cluck sound when first becoming hungry
        if (!c->just_got_hungry) {
            play_cluck_sound();
            c->just_got_hungry = 1;
        }
        
        // Look for nearest food every frame when hungry
        s16 nearest_dist = 10000;
        s16 nearest_x = -1, nearest_y = -1;
        
        for (int i = 0; i < MAX_FOOD; i++) {
            if (foods[i].active) {
                s16 dx = foods[i].x - c->x;
                s16 dy = foods[i].y - c->y;
                s16 dist = dx * dx + dy * dy;
                
                if (dist < nearest_dist) {
                    nearest_dist = dist;
                    nearest_x = foods[i].x;
                    nearest_y = foods[i].y;
                }
            }
        }
        
        if (nearest_x >= 0) {
            c->target_food_x = nearest_x;
            c->target_food_y = nearest_y;
            c->has_target = 1;
        } else {
            c->has_target = 0;  // No food available
        }
    } else {
        c->has_target = 0;  // Not hungry, just wander
        c->just_got_hungry = 0;  // Reset flag when well-fed
    }
    
    // Move towards food if has target
    if (c->has_target) {
        s16 dx = c->target_food_x - c->x;
        s16 dy = c->target_food_y - c->y;
        
        // Check if reached target
        if (dx * dx + dy * dy < 25) {
            c->has_target = 0;
        } else {
            // Move towards target - always speed 1 (aggression affects other behaviors)
            if (dx > 0) c->dx = 1;
            else if (dx < 0) c->dx = -1;
            else c->dx = 0;
            
            if (dy > 0) c->dy = 1;
            else if (dy < 0) c->dy = -1;
            else c->dy = 0;
        }
    } else {
        // Wander randomly - use chicken's age for unique timing
        // This runs when: not seeking food, OR seeking but no food available
        if ((frames + c->age) % 30 == 0) {  // More frequent - every 0.5 seconds
            c->dx = (simple_rand() % 3) - 1;
            c->dy = (simple_rand() % 3) - 1;
            // Don't let both be zero
            if (c->dx == 0 && c->dy == 0) {
                c->dx = (simple_rand() % 2) ? 1 : -1;
            }
        }
    }
    
    // Limit speed to prevent flying off screen
    if (c->dx > 1) c->dx = 1;
    if (c->dx < -1) c->dx = -1;
    if (c->dy > 1) c->dy = 1;
    if (c->dy < -1) c->dy = -1;
    
    // Move chicken
    c->x += c->dx;
    c->y += c->dy;
    
    // Bounds checking - bounce off walls
    if (c->x < 10) {
        c->x = 10;
        c->dx = -c->dx;  // Reverse direction
        if (c->dx == 0) c->dx = 1;  // Make sure it moves
    }
    if (c->x > SCREEN_WIDTH - 25) {
        c->x = SCREEN_WIDTH - 25;
        c->dx = -c->dx;  // Reverse direction
        if (c->dx == 0) c->dx = -1;  // Make sure it moves
    }
    if (c->y < 20) {  // Keep chickens below UI (8px UI + 12px safety = 20)
        c->y = 20;
        c->dy = -c->dy;  // Reverse direction
        if (c->dy == 0) c->dy = 1;  // Make sure it moves
    }
    if (c->y > SCREEN_HEIGHT - 25) {
        c->y = SCREEN_HEIGHT - 25;
        c->dy = -c->dy;  // Reverse direction
        if (c->dy == 0) c->dy = -1;  // Make sure it moves
    }
    
    // Check if chicken can eat food
    for (int i = 0; i < MAX_FOOD; i++) {
        if (foods[i].active) {
            s16 dx = foods[i].x - c->x;
            s16 dy = foods[i].y - c->y;
            if (dx * dx + dy * dy < 64) { // Close enough to eat
                chicken_eat_food(c, &foods[i]);
            }
        }
    }
}

void chicken_eat_food(Chicken* c, Food* f) {
    if (!f->active) return;
    
    // Check if this is the chicken's favorite food
    u8 is_favorite = (f->type == c->genes.food_preference);
    
    if (is_favorite) {
        // Extra happy! Play special sound and show heart
        play_happy_sound();
        c->happiness_timer = 90;  // Show heart for 1.5 seconds
        
        // Bonus nutrition for favorite food (20% more effective)
        u16 nutrition_value = (f->nutrition * c->genes.metabolism / 5) * 6 / 5;
        c->hunger += nutrition_value;
        if (c->hunger > 1000) c->hunger = 1000;
        
        c->satiation += nutrition_value;
        if (c->satiation > 1000) c->satiation = 1000;
    } else {
        // Normal eating
        play_eat_sound();
        
        // Normal nutrition
        u16 nutrition_value = f->nutrition * c->genes.metabolism / 5;
        c->hunger += nutrition_value;
        if (c->hunger > 1000) c->hunger = 1000;
        
        c->satiation += nutrition_value;
        if (c->satiation > 1000) c->satiation = 1000;
    }
    
    // Remove food
    f->active = 0;
    food_count--;
    
    // Clear target
    c->has_target = 0;
}

void chicken_lay_egg(Chicken* c) {
    if (egg_count >= MAX_EGGS) return;
    
    // Find empty egg slot
    for (int i = 0; i < MAX_EGGS; i++) {
        if (!eggs[i].active) {
            eggs[i].x = c->x;
            eggs[i].y = c->y + 10;
            eggs[i].hatch_timer = 180; // 3 seconds at 60fps
            eggs[i].active = 1;
            eggs[i].parent_genes = c->genes;
            egg_count++;
            
            c->can_lay_egg = 0;
            c->egg_cooldown = 300; // 5 second cooldown
            c->satiation -= 300;
            break;
        }
    }
}

void place_food(s16 x, s16 y, FoodType type) {
    if (food_count >= MAX_FOOD) return;
    
    // Find empty slot
    for (int i = 0; i < MAX_FOOD; i++) {
        if (!foods[i].active) {
            foods[i].x = x;
            foods[i].y = y;
            foods[i].type = type;
            
            // Different nutrition values
            switch(type) {
                case FOOD_SEEDS: foods[i].nutrition = 100; break;
                case FOOD_CORN: foods[i].nutrition = 200; break;
                case FOOD_WORMS: foods[i].nutrition = 150; break;
                default: foods[i].nutrition = 100; break;
            }
            
            foods[i].active = 1;
            food_count++;
            break;
        }
    }
}

void update_eggs() {
    for (int i = 0; i < MAX_EGGS; i++) {
        if (eggs[i].active) {
            if (eggs[i].hatch_timer > game_speed) eggs[i].hatch_timer -= game_speed; else eggs[i].hatch_timer = 0;
            
            if (eggs[i].hatch_timer == 0) {
                // Hatch egg with sound
                play_hatch_sound();
                spawn_chicken(eggs[i].x, eggs[i].y, &eggs[i].parent_genes);
                eggs[i].active = 0;
                egg_count--;
            }
        }
    }
}

void create_corpse(s16 x, s16 y) {
    if (corpse_count >= MAX_CORPSES) return;
    
    // Find empty slot
    for (int i = 0; i < MAX_CORPSES; i++) {
        if (!corpses[i].active) {
            corpses[i].x = x;
            corpses[i].y = y;
            corpses[i].timer = 180; // 3 seconds at 60fps
            corpses[i].active = 1;
            corpse_count++;
            break;
        }
    }
}

void update_corpses() {
    for (int i = 0; i < MAX_CORPSES; i++) {
        if (corpses[i].active) {
            if (corpses[i].timer > game_speed) corpses[i].timer -= game_speed; else corpses[i].timer = 0;
            
            if (corpses[i].timer == 0) {
                corpses[i].active = 0;
                corpse_count--;
            }
        }
    }
}

void draw_corpse(Corpse* c) {
    if (!c->active) return;
    // Draw simple crossed bones in white with slight gray shading
    u16 bone_white = COLOR_WHITE;
    u16 bone_shadow = COLOR_GRAY;
    int x = c->x, y = c->y;
    // First bone: bottom-left to top-right
    draw_rect(x,     y + 3, 7, 2, bone_white);
    draw_rect(x + 6, y + 2, 2, 4, bone_white);
    draw_rect(x + 8, y + 1, 2, 6, bone_white);
    draw_rect(x +10, y,     2, 8, bone_white);
    // Round-ish ends
    draw_rect(x -1,  y + 2, 2, 4, bone_white);
    draw_rect(x +12, y -1,  3, 3, bone_white);
    draw_rect(x +12, y +6,  3, 3, bone_white);
    // Light shadow
    draw_rect(x +10, y +7,  2, 1, bone_shadow);

    // Second bone: top-left to bottom-right
    draw_rect(x,     y + 4, 2, 2, bone_white);
    draw_rect(x + 2, y + 3, 2, 4, bone_white);
    draw_rect(x + 4, y + 2, 2, 6, bone_white);
    draw_rect(x + 6, y + 1, 2, 8, bone_white);
    draw_rect(x + 8, y,     2, 10, bone_white);
    // Ends
    draw_rect(x -1,  y + 3, 2, 4, bone_white);
    draw_rect(x + 9, y -1,  3, 3, bone_white);
    draw_rect(x + 9, y +8,  3, 3, bone_white);
}

void update_game() {
    frames++;
    
    // Update cursor
    if (key_is_down(KEY_UP)) cursor_y--;
    if (key_is_down(KEY_DOWN)) cursor_y++;
    if (key_is_down(KEY_LEFT)) cursor_x--;
    if (key_is_down(KEY_RIGHT)) cursor_x++;
    
    // Clamp cursor (keep it below UI and within screen bounds)
    if (cursor_x < 10) cursor_x = 10;
    if (cursor_x > SCREEN_WIDTH - 10) cursor_x = SCREEN_WIDTH - 10;
    if (cursor_y < 20) cursor_y = 20;  // Keep below UI
    if (cursor_y > SCREEN_HEIGHT - 10) cursor_y = SCREEN_HEIGHT - 10;
    
    // Change food type
    if (key_just_pressed(KEY_R)) {
        selected_food = (selected_food + 1) % FOOD_TYPE_COUNT;
    }
    if (key_just_pressed(KEY_L)) {
        selected_food = (selected_food - 1 + FOOD_TYPE_COUNT) % FOOD_TYPE_COUNT;
    }
    
    // Place food
    if (key_just_pressed(KEY_A)) {
        place_food(cursor_x, cursor_y, selected_food);
    }
    
    // Update chickens (bucketed by index to reduce per-frame work)
    int bucket = frames % CHICKEN_UPDATE_STRIDE;
    for (int i = 0; i < MAX_CHICKENS; i++) {
        if (!chickens[i].active) continue;
        if ((i % CHICKEN_UPDATE_STRIDE) == bucket) {
            update_chicken(&chickens[i]);
        }
    }
    
    // Update eggs
    update_eggs();
    
    // Update corpses
    update_corpses();
}

void draw_chicken(Chicken* c) {
    if (!c->active) return;
    
    // Use the chicken's genetic color
    u16 body_color = c->genes.color;
    
    // Determine if chicken is facing left (flip sprite)
    u8 facing_left = (c->dx < 0);
    
    // Determine age stage: baby (0-30s), adult (30-60s), old (60-90s)
    // Baby: 0-1800 frames, Adult: 1800-3600, Old: 3600-5400
    if (c->age < 1800) {
        // Baby chicken - smaller and cuter
        if (facing_left) {
            // Flipped baby (facing left)
            draw_rect(c->x + 4, c->y + 2, 6, 5, body_color);  // Small body
            draw_rect(c->x + 2, c->y, 4, 4, body_color);      // Small head
            draw_rect(c->x, c->y + 1, 2, 2, COLOR_ORANGE);    // Beak (left)
            draw_pixel(c->x + 3, c->y + 1, COLOR_BLACK);      // Eye
            draw_rect(c->x + 3, c->y - 2, 2, 2, COLOR_RED);   // Tiny comb
            
            // Tiny legs (flipped)
            if (c->frame % 2 == 0) {
                draw_rect(c->x + 5, c->y + 7, 1, 2, COLOR_ORANGE);
                draw_rect(c->x + 8, c->y + 7, 1, 2, COLOR_ORANGE);
            } else {
                draw_rect(c->x + 6, c->y + 7, 1, 2, COLOR_ORANGE);
                draw_rect(c->x + 7, c->y + 7, 1, 2, COLOR_ORANGE);
            }
        } else {
            // Normal baby (facing right)
            draw_rect(c->x, c->y + 2, 6, 5, body_color);      // Small body
            draw_rect(c->x + 4, c->y, 4, 4, body_color);      // Small head
            draw_rect(c->x + 8, c->y + 1, 2, 2, COLOR_ORANGE); // Beak
            draw_pixel(c->x + 6, c->y + 1, COLOR_BLACK);      // Eye
            draw_rect(c->x + 5, c->y - 2, 2, 2, COLOR_RED);   // Tiny comb
            
            // Tiny legs
            if (c->frame % 2 == 0) {
                draw_rect(c->x + 1, c->y + 7, 1, 2, COLOR_ORANGE);
                draw_rect(c->x + 4, c->y + 7, 1, 2, COLOR_ORANGE);
            } else {
                draw_rect(c->x + 2, c->y + 7, 1, 2, COLOR_ORANGE);
                draw_rect(c->x + 3, c->y + 7, 1, 2, COLOR_ORANGE);
            }
        }
    } else if (c->age < 3600) {
        // Adult chicken - normal size
        if (facing_left) {
            // Flipped adult (facing left)
            draw_rect(c->x + 3, c->y, 10, 8, body_color);     // Body
            draw_rect(c->x, c->y - 3, 6, 6, body_color);      // Head (left)
            draw_rect(c->x - 2, c->y - 1, 2, 2, COLOR_ORANGE); // Beak (left)
            draw_pixel(c->x + 2, c->y - 1, COLOR_BLACK);      // Eye
            draw_rect(c->x + 3, c->y - 5, 2, 2, COLOR_RED);   // Comb
            
            // Legs (flipped)
            if (c->frame % 2 == 0) {
                draw_rect(c->x + 5, c->y + 8, 2, 3, COLOR_ORANGE);
                draw_rect(c->x + 9, c->y + 8, 2, 3, COLOR_ORANGE);
            } else {
                draw_rect(c->x + 6, c->y + 8, 2, 3, COLOR_ORANGE);
                draw_rect(c->x + 8, c->y + 8, 2, 3, COLOR_ORANGE);
            }
        } else {
            // Normal adult (facing right)
            draw_rect(c->x, c->y, 10, 8, body_color);
            draw_rect(c->x + 7, c->y - 3, 6, 6, body_color);
            draw_rect(c->x + 13, c->y - 1, 2, 2, COLOR_ORANGE);
            draw_pixel(c->x + 11, c->y - 1, COLOR_BLACK);
            draw_rect(c->x + 8, c->y - 5, 2, 2, COLOR_RED);
            
            // Legs
            if (c->frame % 2 == 0) {
                draw_rect(c->x + 2, c->y + 8, 2, 3, COLOR_ORANGE);
                draw_rect(c->x + 6, c->y + 8, 2, 3, COLOR_ORANGE);
            } else {
                draw_rect(c->x + 3, c->y + 8, 2, 3, COLOR_ORANGE);
                draw_rect(c->x + 5, c->y + 8, 2, 3, COLOR_ORANGE);
            }
        }
    } else {
        // Old chicken - slightly hunched/darker
        u16 old_color = RGB((body_color & 0x1F) * 3/4, 
                            ((body_color >> 5) & 0x1F) * 3/4,
                            ((body_color >> 10) & 0x1F) * 3/4);
        
        if (facing_left) {
            // Flipped old (facing left)
            draw_rect(c->x + 3, c->y + 1, 10, 8, old_color);  // Body lower
            draw_rect(c->x, c->y - 2, 6, 6, old_color);       // Head lower (left)
            draw_rect(c->x - 2, c->y, 2, 2, COLOR_ORANGE);    // Beak (left)
            draw_pixel(c->x + 2, c->y, COLOR_BLACK);          // Eye
            draw_rect(c->x + 3, c->y - 4, 2, 2, RGB(20, 0, 0)); // Darker comb
            
            // Slower leg animation for old chicken (flipped)
            if ((c->frame / 2) % 2 == 0) {
                draw_rect(c->x + 5, c->y + 9, 2, 3, COLOR_ORANGE);
                draw_rect(c->x + 9, c->y + 9, 2, 3, COLOR_ORANGE);
            } else {
                draw_rect(c->x + 6, c->y + 9, 2, 3, COLOR_ORANGE);
                draw_rect(c->x + 8, c->y + 9, 2, 3, COLOR_ORANGE);
            }
        } else {
            // Normal old (facing right)
            draw_rect(c->x, c->y + 1, 10, 8, old_color);  // Body lower
            draw_rect(c->x + 7, c->y - 2, 6, 6, old_color); // Head lower
            draw_rect(c->x + 13, c->y, 2, 2, COLOR_ORANGE);
            draw_pixel(c->x + 11, c->y, COLOR_BLACK);
            draw_rect(c->x + 8, c->y - 4, 2, 2, RGB(20, 0, 0)); // Darker comb
            
            // Slower leg animation for old chicken
            if ((c->frame / 2) % 2 == 0) {
                draw_rect(c->x + 2, c->y + 9, 2, 3, COLOR_ORANGE);
                draw_rect(c->x + 6, c->y + 9, 2, 3, COLOR_ORANGE);
            } else {
                draw_rect(c->x + 3, c->y + 9, 2, 3, COLOR_ORANGE);
                draw_rect(c->x + 5, c->y + 9, 2, 3, COLOR_ORANGE);
            }
        }
    }
    
    // Draw hunger bar above chicken
    s16 hunger_bar_y = c->y - 12;
    if (hunger_bar_y >= 16) {  // Only draw if there's room and below UI (15px)
        u8 hunger_width = c->hunger / 50;
        if (hunger_width > 20) hunger_width = 20;
        
        // Draw background bar (gray) - 2 pixels tall
        draw_rect(c->x - 5, hunger_bar_y, 20, 2, COLOR_GRAY);
        
        // Draw hunger level bar on top - same width for both pixels
        if (hunger_width > 0) {
            if (c->hunger > 300) {
                draw_rect(c->x - 5, hunger_bar_y, hunger_width, 2, COLOR_GREEN);
            } else {
                draw_rect(c->x - 5, hunger_bar_y, hunger_width, 2, COLOR_RED);
            }
        }
    }
    
    // Draw heart when happy (ate favorite food)
    if (c->happiness_timer > 0) {
        s16 heart_y = c->y - 8;
        if (heart_y >= 16) {  // Only draw if heart won't overlap UI (15px UI bar)
            // Simple heart shape above chicken
            s16 heart_x = c->x + 14;
            
            // Heart (pink/red)
            draw_rect(heart_x, heart_y + 1, 1, 2, RGB(31, 10, 15));     // Left top
            draw_rect(heart_x + 2, heart_y + 1, 1, 2, RGB(31, 10, 15)); // Right top
            draw_rect(heart_x + 1, heart_y, 1, 4, RGB(31, 10, 15));     // Middle
            draw_pixel(heart_x + 1, heart_y + 3, RGB(31, 5, 10));       // Bottom point
        }
    }
}

void draw_chicken_cleanup(s16 x, s16 y) {
    u16 bg_color = RGB(15, 25, 15);
    
    // Clean up area to cover chicken body, legs, hunger bar, and heart
    // Adult chicken is about 15 pixels wide (including beak), up to 13 pixels tall (including legs)
    // Hunger bar is at y-12, extends 20 pixels wide from x-5
    // Heart is at x+14, y-8
    // Total area: x-5 to x+20 (25 wide), y-13 to y+13 (26 tall)
    s16 cleanup_y = y - 13;
    s16 cleanup_height = 26;
    s16 cleanup_x = x - 5;
    s16 cleanup_width = 25;
    
    // If cleanup would overlap with UI (top 15 pixels), adjust it
    if (cleanup_y < 15) {
        s16 overlap = 15 - cleanup_y;
        cleanup_y = 15;
        cleanup_height -= overlap;
    }
    
    // Always do cleanup if there's area to clean
    if (cleanup_height > 0 && cleanup_width > 0) {
        draw_rect(cleanup_x, cleanup_y, cleanup_width, cleanup_height, bg_color);
    }
}

void draw_food(Food* f) {
    if (!f->active) return;
    
    u16 color;
    switch(f->type) {
        case FOOD_SEEDS:
            color = COLOR_BROWN;
            draw_rect(f->x, f->y, 3, 3, color);
            break;
        case FOOD_CORN:
            color = COLOR_YELLOW;
            draw_rect(f->x, f->y, 4, 6, color);
            draw_rect(f->x + 1, f->y - 1, 2, 2, COLOR_GREEN);
            break;
        case FOOD_WORMS:
            color = RGB(25, 10, 10);
            draw_rect(f->x, f->y, 5, 2, color);
            break;
        default: // Includes FOOD_TYPE_COUNT or any invalid value
            return;
    }
}

void draw_egg(Egg* e) {
    if (!e->active) return;
    
    // Egg shape
    u16 color = RGB(30, 28, 25);
    draw_rect(e->x + 1, e->y, 3, 2, color);
    draw_rect(e->x, e->y + 2, 5, 3, color);
    draw_rect(e->x + 1, e->y + 5, 3, 2, color);
}

// ========== REFACTORED DRAWING SYSTEM ==========

// Draw the UI bar at the top of the screen
void draw_ui() {
    // Redraw UI only when values change to reduce overdraw
    static int prev_chicken_count = -1;
    static int prev_generation_count = -1;
    static FoodType prev_selected_food = (FoodType)-1;

    if (chicken_count == prev_chicken_count &&
        generation_count == prev_generation_count &&
        selected_food == prev_selected_food) {
        return; // No changes; skip drawing UI
    }

    draw_rect(0, 0, SCREEN_WIDTH, 15, RGB(5, 10, 15));
    draw_string(5, 4, "CHICKENS:", COLOR_WHITE);
    draw_number(65, 4, chicken_count, COLOR_YELLOW);
    draw_string(95, 4, "FOOD:", COLOR_WHITE);
    const char* food_names[] = {"SEEDS", "CORN", "WORMS"};
    draw_string(135, 4, food_names[selected_food], COLOR_ORANGE);
    draw_string(190, 4, "GEN:", COLOR_WHITE);
    draw_number(220, 4, generation_count, COLOR_GREEN);

    prev_chicken_count = chicken_count;
    prev_generation_count = generation_count;
    prev_selected_food = selected_food;
}

// Draw all food items
void draw_all_food() {
    for (int i = 0; i < MAX_FOOD; i++) {
        if (foods[i].active) {
            draw_food(&foods[i]);
        }
    }
    
    for (int i = 0; i < MAX_EGGS; i++) {
        if (eggs[i].active) {
            draw_egg(&eggs[i]);
        }
    }
    
    for (int i = 0; i < MAX_CORPSES; i++) {
        if (corpses[i].active) {
            draw_corpse(&corpses[i]);
        }
    }
}

// Draw all chickens with batched cleanup to avoid mid-frame erasure
void draw_all_chickens() {
    u16 bg_color = RGB(15, 25, 15);

    // Local rect type
    typedef struct { s16 x, y, w, h; } DirtyRect;
    DirtyRect dirty[MAX_CHICKENS];
    int dirty_count = 0;

    // 1) Collect dirty rects for all chickens that moved or just died (based on previous position)
    for (int i = 0; i < MAX_CHICKENS; i++) {
        // If chicken was active last frame and now inactive, we need to erase its last drawn sprite area
        if (prev_chicken_active[i] && !chickens[i].active) {
            s16 x = chickens[i].prev_x - 5;
            s16 y = chickens[i].prev_y - 13;
            s16 w = 25;
            s16 h = 26;
            if (x < 0) { w += x; x = 0; }
            if (y < 0) { h += y; y = 0; }
            if (x + w > SCREEN_WIDTH)  w = SCREEN_WIDTH - x;
            if (y + h > SCREEN_HEIGHT) h = SCREEN_HEIGHT - y;
            if (y < 15) { s16 overlap = 15 - y; y = 15; h -= overlap; }
            if (w > 0 && h > 0 && dirty_count < MAX_CHICKENS) {
                dirty[dirty_count++] = (DirtyRect){ x, y, w, h };
            }
            continue;
        }

        if (!chickens[i].active) continue;
        if (chickens[i].x == chickens[i].prev_x && chickens[i].y == chickens[i].prev_y) continue;

        s16 x = chickens[i].prev_x - 5;
        s16 y = chickens[i].prev_y - 13;
        s16 w = 25;
        s16 h = 26;

        // Clip to screen bounds; avoid touching UI (top 15px) since UI draws only on change
        if (x < 0) { w += x; x = 0; }
        if (y < 0) { h += y; y = 0; }
        if (x + w > SCREEN_WIDTH)  w = SCREEN_WIDTH - x;
        if (y + h > SCREEN_HEIGHT) h = SCREEN_HEIGHT - y;
        if (y < 15) {
            s16 overlap = 15 - y;
            y = 15;
            h -= overlap;
        }

        if (w > 0 && h > 0 && dirty_count < MAX_CHICKENS) {
            dirty[dirty_count++] = (DirtyRect){ x, y, w, h };
        }
    }

    // Aggressively merge overlapping/adjacent dirty rects until stable
    if (dirty_count > 1) {
        int merged;
        do {
            merged = 0;
            for (int a = 0; a < dirty_count; a++) {
                for (int b = a + 1; b < dirty_count; ) {
                    s16 ax = dirty[a].x, ay = dirty[a].y, aw = dirty[a].w, ah = dirty[a].h;
                    s16 bx = dirty[b].x, by = dirty[b].y, bw = dirty[b].w, bh = dirty[b].h;
                    // Check intersection or adjacency (1px gap) to merge
                    int overlap = (ax <= bx + bw + 1) && (bx <= ax + aw + 1) &&
                                  (ay <= by + bh + 1) && (by <= ay + ah + 1);
                    if (overlap) {
                        s16 nx = (ax < bx) ? ax : bx;
                        s16 ny = (ay < by) ? ay : by;
                        s16 nx2 = ((ax + aw) > (bx + bw)) ? (ax + aw) : (bx + bw);
                        s16 ny2 = ((ay + ah) > (by + bh)) ? (ay + ah) : (by + bh);
                        dirty[a].x = nx; dirty[a].y = ny; dirty[a].w = nx2 - nx; dirty[a].h = ny2 - ny;
                        // remove b by shifting tail
                        for (int k = b; k < dirty_count - 1; k++) dirty[k] = dirty[k + 1];
                        dirty_count--;
                        merged = 1;
                        // don't advance b; re-check current b index (now next element)
                    } else {
                        b++;
                    }
                }
            }
        } while (merged);
    }

    // 2) Erase all dirty rects first
    for (int k = 0; k < dirty_count; k++) {
        draw_rect(dirty[k].x, dirty[k].y, dirty[k].w, dirty[k].h, bg_color);
    }

    // 3) Redraw static elements covered by any dirty rect (food/eggs/corpses)
    for (int k = 0; k < dirty_count; k++) {
        s16 rx = dirty[k].x, ry = dirty[k].y, rw = dirty[k].w, rh = dirty[k].h;
        // Food
        for (int j = 0; j < MAX_FOOD; j++) {
            if (!foods[j].active) continue;
            if (foods[j].x >= rx - 5 && foods[j].x <= rx + rw + 5 &&
                foods[j].y >= ry - 5 && foods[j].y <= ry + rh + 5) {
                draw_food(&foods[j]);
            }
        }
        // Eggs
        for (int j = 0; j < MAX_EGGS; j++) {
            if (!eggs[j].active) continue;
            if (eggs[j].x >= rx - 5 && eggs[j].x <= rx + rw + 5 &&
                eggs[j].y >= ry - 5 && eggs[j].y <= ry + rh + 5) {
                draw_egg(&eggs[j]);
            }
        }
        // Corpses
        for (int j = 0; j < MAX_CORPSES; j++) {
            if (!corpses[j].active) continue;
            if (corpses[j].x >= rx - 5 && corpses[j].x <= rx + rw + 5 &&
                corpses[j].y >= ry - 5 && corpses[j].y <= ry + rh + 5) {
                draw_corpse(&corpses[j]);
            }
        }
    }

    // 4) Draw all corpses so recent deaths appear immediately (below chickens)
    for (int i = 0; i < MAX_CORPSES; i++) {
        if (corpses[i].active) draw_corpse(&corpses[i]);
    }

    // 5) Build y-sorted draw lists: moved/new first, then stationary but affected by dirties
    typedef struct { int idx; s16 y; } DrawItem;
    DrawItem moved_list[MAX_CHICKENS];
    DrawItem stat_list[MAX_CHICKENS];
    int moved_n = 0, stat_n = 0;

    for (int i = 0; i < MAX_CHICKENS; i++) {
        if (!chickens[i].active) continue;
        int is_new = (!prev_chicken_active[i] && chickens[i].active);
        int moved = (chickens[i].x != chickens[i].prev_x || chickens[i].y != chickens[i].prev_y) || is_new;
        if (moved) {
            moved_list[moved_n++] = (DrawItem){ i, chickens[i].y };
        } else if (dirty_count > 0) {
            // stationary but check intersection with any dirty rect
            s16 cx = chickens[i].x - 5;
            s16 cy = chickens[i].y - 13;
            s16 cw = 25;
            s16 ch = 26;
            int intersects = 0;
            for (int k = 0; k < dirty_count; k++) {
                s16 rx = dirty[k].x, ry = dirty[k].y, rw = dirty[k].w, rh = dirty[k].h;
                if (cx < rx + rw && cx + cw > rx && cy < ry + rh && cy + ch > ry) { intersects = 1; break; }
            }
            if (intersects) stat_list[stat_n++] = (DrawItem){ i, chickens[i].y };
        }
    }

    // Simple insertion sort by y ascending (top to bottom)
    for (int a = 1; a < moved_n; a++) {
        DrawItem key = moved_list[a]; int b = a - 1;
        while (b >= 0 && moved_list[b].y > key.y) { moved_list[b+1] = moved_list[b]; b--; }
        moved_list[b+1] = key;
    }
    for (int a = 1; a < stat_n; a++) {
        DrawItem key = stat_list[a]; int b = a - 1;
        while (b >= 0 && stat_list[b].y > key.y) { stat_list[b+1] = stat_list[b]; b--; }
        stat_list[b+1] = key;
    }

    // Draw moved/new first, then stationary affected, both y-sorted
    for (int m = 0; m < moved_n; m++) {
        draw_chicken(&chickens[moved_list[m].idx]);
    }
    for (int s = 0; s < stat_n; s++) {
        draw_chicken(&chickens[stat_list[s].idx]);
    }

    // 6) Update previous positions and active flags AFTER drawing for next frame's cleanup detection
    for (int i = 0; i < MAX_CHICKENS; i++) {
        if (chickens[i].active) {
            chickens[i].prev_x = chickens[i].x;
            chickens[i].prev_y = chickens[i].y;
        }
        prev_chicken_active[i] = chickens[i].active;
    }
}

// Draw the cursor (should be drawn on top of everything except UI)
void draw_cursor() {
    u16 bg_color = RGB(15, 25, 15);
    static s16 prev_cursor_x = SCREEN_WIDTH / 2;
    static s16 prev_cursor_y = SCREEN_HEIGHT / 2;
    
    // Clean up old cursor position if it moved
    if (cursor_x != prev_cursor_x || cursor_y != prev_cursor_y) {
        // Only erase old cursor if it was in game area (not overlapping UI)
        if (prev_cursor_y >= 18) {  // cursor is 5px tall, center at y, so y-2 should be > 15
            draw_rect(prev_cursor_x - 3, prev_cursor_y - 3, 7, 7, bg_color);
            
            // Redraw anything under old cursor position
            for (int i = 0; i < MAX_FOOD; i++) {
                if (foods[i].active && foods[i].x >= prev_cursor_x - 8 && foods[i].x <= prev_cursor_x + 8 &&
                    foods[i].y >= prev_cursor_y - 8 && foods[i].y <= prev_cursor_y + 8) {
                    draw_food(&foods[i]);
                }
            }
            for (int i = 0; i < MAX_EGGS; i++) {
                if (eggs[i].active && eggs[i].x >= prev_cursor_x - 8 && eggs[i].x <= prev_cursor_x + 8 &&
                    eggs[i].y >= prev_cursor_y - 8 && eggs[i].y <= prev_cursor_y + 8) {
                    draw_egg(&eggs[i]);
                }
            }
            for (int i = 0; i < MAX_CORPSES; i++) {
                if (corpses[i].active && corpses[i].x >= prev_cursor_x - 8 && corpses[i].x <= prev_cursor_x + 8 &&
                    corpses[i].y >= prev_cursor_y - 8 && corpses[i].y <= prev_cursor_y + 8) {
                    draw_corpse(&corpses[i]);
                }
            }
            for (int i = 0; i < MAX_CHICKENS; i++) {
                if (chickens[i].active && chickens[i].x >= prev_cursor_x - 20 && chickens[i].x <= prev_cursor_x + 20 &&
                    chickens[i].y >= prev_cursor_y - 20 && chickens[i].y <= prev_cursor_y + 20) {
                    draw_chicken(&chickens[i]);
                }
            }
        }
        
        // Update previous cursor position
        prev_cursor_x = cursor_x;
        prev_cursor_y = cursor_y;
    }
    
    // ALWAYS draw cursor at current position (even if it didn't move)
    // This ensures cursor stays on top even if chickens moved underneath it
    draw_rect(cursor_x - 2, cursor_y - 2, 5, 1, COLOR_WHITE);
    draw_rect(cursor_x - 2, cursor_y + 2, 5, 1, COLOR_WHITE);
    draw_rect(cursor_x - 2, cursor_y - 2, 1, 5, COLOR_WHITE);
    draw_rect(cursor_x + 2, cursor_y - 2, 1, 5, COLOR_WHITE);
}

void draw_game() {
    static u8 first_draw = 1;
    
    // On first draw, clear entire screen
    if (first_draw) {
        fill_screen(RGB(15, 25, 15));
        first_draw = 0;
        
        // Draw all static elements on first frame
        draw_all_food();
        draw_all_chickens();
        draw_cursor();
        draw_ui();
        return;
    }
    
    // Drawing order (priority from bottom to top):
    // 1. Food/Eggs/Corpses (bottom layer - static, redrawn only when covered by cleanup)
    // 2. Chickens (moving layer - selective cleanup and redraw)
    // 3. Cursor (top layer - always visible on top of chickens)
    // 4. UI (topmost layer - always visible, only redraws when data changes)
    
    // Note: Food layer is not explicitly redrawn here because it's static.
    // It only gets redrawn when chickens or cursor pass over it (in their cleanup routines)
    
    draw_all_chickens();  // Handles cleanup and selective redraw
    draw_cursor();        // Draw cursor last so it stays on top
    draw_ui();            // Always redraw UI; ensures top bar is restored if any cleanup hit it
}

