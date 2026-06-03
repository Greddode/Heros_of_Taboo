# Heroes of Taboo — AI Agent Project Rules & Architecture

**Last Updated**: June 3, 2026 | **Version**: v0.0.9 | **Test Status**: ✅ 28/28 passing

---

## Table of Contents

1. [Non-negotiable Constraints](#non-negotiable-constraints)
2. [Current Architecture](#current-architecture-post-ecsrefactor-v009)
3. [ECS Pattern & Data Model](#ecs-pattern--data-model)
4. [Recent Refactoring Work](#recent-refactoring-work-may-30--june-3-2026)
5. [Profiling Hot-Spots](#profiling-hot-spots-optimization-roadmap)
6. [Remaining Work & Known Issues](#remaining-work--known-issues)
7. [Build & Test Workflow](#build--test-workflow)
8. [Common Refactoring Patterns](#common-refactoring-patterns)
9. [Next Steps for AI Agents](#next-steps-for-ai-agents)
10. [Quick Reference](#quick-reference-file-locations)
11. [Module Dependency Graph](#module-dependency-graph)
12. [FAQ](#frequently-questioned-decisions)

---

## Non-negotiable Constraints

These apply to **every task**, every commit, every AI agent contribution.

- **C99 only.** No C11/C23 features. No VLAs, designated initializers, or `_Generic`.
- **Raylib types directly** (`Texture2D`, `Camera2D`, `Vector2`, `Color`, `Sound`, `Music`). No custom wrappers.
- **All textures loaded exclusively through** `Resources_LoadTexture(path)` in `engine/resources.h`. No direct `LoadTexture()` calls outside the resource manager.
- **All new component structs** go in `game/src/components.h` only. Never scatter component definitions across system files.
- **Systems are stateless** — no `static` local variables in system `.c` files. All mutable state lives in `GameWorld` or caller-owned structs passed by pointer.
- **Entity 0 is `ENTITY_NONE`** — never allocate or write to entity slot 0. It is reserved as a sentinel.
- **No new runtime dependencies.** No additional libraries beyond Raylib. Update `game/premake5.lua` include paths if new files are added.
- **Preserve all existing gameplay** — turn order, combat formulas, fog of war, exp curves, AI behavior, key bindings, and rendering output must be identical unless a task **explicitly specifies** a change.
- **All 28 unit tests must pass** before committing. Keep test runtime under 2 seconds. Run with `cd game && make test`.
- **ECS iteration is mandatory** — always check component masks with `World_HasComponents()`. Never iterate by assuming entity types.
- **Idempotent functions preferred** — functions like `EquipmentBonus_Apply()`, `EquipmentBonus_Remove()` should be safe to call multiple times without double-applying effects.

---

## Current Architecture (Post-ECS Refactor, v0.0.9)

```
project/
├── engine/                       # ECS core + resource manager
│   ├── ecs.h/.c                  # Entity pool, component bitmask system
│   ├── resources.h/.c            # Texture/audio loading via path (single entry point)
│   └── world.h/.c                # GameWorld struct with ECS world + cached refs
│
├── game/
│   ├── include/
│   │   ├── game_balance.h         # ⭐ CENTRALIZED: all formulas, magic numbers, constants
│   │   │                          #   - Damage calculations (melee, ranged, magic)
│   │   │                          #   - Dodge/crit formulas
│   │   │                          #   - XP scaling curves
│   │   │                          #   - Player base stats, UI scale, camera constants
│   │   ├── equipment_bonus.h      # Equipment stat apply/remove/recalculate (idempotent)
│   │   ├── floor_init.h           # Shared floor setup (InitGame + DescendFloor)
│   │   └── validation.h           # Input validation predicates (pure, no side effects)
│   │
│   ├── src/
│   │   ├── main.c                 # Entry point, scene dispatcher (MENU, GAME, MAP, WIN)
│   │   ├── components.h           # All component struct definitions (CANONICAL location)
│   │   │                          #   - CPosition, CStats, CSpriteAnim, CAI, CInventory, etc.
│   │   ├── game.h/.c              # Game state bridge (thin: holds GameWorld + UI textures)
│   │   │                          #   - InitGame, CleanupGame, UpdateGame, DescendFloor
│   │   │                          #   - HandleInput, set game state flags
│   │   ├── world.h/.c             # GameWorld struct (ECS world + cached references)
│   │   │                          #   - Entity pool, component arrays
│   │   │                          #   - Cached player entity, visible monsters, etc.
│   │   │                          #   - Dirty flag tracking for cache invalidation
│   │   │
│   │   ├── inventory.c/.h         # Potion/item logic (legacy, not refactored)
│   │   ├── equipment_bonus.c      # Equipment stat apply/remove (extracted, no duplication)
│   │   ├── floor_init.c           # Floor initialization (extracted from InitGame/DescendFloor)
│   │   ├── validation.c           # Validation helpers (pure predicates, no side effects)
│   │   │
│   │   ├── systems/
│   │   │   ├── ai_system.h/.c     # Monster AI (hunt, flank, kite, ranged targeting)
│   │   │   ├── combat_system.h/.c # Damage calc, hit/miss resolution, ranged projectiles
│   │   │   ├── input_system.h/.c  # Keyboard input handling (sprint, inventory, etc.)
│   │   │   ├── movement_system.h/.c # Player/monster movement + collision
│   │   │   ├── render_system.h/.c # Map tiles, entities, HUD, UI elements
│   │   │   ├── world_monster.c/.h # Monster queries (FindMonsterAt, CountAlive, animations)
│   │   │   └── spawner.c/.h       # Entity spawning, loot tables, treasure generation
│   │   │
│   │   ├── ui/
│   │   │   ├── menu.c/.h          # Main menu, story, controls, settings screens
│   │   │   ├── combat_log.c/.h    # Combat message ring buffer + rendering
│   │   │   ├── inspector.c/.h     # Monster/item info panel
│   │   │   ├── inventory_ui.c/.h  # Inventory overlay with tabs (Inventory/Equipment/Stats)
│   │   │   ├── text_data.c/.h     # XOR-encrypted story/controls text
│   │   │   └── hud.c/.h           # Floor/HP/level display
│   │   │
│   │   ├── data/
│   │   │   ├── monster_data.c/.h  # Monster templates, stat scaling, spawn rates
│   │   │   ├── loot_data.c/.h     # Loot table (4 tiers, rarity weights, floor filters)
│   │   │   ├── item_data.c/.h     # Potion metadata (name, sprite, heal amount)
│   │   │   └── equip_data.c/.h    # Equipment metadata (17 types, stat bonuses)
│   │   │
│   │   ├── map/
│   │   │   ├── procedural.c/.h    # Dungeon generation (room/corridor BSP)
│   │   │   ├── tmx_loader.c/.h    # Tiled map loading and parsing
│   │   │   └── map_helpers.c/.h   # FOW, visibility rays, blocking map, stairs
│   │   │
│   │   └── audio/
│   │       ├── audio_system.c/.h  # Music context system, sound categories
│   │       └── [music/sounds]     # Resource loading (see resources/)
│   │
│   ├── Makefile                   # Build targets: config=debug_x64, test
│   └── Makefile.windows           # Windows-specific build (uses w64devkit)
│
├── tests/
│   ├── test_runner.c              # 28 unit tests (all formulas, equipment, validation, edge cases)
│   └── README.md                  # Test documentation
│
├��─ docs/
│   ├── API.md                     # Complete public API for all modules
│   ├── profilingreport.md         # ⭐ Hot-path analysis + optimization recommendations
│   ├── VERSIONS.md                # Git tag mapping (v0.0.8, v0.0.9, etc.)
│   └── HISTORY.md                 # Pre-refactor changelog (ALPHA-0.0.4 through 0.0.7)
│
├── resources/
│   ├── tilesets/                  # VelmoraRealms tileset + TSX definition
│   ├── sprites/                   # Equipment, monsters, UI, items
│   ├── audio/                     # Music (game/ + menu/), sound effects
│   └── ...
│
├── compile_flags.txt              # Clang/LSP configuration
├── premake5.lua                   # Main build config (Lua)
├── raylib_premake5.lua            # Raylib submodule config
├── changelog.md                   # Feature history (v0.0.4 → v0.0.9, latest first)
└── AGENTS.md                      # THIS FILE

```

---

## ECS Pattern & Data Model

**Type**: Structure-of-Arrays with bitmask component ownership.

```c
// Entity ID = index into component arrays
typedef uint32_t EntityId;
#define ENTITY_NONE 0

// Component bitmask (each bit = component present on entity)
#define COMPONENT_POSITION    (1 << 0)
#define COMPONENT_STATS       (1 << 1)
#define COMPONENT_SPRITE_ANIM (1 << 2)
#define COMPONENT_AI          (1 << 3)
#define COMPONENT_PLAYER_TAG  (1 << 4)
// ... etc

// Mandatory iteration pattern (REQUIRED in all systems)
for (EntityId e = 1; e < gw->ecs.count; e++) {
    uint32_t mask = COMPONENT_POSITION | COMPONENT_STATS;
    if (!World_HasComponents(&gw->ecs, e, mask)) continue;
    
    // Access components via accessor functions
    CPosition* pos = World_GetPosition(gw, e);
    CStats* stats = World_GetStats(gw, e);
    // ... no direct array indexing
}
```

**Key Rule**: Never iterate without checking component mask. Use accessor functions (not direct array indexing).

---

## Recent Refactoring Work (May 30 – June 3, 2026)

### ✅ Completed Tasks

| Task | Scope | Status | Key Changes |
|------|-------|--------|-------------|
| **Step 9: ECS Migration** | Remove `Player` struct, unify state in `GameWorld` | ✅ Complete | `Game` now thin bridge; all state via ECS accessors |
| **game_balance.h** | Centralize all magic numbers & formulas | ✅ Complete | ~121 lines of constants replacing scattered literals |
| **equipment_bonus.c** | Extract duplicated equip/unequip logic | ✅ Complete | Idempotent Apply/Remove/Recalculate; used by 3 call sites |
| **floor_init.c** | Extract shared InitGame/DescendFloor setup | ✅ Complete | ~60 lines deduplication |
| **validation.c** | Input validation helpers | ✅ Complete | 7 pure predicates (no side effects) |
| **Unit Tests** | Expand from 16 → 28 tests | ✅ Complete | All formulas, equipment, validation, edge cases covered |
| **API Documentation** | Complete public API reference | ✅ Complete | `docs/API.md` — all modules, signatures, side effects |
| **Profiling Report** | Identify performance hot-paths | ✅ Complete | `docs/profilingreport.md` — 5 bottlenecks with fixes |
| **v0.0.9 Bug Fixes** | 7 critical bugs | ✅ Complete | Tile tearing, wrap, ranged AI, magic def, map close, HP scaling, ranged weapons |
| **Git Tags** | Version tracking | ✅ Complete | `v0.0.8`, `v0.0.9` created, mapped in `docs/VERSIONS.md` |

---

## Profiling Hot-Spots (Optimization Roadmap)

**Source**: `docs/profilingreport.md` (generated June 2, 2026)

### Top 5 Bottlenecks

| Rank | Function | Complexity | Calls/Frame | Impact | Suggested Fix |
|------|----------|-----------|-------------|--------|----------------|
| **1** | `World_FindMonsterAt` | O(n) | 40–120 | **CRITICAL** | Add spatial hash (2D position map) — ~20 LOC, **Low risk** |
| **2** | `AISystem_ProcessAll` | O(n²) | ~128 | **HIGH** | Batch component fetches into local array — ~10 LOC |
| **3** | `World_CountAliveMonsters` | O(n) | 1/frame | **MEDIUM** | Cache counter with dirty flag — ~5 LOC, **Very low risk** |
| **4** | `RenderSystem_DrawMonsters` | O(n) | 1/frame | **LOW** | Pre-filter visible entities — ~15 LOC |
| **5** | `World_UpdateMonsterAnimations` | O(n) | 1/frame | **LOW** | Combine with render cache — ~5 LOC |

**Estimated improvement**: Spatial hash alone reduces AI turn from **O(n²) to O(n)**, improving floor 10+ (20+ monsters) by ~8–12x.

### Cache Invalidation Points (Critical for Safety)

```c
// Spatial hash invalidation must occur:
- On monster spawn (add to grid)
- On monster death (remove from grid)
- On monster move (update grid cell)
- On floor transition (clear grid, rebuild)
- On map resize (rebuild grid)

// Alive-count invalidation must occur:
- On monster spawn (increment)
- On monster death (decrement)
- On floor transition (reset to 0, recount from spawner)

// Render filter invalidation:
- On monster spawn/death
- On visibility change (FOW update)
- On floor transition (rebuild list)
```

---

## Remaining Work & Known Issues

### 🔴 High Priority (Blocking Performance)

**1. ECS Spatial Hash Optimization** [RECOMMENDED FOR AI]
   - **Problem**: `World_FindMonsterAt()` is O(n), called 40–120 times per AI turn for collision detection
   - **Symptom**: Noticeable lag on floor 8+ with 20+ monsters
   - **Solution**: Implement grid-based spatial hash with dirty-flag invalidation
   - **Risk**: Cache coherency across floor transitions — must validate in tests
   - **Effort**: 4–6 hours
   - **Files**: `world.h/.c`, new optional `spatial_hash.h/.c` (or inline in `world.c`)
   - **Reference**: See `docs/profilingreport.md` section 1 for details

**2. ECS Query Batching in AI Loop**
   - **Problem**: `AISystem_ProcessAll()` fetches components per entity (repeated overhead)
   - **Solution**: Batch component fetches into local arrays before loop
   - **Effort**: 2–3 hours
   - **Files**: `systems/ai_system.c`

**3. Alive-Count Caching**
   - **Problem**: `World_CountAliveMonsters()` is full O(n) scan per frame for state logic
   - **Solution**: Maintain cached counter with dirty-flag on spawn/death
   - **Effort**: 1–2 hours
   - **Files**: `world.h/.c`, invalidation points in spawner + combat system

### 🟡 Medium Priority (Testing & CI/CD) [RECOMMENDED FOR AI]

**4. CI/CD Pipeline**
   - **Missing**: `.github/workflows/ci.yml`
   - **Should do**: Run tests, cross-compile (Linux/Windows), fail on test failure
   - **Test runtime**: Keep under 2 seconds
   - **Effort**: 1–2 hours
   - **Reference**: Use existing `make test` in `game/Makefile`

**5. Integration Tests**
   - **Current**: 28 unit tests (formulas, validation)
   - **Missing**: Gameplay scenarios (combat flow, floor descent, equipment equip)
   - **Add**: 5–10 tests covering:
     - Player melee damage → monster death → loot drop
     - Monster spawn per floor → stat scaling verification
     - Equipment equip → stat recalc → UI display
     - Floor descent → state preservation (player HP, inventory, level)
     - Loot tier probabilities per floor
   - **Effort**: 3–4 hours
   - **Framework**: C native (no external dependencies)

### 🟢 Low Priority (Documentation & Cleanup)

**6. Repository Bloat**
   - **Problem**: Binary premake files (2.6MB+) committed
   - **Solution**: Add to `.gitignore`, document build setup
   - **Impact**: Minimal (doesn't affect functionality)
   - **Effort**: 30 min

**7. Changelog Reorganization**
   - **Problem**: `changelog.md` mixes recent refactoring with old historical entries
   - **Solution**: Move pre-refactor history to `docs/HISTORY.md` (already done)
   - **Benefit**: Easier navigation for new contributors
   - **Effort**: 30 min

**8. Build Documentation**
   - **Missing**: Clear setup instructions for w64devkit
   - **Add**: `BUILD.md` with platform-specific steps
   - **Effort**: 1 hour

---

## Build & Test Workflow

### Prerequisites

**Windows (w64devkit @ `C:\raylib\w64devkit`)**:
```sh
# Verify installation
C:\raylib\w64devkit\bin\gcc.exe --version
C:\raylib\w64devkit\bin\make.exe --version
```

**macOS/Linux**:
```sh
brew install premake5          # or apt-get install premake5
```

### Compile

**Windows**:
```cmd
cd game
C:\raylib\w64devkit\bin\make.exe config=debug_x64
```

**macOS/Linux**:
```sh
cd game
make config=debug_x64
```

### Run Tests

**Windows**:
```cmd
cd game
C:\raylib\w64devkit\bin\make.exe config=debug_x64 test
```

**macOS/Linux**:
```sh
cd game
make config=debug_x64 test
```

**Expected Output**:
```
Running 28 tests...
✓ test_xp_curve
✓ test_damage_formula
✓ test_equipment_bonus_apply
✓ test_equipment_bonus_remove
✓ test_dodge_formula
✓ test_crit_chance
✓ test_throw_range
✓ test_wait_heal
✓ test_potion_heal
✓ test_inventory_slot_validate
✓ test_equip_type_validate
✓ test_item_type_validate
✓ test_monster_type_validate
✓ test_stat_index_validate
✓ test_floor_validate
✓ test_clamp_int
... (28 total)
All tests passed in 1.2s
```

### Clean

**Windows**:
```cmd
C:\raylib\w64devkit\bin\make.exe clean
```

**macOS/Linux**:
```sh
make clean
```

---

## Common Refactoring Patterns

### Pattern 1: Adding a New System

**Goal**: Create a modular system that runs independently within the game loop.

1. **Create header** (`game/src/systems/my_system.h`):
   ```c
   #ifndef MY_SYSTEM_H
   #define MY_SYSTEM_H
   
   #include "world.h"
   
   void MySystem_Update(GameWorld* gw);
   
   #endif
   ```

2. **Define component** in `game/src/components.h`:
   ```c
   typedef struct {
       int myField;
       float timer;
   } CMyComponent;
   
   #define COMPONENT_MY_COMPONENT (1 << 7)
   ```

3. **Implement system** (`game/src/systems/my_system.c`):
   ```c
   #include "my_system.h"
   #include "components.h"
   
   void MySystem_Update(GameWorld* gw) {
       for (EntityId e = 1; e < gw->ecs.count; e++) {
           uint32_t mask = COMPONENT_MY_COMPONENT;
           if (!World_HasComponents(&gw->ecs, e, mask)) continue;
           
           CMyComponent* comp = World_GetComponent(gw, e, COMPONENT_MY_COMPONENT);
           // ... update comp->timer, comp->myField
           // NO static state allowed
       }
   }
   ```

4. **Integrate into game loop** (`game/src/game.c`):
   ```c
   #include "systems/my_system.h"
   
   void UpdateGame(Game* game, float dt) {
       GameWorld* gw = &game->ecsWorld;
       // ... existing systems
       MySystem_Update(gw);
       // ... other systems
   }
   ```

5. **Add test** in `tests/test_runner.c`:
   ```c
   void test_my_system_updates_component() {
       GameWorld gw = World_Create(128);
       EntityId e = World_AllocateEntity(&gw);
       World_AddComponent(&gw, e, COMPONENT_MY_COMPONENT);
       
       CMyComponent* comp = World_GetComponent(&gw, e, COMPONENT_MY_COMPONENT);
       comp->timer = 5.0f;
       
       MySystem_Update(&gw);
       
       ASSERT(comp->timer == 4.0f); // decreased by 1.0
       World_Destroy(&gw);
   }
   ```

6. **Update documentation** in `docs/API.md`:
   ```markdown
   ### MySystem
   
   **Header**: `game/src/systems/my_system.h`
   
   #### MySystem_Update
   - **Signature**: `void MySystem_Update(GameWorld* gw)`
   - **Side effects**: Updates `CMyComponent.timer` on all entities with `COMPONENT_MY_COMPONENT`
   - **Preconditions**: `gw` is valid and initialized
   - **Thread safety**: Not thread-safe (assumes single-threaded)
   ```

### Pattern 2: Extracting Duplicated Logic

**Goal**: Identify repeated code across 2+ files and extract to a shared module.

1. **Identify duplication** (use `grep -rn` across `game/src/`):
   ```sh
   grep -rn "EquipmentBonus_Apply" game/src/
   # Result: 3 call sites in inventory.c, game.c, input_system.c
   ```

2. **Create shared module** (`game/include/equipment_bonus.h`):
   ```c
   #ifndef EQUIPMENT_BONUS_H
   #define EQUIPMENT_BONUS_H
   
   #include "world.h"
   
   // Idempotent: safe to call multiple times
   void EquipmentBonus_Apply(GameWorld* gw, EntityId player, EquipType eq);
   void EquipmentBonus_Remove(GameWorld* gw, EntityId player, EquipType eq);
   void EquipmentBonus_Recalculate(GameWorld* gw, EntityId player);
   
   #endif
   ```

3. **Implement** (`game/src/equipment_bonus.c`):
   ```c
   #include "equipment_bonus.h"
   #include "game_balance.h"
   #include "data/equip_data.h"
   
   void EquipmentBonus_Apply(GameWorld* gw, EntityId player, EquipType eq) {
       // Extract the logic from all 3 call sites
       // Make it work for any call order (idempotent)
   }
   ```

4. **Replace all call sites**:
   ```c
   // Before (in inventory.c)
   game->player.stats.atk += EQUIP_TABLE[eq].atk;
   
   // After (in inventory.c)
   EquipmentBonus_Apply(&game->ecsWorld, game->ecsWorld.playerEntity, eq);
   ```

5. **Verify no duplication remains**:
   ```sh
   grep -rn "stats.atk +=" game/src/ | grep -v equipment_bonus.c
   # Should return 0 matches
   ```

6. **Add tests** covering edge cases (NULL, EQUIP_NONE, missing components):
   ```c
   void test_equipment_bonus_apply_null_world() {
       // Verify behavior with NULL pointer
   }
   
   void test_equipment_bonus_apply_idempotent() {
       // Call Apply twice, verify result same as once
   }
   ```

### Pattern 3: Profiling & Optimizing

**Goal**: Identify bottlenecks, measure improvement, validate correctness.

1. **Run baseline** (establish current state):
   ```sh
   cd game
   make test
   # Time the test suite
   ```

2. **Measure hot-path** (use `perf` on Linux or manual timing):
   ```c
   // In system that needs optimization
   #include <time.h>
   
   clock_t start = clock();
   // ... code to measure
   clock_t end = clock();
   double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
   printf("Elapsed: %f seconds\n", elapsed);
   ```

3. **Implement optimization** (from `docs/profilingreport.md`):
   ```c
   // Before: O(n) search per call
   EntityId World_FindMonsterAt(GameWorld* gw, int x, int y) {
       for (EntityId e = 1; e < gw->ecs.count; e++) {
           CPosition* pos = World_GetPosition(gw, e);
           if (pos->x == x && pos->y == y) return e;
       }
       return ENTITY_NONE;
   }
   
   // After: O(1) lookup via spatial hash
   EntityId World_FindMonsterAt(GameWorld* gw, int x, int y) {
       return gw->spatialHash[y][x];
   }
   ```

4. **Re-measure** (show before/after timings):
   ```
   Before: 145ms per AI turn (12 monsters)
   After:  18ms per AI turn
   Improvement: 8.0x
   ```

5. **Validate correctness** (ensure all tests pass):
   ```sh
   make test
   # All 28 tests must pass
   ```

6. **Document** in commit message and `docs/profilingreport.md`:
   ```
   docs: update profiling report with spatial hash results
   
   Implemented spatial hash for monster position lookups.
   
   Before: O(n) linear scan, 145ms per AI turn (12 monsters)
   After: O(1) hash lookup, 18ms per AI turn
   Improvement: 8.0x faster
   
   Cache invalidation points:
   - Monster spawn: add to grid
   - Monster death: remove from grid
   - Monster move: update grid[old_y][old_x] → grid[new_y][new_x]
   - Floor transition: clear and rebuild grid
   
   All 28 tests pass. No gameplay changes observed.
   ```

---

## Next Steps for AI Agents

**Recommended Priority Order** (with effort estimates):

### Phase 1: Foundation (CI/CD & Testing) — 4–5 hours

**Task 1.1: Add CI/CD Pipeline** 🔵 QUICK WIN
- Create `.github/workflows/ci.yml`
- Run `make test` on push
- Cross-compile Windows + Linux
- Fail build on test failure
- **Effort**: 1–2 hours
- **Highest ROI**: Catches regressions immediately

**Task 1.2: Add Integration Tests** 🟡 MEDIUM
- Write 5–10 gameplay scenario tests
- Coverage: combat flow, floor descent, equipment
- Use existing `tests/test_runner.c` framework
- **Effort**: 2–3 hours
- **Value**: Validates gameplay contracts

### Phase 2: Performance (ECS Optimization) — 4–6 hours

**Task 2.1: Implement Spatial Hash for FindMonsterAt** 🔴 HIGH PRIORITY
- Implement grid-based lookup (O(n) → O(1))
- Add cache invalidation on monster move/spawn/death
- Benchmark before/after (target 8x improvement on floor 10+)
- **Effort**: 4–6 hours
- **Risk**: Low if cache invalidation is correct
- **Impact**: Critical for late-game performance

**Task 2.2: Batch AI Component Fetches**
- Cache component pointers in `AISystem_ProcessAll`
- Reduce redundant `World_GetPosition`/`World_GetStats` calls
- **Effort**: 1–2 hours
- **Risk**: Very low

**Task 2.3: Cache Alive-Monster Count**
- Replace O(n) `World_CountAliveMonsters()` with cached counter
- Track dirty flag on spawn/death
- **Effort**: 1–2 hours
- **Risk**: Very low

### Phase 3: Polish (Documentation) — 2–3 hours

**Task 3.1: Create Build.md**
- Document w64devkit setup (`C:\raylib\w64devkit`)
- Platform-specific build steps
- Troubleshooting section
- **Effort**: 1 hour

**Task 3.2: Organize Repository**
- Add premake binaries to `.gitignore`
- Move `HISTORY.md` for pre-refactor entries (optional)
- Create version tag documentation
- **Effort**: 1–2 hours

---

## Quick Reference: File Locations

| Category | Files | Purpose |
|----------|-------|---------|
| **Core ECS** | `engine/ecs.h/.c` | Entity pool, component management |
| | `game/src/world.h/.c` | GameWorld struct, cached refs |
| **Formulas** | `game/include/game_balance.h` | ALL magic numbers, constants |
| **Equipment** | `game/src/equipment_bonus.c/.h` | Stat apply/remove/recalc |
| | `game/src/data/equip_data.c/.h` | Equipment table (17 types) |
| **Combat** | `game/src/systems/combat_system.c/.h` | Damage calc, hit/miss, projectiles |
| **AI** | `game/src/systems/ai_system.c/.h` | Monster behavior (hunt, flank, kite) |
| **Rendering** | `game/src/systems/render_system.c/.h` | Map tiles, entities, HUD |
| **Input** | `game/src/systems/input_system.c/.h` | Keyboard handling, sprint logic |
| **Spawning** | `game/src/systems/spawner.c/.h` | Monster/loot generation |
| **UI** | `game/src/ui/` | Menu, combat log, inventory, inspector |
| **Map** | `game/src/map/` | Procedural generation, TMX loader, FOW |
| **Data** | `game/src/data/` | Monster, loot, item, equip tables |
| **Tests** | `tests/test_runner.c` | 28 unit tests (formulas, edge cases) |
| **Docs** | `docs/API.md` | Complete public API reference |
| | `docs/profilingreport.md` | Hot-path analysis + fixes |
| | `docs/VERSIONS.md` | Git tag mapping |
| | `changelog.md` | Feature history (v0.0.4 → v0.0.9) |
| **Build** | `game/Makefile` | Compile, test targets |
| | `premake5.lua` | Main Premake config |
| | `raylib_premake5.lua` | Raylib submodule |

---

## Module Dependency Graph

```
game_balance.h (LEAF — no dependencies)
  ↑
  ├─ combat_system.c (damage formulas)
  ├─ player.c (level-up, stat allocation)
  ├─ spawner.c (XP table, loot weighting)
  └─ monster_data.c (stat scaling)

equipment_bonus.h
  ├─ game_balance.h
  ↑
  ├─ inventory.c
  ├─ game.c
  └─ input_system.c

floor_init.h
  ├─ game_balance.h
  ├─ monster_data.h
  ↑
  ├─ game.c (InitGame, DescendFloor)
  └─ map_helpers.c

validation.h (LEAF — no dependencies)
  ↑
  └─ [all systems that validate input]

components.h (LEAF — only struct definitions)
  ↑
  └─ [all systems: ai, combat, render, etc.]

world.h/.c
  ├─ components.h
  ├─ game_balance.h
  ↑
  └─ [everything else: game.c, all systems]

game.h/.c
  ├─ world.h
  ├─ game_balance.h
  ├─ equipment_bonus.h
  ├─ floor_init.h
  ├─ all systems
  ↑
  └─ main.c
```

**Rule**: Never add circular dependencies. All modules must be acyclic. If you find a cycle, refactor.

---

## Frequently Questioned Decisions

**Q: Why no `malloc` for dynamic arrays in the entity system?**  
A: Entity pool is fixed-size (pre-allocated in `ecs.h`). Predictable memory layout, no heap fragmentation, cache-friendly for iteration.

**Q: Why XOR-encrypt story/controls text?**  
A: Reduces binary size (~2KB saved) + obfuscates spoilers in hex dumps. Decryption is O(n) at startup only, no runtime cost.

**Q: Why not use a real test framework (CUnit, Check)?**  
A: Constraint: C99 only, no external dependencies. Custom harness in `tests/test_runner.c` is 100 lines and sufficient for current needs.

**Q: Why structure-of-arrays instead of entity-component (structs)?**  
A: Better cache locality for systems that iterate one component type at a time (AI loop, render loop). Reduces CPU cache misses.

**Q: Why Lua for build config, not CMake?**  
A: Premake5 is lightweight, cross-platform, and already in use. CMake would add complexity without benefit for this project size.

**Q: Why not use C++ for type safety?**  
A: Constraint: C99 only. Raylib is C, codebase is C. Adding C++ would require linker magic and complicate the build.

**Q: Why keep `inventory.c` unrefactored?**  
A: Scope decision: `inventory.c` was marked out-of-scope in recent refactoring (June 2026). It works; refactoring it would require large cascade of changes (old `game->` API vs new ECS). Future task.

**Q: How do we handle floor transitions without memory leaks?**  
A: `DescendFloor()` calls `Floor_InitNewFloor()` which resets component arrays (via `memset`) but keeps entity pool allocated. No dynamic allocation per floor.

**Q: What happens if we exhaust the entity pool (128 entities)?**  
A: `World_AllocateEntity()` logs warning and returns `ENTITY_NONE`. Caller must check. This is intentional (fail-fast). Increase `MAX_ENTITIES` in `ecs.h` if needed.

---

## Appendix: Version History

See `docs/VERSIONS.md` for Git tag mapping:

| Version | Tag | Date | Focus |
|---------|-----|------|-------|
| v0.0.9 | `v0.0.9` | 2026-06-03 | Bug fixes (7 critical: tile tearing, AI, magic, HP scaling, etc.) |
| v0.0.8 | `v0.0.8` | 2026-06-02 | Refactoring (formulas, equipment bonus, floor init, tests, profiling, API docs) |
| ALPHA-0.0.7 | (historical) | 2026-06-01 | Ranged combat + AI rebalance |
| ALPHA-0.0.6 | (historical) | (earlier) | Sprint + inventory system |
| ALPHA-0.0.5 | (historical) | (earlier) | Multi-floor dungeon + story |
| ALPHA-0.0.4 | (historical) | (earlier) | Music + tileset update |

See `changelog.md` for detailed feature lists.

---

**End of AGENTS.md**
