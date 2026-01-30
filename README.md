# Chicken Farmer

A genetic simulation game for the Game Boy Advance. Manage a flock of chickens, feed them, and watch evolution unfold as offspring inherit and mutate their parents' traits.

## Gameplay

You oversee a flock of up to **8 chickens**. Place food on the ground and watch them eat, grow, reproduce, and eventually die. Each chicken carries a set of genetic traits that get passed down to offspring — with a chance of mutation — creating emergent variation over generations.

### Controls

| Button | Action |
|--------|--------|
| **D-Pad** | Move cursor |
| **A** | Place food |
| **L / R** | Cycle food type (Seeds, Corn, Worms) |
| **START** | Restart (when all chickens are dead) |

### Chicken Lifecycle

| Stage | Duration | Notes |
|-------|----------|-------|
| **Baby** | 0 – 30 s | Smaller appearance |
| **Adult** | 30 – 60 s | Full size, can reproduce |
| **Old** | 60 – 90 s | Hunched, darker color, slower |
| **Death** | 90 s+ | Dies of old age or starvation |

### Genetics

Every chicken has the following heritable traits:

- **Hunger Rate** (1–10) — how fast it gets hungry
- **Aggression** (1–10) — movement speed
- **Metabolism** (1–10) — food processing efficiency
- **Fertility** (1–10) — egg-laying frequency
- **Food Preference** — Seeds, Corn, or Worms
- **Color** — RGB body color

When a chicken reproduces, offspring inherit traits from the parent with:

- **30%** chance of mutation per numeric trait (±1)
- **20%** chance of food preference change
- **33%** chance of color shift per RGB channel
- **~0.4%** chance of a rare **pink color** mutation

## Building from Source

Requires [devkitARM / devkitPro](https://devkitpro.org/).

```bash
make
```

The ROM is output to `rom/chicken-farmer.gba`.

## Playing

Load `rom/chicken-farmer.gba` in any GBA emulator (mGBA, VisualBoyAdvance, etc.) or on real hardware via a flash cart.
