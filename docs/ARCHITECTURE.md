# Architecture & Design Patterns

**Heroes of Taboo** — v0.0.10 | June 5, 2026

---

## Table of Contents

1. [Architectural Overview](#architectural-overview)
2. [Entity-Component-System (ECS)](#entity-component-system-ecs)
3. [Design Patterns in Use](#design-patterns-in-use)
4. [Layered Architecture](#layered-architecture)
5. [State Management](#state-management)
6. [Memory Model](#memory-model)
7. [Performance Architecture](#performance-architecture)
8. [Dependency Rules](#dependency-rules)
9. [Anti-Patterns & Known Debt](#anti-patterns--known-debt)

---

## Architectural Overview

Heroes of Taboo follows a **layered ECS architecture** with three distinct tiers:

```
┌─────────────────────────────────────────────────────┐
│                    Application Layer                  │
│  main.c → scene dispatcher → game.c → systems       │
├─────────────────────────────────────────────────────┤
│                     Game Logic Layer                  │
│  systems/ (AI, combat, input, movement, render)      │
│  data/ (monsters, loot, equipment, biomes)           │
│  map/ (procedural, TMX, FOW)                        │
│  ui/ (menu, inventory, shop, inspector, minimap)     │
├─────────────────────────────────────────────────────┤
│                    Engine Layer                       │
│  ecs.h/c (entity pool, components, accessors)        │
│  resources.h/c (texture cache)                       │
│  audio.h/c (music contexts, sound categories)        │
└─────────────────────────────────────────────────────┘
```

Each layer depends only on layers below it. The engine layer has zero game-specific knowledge.

---

## Entity-Component-System (ECS)

### Pattern: Structure-of-Arrays with Bitmask Ownership

The ECS uses a **fixed-size pool** (128 entities) with **parallel component arrays** and a **bitmask** per entity tracking which components are attached.

```
Entity 0: ENTITY_NONE (reserved sentinel — never allocated)
Entity 1: player  → mask = COMP_POSITION | COMP_STATS | COMP_SPRITE_ANIM | ...
Entity 2: goblin  → mask = COMP_POSITION | COMP_STATS | COMP_AI | COMP_SPRITE_ANIM | ...
Entity 3: pickup  → mask = COMP_POSITION | COMP_PICKUP | COMP_NAME
```

### Why Structure-of-Arrays?

- **Cache locality**: Systems that iterate one component type (e.g., AI loop reading only `CAI` and `CPosition`) benefit from contiguous memory layout
- **No heap allocation**: All arrays are pre-allocated in the `World` struct — zero fragmentation
- **O(1) access**: Component lookup by entity ID is a direct array index, not a hash map lookup

### Component Bitmask System

Each component type is assigned a single bit in a `uint32_t` mask:

```c
#define COMP_POSITION       (1u << 0)
#define COMP_STATS          (1u << 1)
#define COMP_SPRITE_ANIM    (1u << 2)
#define COMP_FALLBACK_COLOR (1u << 3)
#define COMP_AI             (1u << 4)
#define COMP_PICKUP         (1u << 5)
#define COMP_NAME           (1u << 6)
#define COMP_PLAYER_TAG     (1u << 7)
#define COMP_HIT_FLASH      (1u << 8)
#define COMP_ABILITIES      (1u << 9)
```

Checking multiple components is a single bitwise AND:

```c
uint32_t required = COMP_POSITION | COMP_STATS | COMP_AI;
if ((world->masks[e] & required) == required) { /* has all three */ }
```

### Entity Lifecycle

```
World_CreateEntity()
  → Allocates from free list or increments count
  → Sets alive[e] = true, masks[e] = 0
  → Returns EntityId

World_AddComponent(w, e, COMP_STATS)
  → Sets bit: masks[e] |= COMP_STATS
  → Caller fills component data via World_GetStats(w, e)

World_DestroyEntity(w, e)
  → Zeroes all component arrays for slot e
  → Clears mask, sets alive[e] = false
  → Pushes e onto free list for reuse
```

### Component Accessors

All component access goes through typed accessor functions that include debug assertions:

```c
CStats* World_GetStats(World* w, EntityId e);
// In DEBUG builds: asserts e != ENTITY_NONE, e < count, alive[e], mask has COMP_STATS
// In release builds: no-op assertion, direct array return
```

### The 10 Component Types

| Component | Bit | Purpose | Used By |
|-----------|-----|---------|---------|
| `CPosition` | 0 | Grid coordinates + facing direction | All entities |
| `CStats` | 1 | HP, attack, defense, stats, exp | Player, monsters |
| `CSpriteAnim` | 2 | Sprite sheet animation state | Player, monsters |
| `CFallbackColor` | 3 | Solid color when no sprite | Player fallback |
| `CAI` | 4 | Monster behavior state | Monsters only |
| `CPickup` | 5 | Ground item data | Loot entities |
| `CName` | 6 | Display name (32 chars) | Player, monsters |
| `COMP_PLAYER_TAG` | 7 | Identifies the player entity | Player only |
| `CHitFlash` | 8 | White flash timer on hit | Player, monsters |
| `CAbilities` | 9 | Ability slots + MP + cooldowns | Player, some monsters |

---

## Design Patterns in Use

### 1. Stateless Systems

Every system is a collection of **pure functions** that take a `GameWorld*` and produce side effects only through that pointer. No `static` local variables exist in system `.c` files.

```c
// GOOD: stateless — all state lives in GameWorld
void AISystem_ProcessAll(GameWorld* gw, int timeWaited) { ... }

// BAD: static state would break floor transitions
static int turnCounter = 0;  // NEVER DO THIS
```

**Rationale**: Floor transitions call `memset(game, 0, sizeof(GameWorld))` to reset all state. Static variables would survive this reset and cause stale data bugs.

### 2. Centralized Constants (game_balance.h)

All magic numbers, formulas, and tuning values live in a single header file. This prevents scattered literals and makes balance changes trivial.

```c
// Instead of: damage = attack + str * 2;  (what does 2 mean?)
// Use:        damage = calc_melee_damage(attack, str);
// Where:      calc_melee_damage uses STR_MELEE_MULT (defined as 2)
```

**12 inline formula functions** encapsulate all damage/XP/healing calculations.

### 3. Idempotent Operations

`EquipmentBonus_Apply()` and `EquipmentBonus_Remove()` are designed to be safe when called multiple times or on entities without the required components:

```c
void EquipmentBonus_Apply(World* w, EntityId unit, EquipType type) {
    if (!w || unit == ENTITY_NONE || type == EQUIP_NONE) return;  // safe no-op
    if (!World_HasComponents(w, unit, COMP_STATS)) return;        // safe no-op
    // ... apply bonuses ...
    EquipmentBonus_Recalculate(w, unit);  // always recalculate derived stats
}
```

### 4. Spatial Hash Grid (O(1) Position Lookup)

Monster positions are tracked in a 2D grid (`monsterGrid[MAP_HEIGHT][MAP_WIDTH]`) for instant tile-to-entity lookup:

```
monsterGrid[y][x] = EntityId  (or ENTITY_NONE if empty)
```

**Invalidation points** (must be kept in sync):
- Spawn → `SpatialHash_Add()`
- Death → `SpatialHash_Remove()`
- Move → `SpatialHash_Move()`
- Floor transition → `GameWorld_Init()` zeroes the entire struct

### 5. Cached Counter (O(1) Alive Count)

Instead of scanning all entities to count alive monsters, a counter is maintained:

```c
gw->aliveMonsterCount++;  // on spawn
gw->aliveMonsterCount--;  // on death
// GameWorld_Init() zeroes it on floor transition
```

### 6. Resource Manager (Singleton Cache)

All textures are loaded through a single entry point that caches by path:

```c
Texture2D* Resources_LoadTexture(const char* path);
// Returns pointer to cached texture, or loads + caches on first call
// Max 64 cached textures
```

### 7. Scene Dispatcher (main.c)

The main loop uses a simple enum-based scene state machine:

```c
typedef enum {
    SCENE_MENU, SCENE_SETTINGS, SCENE_CONTROLS,
    SCENE_STORY, SCENE_CREDITS, SCENE_GAME, SCENE_EXIT
} Scene;
```

Each scene handles its own update/render in a switch statement.

### 8. Turn-Based State Machine

Game logic uses a `GameState` enum to control the turn flow:

```
STATE_PLAYER_TURN → (player acts) → STATE_ENEMY_TURN
STATE_ENEMY_TURN  → (AI processes) → STATE_PLAYER_TURN
                  → (player dies)  → STATE_GAME_OVER
                  → (all dead)     → floor clear notification
                  → (max floors)   → escape tile spawns
```

Additional states: `STATE_INVENTORY`, `STATE_MAP`, `STATE_SHOP`, `STATE_WIN`.

### 9. Observer-Lite (FloatMsg / DamageNumber Pools)

Instead of a full observer pattern, the game uses **fixed-size pools** of visual effect objects that systems spawn and the renderer consumes:

```c
DamageNumber_Spawn(&gw->damageNumbers, value, tileX, tileY, tw, th, RED);
FloatMsg_Spawn(gw, tileX, tileY, GOLD, "Level %d!", level);
```

The renderer iterates active entries each frame. Expired entries are deactivated.

### 10. Data-Driven Monster Templates

Monster definitions are stored in a static table (`MonsterTemplate[]`) initialized at startup. The spawner reads templates to create ECS entities with floor-scaled stats.

---

## Layered Architecture

### Layer 0: Engine (engine/)

Pure infrastructure with no game knowledge:

| Module | Responsibility |
|--------|---------------|
| `ecs.h/c` | Entity pool, component bitmask, typed accessors |
| `resources.h/c` | Texture cache (path-keyed, 64 slots) |
| `audio.h/c` | Music context switching, sound categories, volume |

### Layer 1: Shared Types (game/src/game_types.h, components.h)

Game-wide type definitions that multiple layers depend on:

| Module | Responsibility |
|--------|---------------|
| `game_types.h` | Enums (AttackType, Direction, MonsterType, ItemType, EquipType, GameState), Projectile struct |
| `components.h` | All 10 ECS component structs + bitmask defines |
| `game_balance.h` | All constants, formulas, inline calculation functions |

### Layer 2: Data Tables (game/src/data/)

Static, read-only data that systems consume:

| Module | Responsibility |
|--------|---------------|
| `monster_data` | 11 monster templates with stat scaling, CR system |
| `loot_data` | Loot table with 4 tiers and rarity weights |
| `equip_data` | Equipment power scores and pricing |
| `biome_data` | Biome definitions and floor selection |
| `ability_data` | Ability metadata (name, MP cost, cooldown) |

### Layer 3: Systems (game/src/systems/)

Stateless processing units that operate on ECS entities:

| System | Input | Output |
|--------|-------|--------|
| `input_system` | Keyboard state | Player actions (move, attack, use item) |
| `movement_system` | Direction + walkability | Position updates, FOW reveal, pickup collection |
| `combat_system` | Attacker + target | Damage, dodge, crit, death, XP, loot drops |
| `ai_system` | Monster entities + player position | Monster movement, attacks, ability use |
| `render_system` | All visible entities | Map tiles, monsters, player drawn to screen |
| `spawner_system` | Floor number + rooms | Monster entities, pickup entities |
| `player_system` | GameWorld | Player entity creation |
| `ability_system` | Caster + target + ability type | Ability effects (stub — not yet implemented) |
| `spatial_hash` | Entity positions | Grid occupancy updates |
| `world_monster` | GameWorld | Monster queries, animation updates |

### Layer 4: Game Logic (game/src/)

Orchestration and state management:

| Module | Responsibility |
|--------|---------------|
| `game.c` | InitGame, UpdateGame, DescendFloor, CleanupGame |
| `world.c` | GameWorld_Init, DamageNumber/FloatMsg pools |
| `inventory.c` | Potion/equipment data tables, equip/unequip logic |
| `equipment_bonus.c` | Stat apply/remove/recalculate |
| `floor_init.c` | Shared floor setup (blocking map, spawning, FOW) |
| `validation.c` | Pure input validation predicates |

### Layer 5: Presentation (game/src/ui/, game/src/systems/renderer.c)

All rendering and UI:

| Module | Responsibility |
|--------|---------------|
| `renderer.c` | Master render orchestrator (calls all sub-renderers) |
| `render_system.c` | Map tiles, monsters, FOW overlay |
| `menu.c` | Main menu, settings, story, controls, credits, pause menu |
| `inventory_ui.c` | Inventory overlay with 3 tabs |
| `shop_ui.c` | Shop buy/sell interface |
| `inspector.c` | Monster/item info panel |
| `map_ui.c` | Minimap overlay |
| `text_data.c` | XOR-encrypted story/controls text |

---

## State Management

### GameWorld: The God Object

`GameWorld` is the single source of truth for all game state. It contains:

```
GameWorld
├── ecs: World                    // ECS entity pool + component arrays
├── playerEntity: EntityId        // Cached player entity ID
├── monsterGrid[100][100]         // Spatial hash (tile → entity)
├── aliveMonsterCount: int        // Cached alive monster count
├── gold: int                     // Player gold
├── statCapsRemoved: bool         // Band of Growth flag
├── currentBiome: BiomeType       // Active biome for this floor
├── shopkeeperEntity: EntityId    // Shop NPC entity
├── damageNumbers: DamageNumberPool  // Floating damage text pool
├── floatMsgs: FloatMsgPool       // Floating status message pool
├── map: MapData*                 // Current map (TMX or procedural)
├── tilesetTextures[8]            // Loaded tileset textures
├── state: GameState              // Turn-based state machine
├── turnCount, timeWaited         // Turn tracking
├── currentFloor, maxFloors       // Dungeon progress
├── stairX/Y, escapeX/Y          // Exit tile positions
├── animTimer, monsterAnimTimer   // Animation interpolation
├── projectile: Projectile        // Active projectile state
├── visibility[100][100]          // FOW state (0=hidden, 1=seen, 2=explored)
├── blocking[100][100]            // Walkability map
├── camera: Camera2D              // Viewport camera
├── levelUpTimer: float           // Level-up flash duration
├── selectedMonsterEntity         // Inspector target
├── inventory[16]                 // Potion slots
├── equipped[5]                   // Equipment slots
├── equipInventory[16]            // Equipment bag
└── mapOpen: bool                 // Minimap toggle
```

### Floor Transition Protocol

When the player descends to a new floor:

```
1. Save player state (position, stats, sprite, hit flash)
2. Save inventory, equipment, equipped items
3. Unload current map (UnloadTMX)
4. memset(game, 0, sizeof(GameWorld))  ← resets EVERYTHING
5. GameWorld_Init(game)                ← reinitializes ECS, zeroes grid
6. Restore player state
7. Re-apply equipment bonuses
8. Generate new procedural map
9. Load tilesets
10. Floor_InitNewFloor(game)           ← spawn monsters, pickups, FOW, camera
```

This protocol ensures no stale data survives between floors.

---

## Memory Model

### No Dynamic Allocation (Almost)

| Allocation | Strategy |
|-----------|----------|
| Entity pool | Fixed array: `World.positions[128]`, `World.stats[128]`, etc. |
| Component masks | Fixed array: `World.masks[128]` |
| Spatial hash | Fixed array: `GameWorld.monsterGrid[100][100]` |
| FOW/blocking | Fixed arrays: `GameWorld.visibility[100][100]`, `blocking[100][100]` |
| Damage numbers | Fixed pool: `DamageNumberPool.entries[32]` |
| Float messages | Fixed pool: `FloatMsgPool.entries[16]` |
| Texture cache | Fixed array: `CachedTexture s_cache[64]` |
| Map tile data | **Heap allocated** via `malloc` in `LoadTMX()` / `GenerateProceduralMap()` |
| Inventory | Fixed arrays in `GameWorld` |

The only heap allocations are:
1. `MapData` struct and its `TileLayer.data` arrays (freed via `UnloadTMX()`)
2. Temporary allocations in tests (`calloc` for test map data)

### Entity Pool Recycling

Destroyed entities are pushed onto a free list. `World_CreateEntity()` pops from the free list first, then falls back to incrementing the count. This prevents pool exhaustion during long play sessions.

---

## Performance Architecture

### Optimized Hot Paths

| Operation | Before | After | Technique |
|-----------|--------|-------|-----------|
| Find monster at tile | O(n) scan | O(1) lookup | Spatial hash grid |
| Count alive monsters | O(n) scan | O(1) read | Cached counter |
| AI processing | O(n²) pointer chasing | O(n) batched | Pre-fetched pointers |

### Render Pipeline

```
BeginMode2D(camera)
  → RenderSystem_DrawMap()         // Tile layers with FOW
  → Draw pickups (ECS iteration)   // Potions + equipment on ground
  → RenderSystem_DrawMonsters()    // ECS monsters with interpolation
  → Draw player                    // Sprite or fallback color
  → Draw shopkeeper                // Gold rectangle + label
  → Draw projectile                // Arrow/spell/throw animation
  → Draw damage numbers            // Floating text (world-space)
  → Draw float messages            // Status text (world-space)
EndMode2D()

// Screen-space HUD:
  → HP bar, EXP bar, floor info, gold
  → Inspector panel (top-right)
  → Tile info panel (if active)
  → State text (bottom-center)
  → Combat hints (bottom-left)
  → Level-up overlay (if active)
  → Inventory overlay (if open)
  → Shop overlay (if open)
```

---

## Dependency Rules

### Acyclic Dependency Graph

All module dependencies form a **directed acyclic graph (DAG)**. No circular dependencies exist.

```
game_balance.h (LEAF)
  ↑
  ├─ combat_system.c
  ├─ player.c
  ├─ spawner_system.c
  └─ monster_data.c

components.h (LEAF)
  ↑
  └─ ecs.h → world.h → [everything]

game_types.h (LEAF)
  ↑
  ├─ components.h
  ├─ inventory.h
  ├─ monster_data.h
  └─ validation.h
```

### Include Direction Rules

1. **Systems include `world.h`** — never include other system headers directly unless calling their functions
2. **Data modules include `game_types.h`** — never include system headers
3. **UI modules include `world.h`** — read-only access to game state for rendering
4. **`game.h` includes everything** — it's the orchestration layer
5. **`main.c` includes `game.h` + UI headers** — scene dispatching

---

## Anti-Patterns & Known Debt

### 1. inventory.c Is Monolithic

`inventory.c` (341 lines) contains both data tables AND logic. It mixes:
- Static equipment table (`EQUIP_TABLE[30]`)
- Potion data arrays
- Inventory add/use logic
- Equipment equip/unequip logic
- Texture loading

**Ideal**: Split into `item_data.c`, `equip_table.c`, `inventory_logic.c`, `equipment_management.c`.

### 2. renderer.c Is a God Function

`renderer.c` (516 lines) handles ALL rendering in a single `RenderGame()` function:
- Map tiles, pickups, player, shopkeeper, projectiles
- Damage numbers, float messages
- HUD (HP bar, EXP bar, floor info, gold)
- Inspector panel, tile info panel
- Level-up overlay, combat hints
- Inventory and shop overlays

**Ideal**: Split into `hud_renderer.c`, `world_renderer.c`, `overlay_renderer.c`.

### 3. shop_ui.c Uses Static State

`shop_ui.c` has `static int s_selection` and `static ShopSection s_section` — violating the stateless systems rule. This works because there's only one shop at a time, but it's inconsistent with the rest of the architecture.

### 4. procedural.c Uses Static State

`procedural.c` has `static ProceduralRoom s_generatedRooms[]`, `static int s_generatedRoomCount`, `static BiomeType s_currentBiome`, and `static int s_stairX/Y`. These are accessed via getter functions. This is a pragmatic choice since procedural generation is a one-shot operation per floor.

### 5. game_audio.c Uses Static State

`game_audio.c` stores registered audio contexts and categories in static variables. This is acceptable because audio initialization happens once at startup and contexts are immutable.

### 6. text_data.c Uses Static Arrays

`text_data.c` contains large static byte arrays for XOR-encrypted story/controls/credits text. This is intentional for binary size reduction.

### 7. No Error Propagation

Most functions return `void` or `bool` without detailed error codes. `InitGame()` returns `false` on failure but doesn't distinguish between "TMX load failed" and "map generation failed" vs "player spawn failed".
