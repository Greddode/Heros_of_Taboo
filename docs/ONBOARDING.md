# Onboarding Guide for New Contributors

**Heroes of Taboo** — v0.0.10 | June 5, 2026

---

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Getting the Code](#getting-the-code)
3. [Building the Project](#building-the-project)
4. [Running the Game](#running-the-game)
5. [Running Tests](#running-tests)
6. [Understanding the Codebase](#understanding-the-codebase)
7. [Making Your First Change](#making-your-first-change)
8. [Coding Conventions](#coding-conventions)
9. [Common Tasks](#common-tasks)
10. [Debugging Tips](#debugging-tips)
11. [Getting Help](#getting-help)

---

## Prerequisites

### Required Software

| Tool | Purpose | Installation |
|------|---------|-------------|
| **Git** | Version control | [git-scm.com](https://git-scm.com) |
| **w64devkit** | C compiler + make (Windows) | Extract to `C:\raylib\w64devkit` |
| **Premake5** | Build system generator | [premake.github.io](https://premake.github.io) |
| **Text editor** | Code editing | VS Code with C/C++ extension recommended |

### Windows Setup (Recommended)

1. Install [w64devkit](https://github.com/skeeto/w64devkit/releases) — extract to `C:\raylib\w64devkit`
2. Verify: `C:\raylib\w64devkit\bin\gcc.exe --version`
3. Install [Premake5](https://premake.github.io/download) — add to PATH
4. Clone the repository

### macOS/Linux Setup

1. Install GCC or Clang: `brew install gcc` or `apt-get install build-essential`
2. Install Premake5: `brew install premake5` or `apt-get install premake5`
3. Install Raylib dev libraries: `brew install raylib` or `apt-get install libraylib-dev`

---

## Getting the Code

```bash
git clone https://github.com/your-org/Heros_of_Taboo.git
cd Heros_of_Taboo
git submodule update --init --recursive   # pulls Raylib submodule
```

---

## Building the Project

### Generate Build Files

```bash
premake5 gmake2    # generates Makefiles
```

### Compile (Windows)

```cmd
cd game
C:\raylib\w64devkit\bin\make.exe config=debug_x64
```

### Compile (macOS/Linux)

```bash
cd game
make config=debug_x64
```

### Build Configurations

| Config | Description |
|--------|-------------|
| `debug_x64` | Debug build, 64-bit, with assertions |
| `debug_x86` | Debug build, 32-bit |
| `release_x64` | Optimized release, 64-bit |
| `release_x86` | Optimized release, 32-bit |

### Output

The compiled executable is placed in `bin/Debug/` or `bin/Release/`:
- Windows: `Heroes of Taboo.exe`

### Clean Build

```cmd
C:\raylib\w64devkit\bin\make.exe clean
```

---

## Running the Game

```cmd
bin\Debug\Heroes of Taboo.exe
```

The game window opens at 1024×768 (resizable). The main menu appears first.

### Default Controls

| Key | Action |
|-----|--------|
| WASD / Arrow keys | Move |
| F | Attack (melee, in facing direction) |
| T | Throw equipped weapon |
| Space | Wait one turn (heals HP) |
| I | Open/close inventory |
| M / Z | Toggle minimap |
| E | Interact (inspect tile, enter shop) |
| ESC | Pause menu / close overlay |
| Shift+R | Restart game (when dead or won) |

---

## Running Tests

### Run All Tests

```cmd
cd game
C:\raylib\w64devkit\bin\make.exe config=debug_x64 test
```

### Expected Output

```
=== Heroes of Taboo Unit Tests ===
Total: 28 tests

[ 1/28] xp_curve
  PASS  test_xp_curve
[ 2/28] max_hp
  PASS  test_max_hp
...
[28/28] game_world_init_clears_grid
  PASS  test_game_world_init_clears_grid

=== Results: 28 passed, 0 failed, 28 total ===
```

### Test Categories

| Category | Tests | What They Cover |
|----------|-------|----------------|
| Stat calculations | 11 | XP curve, max HP, melee/ranged/throw/magic damage, defense, dodge, throw range, wait heal, potion heal |
| Data tables | 2 | Equipment table validation, item data validation |
| Equipment bonus | 3 | Apply/remove, recalculate, edge cases (NULL, EQUIP_NONE, missing components) |
| Validation | 7 | Inventory slot, equip type, item type, monster type, stat index, floor, clamp |
| Challenge Rating | 3 | Goblin CR, floor budget, floor scale |
| Spatial hash | 5 | Basic add/remove, move, exclude, alive counter, grid clear |

### Writing a New Test

Add to `tests/test_runner.c`:

```c
static bool test_my_new_feature(void)
{
    // Setup
    GameWorld gw;
    GameWorld_Init(&gw);

    // Exercise
    EntityId e = World_CreateEntity(&gw.ecs);
    CHECK(e != ENTITY_NONE, "entity creation succeeded");

    World_AddComponent(&gw.ecs, e, COMP_STATS);
    CStats* s = World_GetStats(&gw.ecs, e);
    s->str = 5;

    // Assert
    CHECK_EQ(s->str, 5, "STR is 5");

    TEST_PASS();
    return true;
}
```

Then add to the `g_tests[]` array:

```c
{"my_new_feature", test_my_new_feature},
```

---

## Understanding the Codebase

### Start Here (Reading Order)

1. **[PROJECT_OVERVIEW.md](PROJECT_OVERVIEW.md)** — What the game is, key features, tech stack
2. **[ARCHITECTURE.md](ARCHITECTURE.md)** — ECS pattern, design patterns, layered architecture
3. **[FOLDER_BREAKDOWN.md](FOLDER_BREAKDOWN.md)** — What each file does
4. **[DATA_FLOW.md](DATA_FLOW.md)** — How data moves through the game loop
5. **[SYSTEMS.md](SYSTEMS.md)** — How systems interact with each other

### Key Files to Read First

| File | Why |
|------|-----|
| `game/src/game_types.h` | All enums and core types — the vocabulary of the codebase |
| `game/src/components.h` | All ECS component structs — the data model |
| `game/include/game_balance.h` | All formulas and constants — the game rules |
| `game/src/world.h` | The GameWorld struct — the central state object |
| `engine/ecs.h` | The ECS API — how entities and components work |
| `game/src/game.c` | The game loop — how everything connects |

### Mental Model

Think of the game as three layers:

1. **Data** (components, data tables, GameWorld fields) — the state
2. **Systems** (AI, combat, input, movement, etc.) — the logic that reads/mutates state
3. **Rendering** (renderer, UI modules) — the visual output

Every frame: Input → Systems mutate state → Renderer draws state.

---

## Making Your First Change

### Example: Add a New Potion Type

**Step 1**: Add enum value in `game_types.h`:
```c
typedef enum {
    ITEM_NONE = 0,
    ITEM_SMALL_HP_POTION,
    ITEM_MEDIUM_HP_POTION,
    ITEM_LARGE_HP_POTION,
    ITEM_MANA_POTION,      // ← NEW
    ITEM_COUNT
} ItemType;
```

**Step 2**: Add data in `inventory.c`:
```c
static const char* ITEM_NAMES[ITEM_COUNT] = {
    "", "Small HP Potion", "Medium HP Potion", "Large HP Potion",
    "Mana Potion"          // ← NEW
};
// ... add to ITEM_HEALS, ITEM_DESCS, ITEM_SPRITES
```

**Step 3**: Add handling in `InventoryUse()`:
```c
// In inventory.c, InventoryUse():
if (s->type == ITEM_MANA_POTION) {
    CAbilities* ab = World_GetAbilities(&game->ecs, game->playerEntity);
    ab->mp += 20;
    if (ab->mp > ab->maxMp) ab->mp = ab->maxMp;
}
```

**Step 4**: Add test in `test_runner.c`:
```c
static bool test_mana_potion(void) {
    // ... setup, use mana potion, assert MP increased
}
```

**Step 5**: Build and test:
```cmd
make config=debug_x64
make config=debug_x64 test
```

---

## Coding Conventions

### Must Follow (Non-Negotiable)

| Rule | Example |
|------|---------|
| **C99 only** | No `_Generic`, no VLAs, no designated initializers |
| **Use Raylib types** | `Texture2D`, `Vector2`, `Color` — no custom wrappers |
| **Textures via Resources_LoadTexture()** | Never call `LoadTexture()` directly |
| **Components in components.h** | All component structs in one file |
| **No static in systems** | All mutable state in GameWorld or caller-owned structs |
| **Entity 0 is ENTITY_NONE** | Never allocate or write to slot 0 |
| **Check component masks** | Always `World_HasComponents()` before accessing |
| **Idempotent functions** | Apply/Remove safe to call multiple times |
| **All tests pass** | 28/28 before any commit |

### Style Conventions

| Convention | Example |
|-----------|---------|
| **Naming** | `PascalCase` for types, `camelCase` for functions, `UPPER_SNAKE` for constants |
| **System functions** | `SystemName_Verb()` — e.g., `AISystem_ProcessAll()`, `CombatSystem_PlayerMeleeAttack()` |
| **Component prefix** | `C` prefix — `CPosition`, `CStats`, `CAI` |
| **Component bitmask** | `COMP_` prefix — `COMP_POSITION`, `COMP_STATS` |
| **Header guards** | `#ifndef MODULE_H` / `#define MODULE_H` / `#endif` |
| **Forward declarations** | `typedef struct GameWorld GameWorld;` to avoid circular includes |
| **Error handling** | Return `bool` (true=success), `ENTITY_NONE` for failed allocations |
| **No comments** | Unless explicitly asked — code should be self-documenting |

### File Organization

- **Headers** declare the public API (types + function signatures)
- **Source files** implement the API
- **One header per module** — no multi-header modules
- **Include what you use** — each `.c` file includes only what it needs

---

## Common Tasks

### Adding a New Monster Type

1. Add enum value to `MonsterType` in `game_types.h`
2. Add template entry in `monster_data.c` (stats, sprite, behavior)
3. Add sprite file to `resources/sprites/monsters/`
4. Add to biome monster pool in `biome_data.c`
5. Add name variants in `spawner_system.c` (optional meme names)
6. Build and test

### Adding a New Equipment Type

1. Add enum value to `EquipType` in `game_types.h`
2. Add entry to `EQUIP_TABLE[]` in `inventory.c`
3. Add sprite file to `resources/sprites/items/equipment/`
4. Add to loot table in `loot_data.c` (if droppable)
5. Add to monster weapon/armor pools in `monster_data.c` (if equippable by monsters)
6. Build and test

### Adding a New Game State

1. Add enum value to `GameState` in `game_types.h`
2. Add handling in `UpdateGame()` (game.c)
3. Add handling in `InputSystem_PlayerTurn()` (input_system.c)
4. Add rendering in `RenderGame()` (renderer.c)
5. Build and test

### Modifying a Damage Formula

1. Edit the formula in `game_balance.h` (inline function)
2. Update related tests in `test_runner.c`
3. Build and run tests — all 28 must pass
4. Play-test to verify feel

### Adding a New UI Panel

1. Create `game/src/ui/my_panel.h` and `my_panel.c`
2. Declare render function: `void MyPanel_Render(const GameWorld* gw)`
3. Call from `RenderGame()` in `renderer.c` (screen-space, after `EndMode2D()`)
4. Add to `premake5.lua` if needed (auto-detected by glob)
5. Build and test

---

## Debugging Tips

### Enable Debug Assertions

Build with `config=debug_x64` to enable ECS assertions. These check:
- Entity is not `ENTITY_NONE`
- Entity is within bounds
- Entity is alive
- Entity has the required component

If an assertion fires, it means a system is accessing a component on an entity that doesn't have it. Check the component mask before accessing.

### Common Bugs

| Symptom | Cause | Fix |
|---------|-------|-----|
| Crash on floor transition | Stale entity reference | Ensure all entity IDs are reset or re-validated after `DescendFloor()` |
| Monster appears at (0,0) | Spatial hash not updated | Call `SpatialHash_Add()` after spawning |
| Stats don't change after equip | Missing `EquipmentBonus_Apply()` | Call Apply after setting `equipped[slot]` |
| HP exceeds maxHp after equip | Missing `EquipmentBonus_Recalculate()` | Recalculate is called automatically by Apply/Remove |
| Ghost monster (can't interact) | Grid says monster exists but entity is dead | Call `SpatialHash_Remove()` on death |
| Test fails after formula change | Test expectations outdated | Update expected values in `test_runner.c` |

### Using TraceLog

Raylib provides `TraceLog(level, fmt, ...)` for debug output:

```c
TraceLog(LOG_INFO, "Player at (%d, %d)", pos->x, pos->y);
TraceLog(LOG_WARNING, "Could not load texture: %s", path);
TraceLog(LOG_ERROR, "Failed to generate map for floor %d", floor);
```

Output appears in the console window.

---

## Getting Help

- **Architecture questions**: Read [ARCHITECTURE.md](ARCHITECTURE.md) and [SYSTEMS.md](SYSTEMS.md)
- **API questions**: Read [API_REFERENCE.md](API_REFERENCE.md)
- **Data flow questions**: Read [DATA_FLOW.md](DATA_FLOW.md)
- **Build issues**: Check [AGENTS.md](../AGENTS.md) for build instructions
- **Bug reports**: https://github.com/your-org/Heros_of_Taboo/issues
- **AI agent contributors**: Read [AGENTS.md](../AGENTS.md) for non-negotiable constraints
