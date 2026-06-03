# Heroes of Taboo — Project Rules

## Non-negotiable constraints (apply to every task)

- **C99 only.** No C11/C23 features.
- **Raylib types directly** (`Texture2D`, `Camera2D`, `Vector2`, `Color`, `Sound`, `Music`). No wrappers.
- **All textures loaded exclusively through** `Resources_LoadTexture(path)` in `engine/resources.h`. No direct `LoadTexture` calls outside the resource manager (exception: `GenImageColor`/`LoadTextureFromImage` for procedural fallback textures in the map loader).
- **All new component structs** go in `game/src/components.h` only. Never in system files.
- **Systems are stateless** — no `static` local variables in system `.c` files. All state lives in `GameWorld` or a caller-owned struct passed in.
- **Entity 0 is `ENTITY_NONE`** — never write to or read from slot 0.
- **No new dependencies.** No new build systems. Update `game/premake5.lua` include paths if new files are added.
- **Preserve all existing gameplay** — turn order, combat formulas, fog of war, exp curves, AI behaviour, key bindings, and rendering output must be identical unless a task explicitly changes them.

## Architecture

```
project/
├── engine/           # ECS core (ecs.h/.c), resource manager (resources.h/.c)
├── game/
│   ├── include/      # Shared public headers (game_balance.h, equipment_bonus.h, etc.)
│   └── src/
│       ├── main.c, game.h/.c, world.h/.c, components.h
│       ├── inventory.c, equipment_bonus.c
│       ├── systems/  # All game systems (ai, combat, input, movement, render, spawner, etc.)
│       ├── ui/       # Menu, HUD, inspector, inventory UI, combat log, etc.
│       ├── data/     # Monster/loot static data tables
│       └── map/      # Procedural generation, TMX loader, map helpers
└── tests/            # test_runner.c (C native, no framework)
```

**ECS pattern**: Structure-of-Arrays with bitmask component ownership. `EntityId` = `uint32_t`. `World` struct holds parallel arrays (`positions[MAX_ENTITIES]`, `stats[MAX_ENTITIES]`, etc.) and a `ComponentMask` per entity.

```c
for (Entity e = 1; e < (Entity)gw->ecs.count; e++) {
    if (!World_HasComponents(&gw->ecs, e, REQUIRED_MASK)) continue;
    // ... access components via world struct pools
}
```

## Build & test

```sh
cd game
make config=debug_x64       # compile
make config=debug_x64 test  # run unit tests
```

All 28 unit tests must pass before committing. Keep test runtime under 2 seconds.
