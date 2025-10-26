#ifndef GAME_H
#define GAME_H

#include "gba.h"

// Game constants
#define MAX_CHICKENS 8
#define MAX_FOOD 20
#define MAX_EGGS 15
#define MAX_CORPSES 10

// Food types
typedef enum {
    FOOD_SEEDS = 0,
    FOOD_CORN = 1,
    FOOD_WORMS = 2,
    FOOD_TYPE_COUNT = 3
} FoodType;

// Chicken genetics
typedef struct {
    u8 hunger_rate;      // How fast chicken gets hungry (1-10)
    u8 aggression;       // How fast chicken moves (1-10)
    u8 metabolism;       // How efficiently chicken processes food (1-10)
    u8 fertility;        // How likely to lay eggs (1-10)
    u8 food_preference;  // Preferred food type: 0=SEEDS, 1=CORN, 2=WORMS
    u16 color;           // RGB555 color of the chicken
} ChickenGenes;

// Chicken structure
typedef struct {
    s16 x, y;
    s16 prev_x, prev_y;
    s8 dx, dy;  // MUST be signed char (s8) for proper movement in all directions
    u16 hunger;
    u16 satiation;
    u16 age;
    u8 active;
    u8 frame;
    u8 anim_counter;
    u8 can_lay_egg;
    u16 egg_cooldown;
    s16 target_food_x, target_food_y;
    u8 has_target;
    u8 just_got_hungry;
    u8 happiness_timer;  // Timer for showing heart when eating favorite food
    ChickenGenes genes;
    // Accumulators for time-scaled periodic effects
    u16 hunger_tick_accum;     // accumulates toward hunger decrement period
    u16 satiation_tick_accum;  // accumulates toward satiation decrement period
    // Egg-laying pre-sit state
    u8 is_sitting;      // 1 when preparing to lay egg (shows sitting sprite)
    u16 sit_timer;      // counts down (~5s) before egg appears
    // Natural movement timing
    u16 idle_timer;     // when >0, chicken rests (dx=dy=0)
    u16 move_timer;     // duration to keep current direction before reevaluating
} Chicken;

// Food structure
typedef struct {
    s16 x, y;
    u16 nutrition;
    FoodType type;
    u8 active;
} Food;

// Egg structure
typedef struct {
    s16 x, y;
    u16 hatch_timer;
    ChickenGenes parent_genes;
    u8 active;
} Egg;

// Corpse structure (for dead chickens)
typedef struct {
    s16 x, y;
    u16 timer;  // How long corpse remains visible
    u8 active;
} Corpse;

// Game functions
void init_game();
void update_game();
void draw_game();
void spawn_chicken(s16 x, s16 y, ChickenGenes* parent_genes);
void place_food(s16 x, s16 y, FoodType type);

// Adjust global game speed (1 = normal). Higher values speed up timers (testing aid).
void set_game_speed(u8 speed);

#endif // GAME_H
