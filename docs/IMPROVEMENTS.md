# Potential Improvements & Refactors

**Heroes of Taboo** — v0.0.12 | June 5, 2026

---

## Table of Contents

1. [High Priority](#high-priority)
2. [Medium Priority](#medium-priority)
3. [Low Priority](#low-priority)
4. [Performance Optimizations](#performance-optimizations)
5. [Architecture Improvements](#architecture-improvements)
6. [Code Quality](#code-quality)
7. [Testing Improvements](#testing-improvements)
8. [Feature Ideas](#feature-ideas)

---

## High Priority

### 1. Implement Ability System

**Current State**: `ability_system.c` is a stub — both functions return false / do nothing.

**Impact**: The `CAbilities` component exists on the player and some monsters, MP is tracked, ability data is defined, but no abilities actually work. This is a major gameplay gap.

**Effort**: 4–6 hours

**Steps**:
1. Implement `AbilitySystem_Use()` for each `AbilityType`:
   - `ABILITY_PUNCH`: Low damage, no MP cost, no cooldown
   - `ABILITY_LIGHT_MELEE`: Medium damage, low MP cost
   - `ABILITY_HEAVY_MELEE`: High damage, high MP cost, longer cooldown
   - `ABILITY_RANGED`: Ranged attack using weapon ability
2. Implement `AbilitySystem_TickCooldowns()` — decrement cooldowns each turn
3. Add MP cost checking and deduction
4. Add ability selection UI (key binding or action menu)
5. Integrate with combat system for damage resolution
6. Add tests for each ability type

---

### 2. Split inventory.c **[DONE v0.0.12]**

**Current State**: `inventory.c` (341 lines) is a monolithic file containing:
- Static item data tables (names, heals, descriptions, sprites)
- Static equipment data table (30 entries)
- Potion add/use logic
- Equipment equip/unequip/drop logic
- Dual-wield checks
- Texture loading
- Weapon ability mapping

**Impact**: Hard to maintain, hard to test individual pieces, violates single responsibility.

**Effort**: 2–3 hours

**Proposed Split**:
| New File | Contents |
|----------|----------|
| `data/item_data.c/h` | `ITEM_NAMES[]`, `ITEM_HEALS[]`, `ITEM_DESCS[]`, `ITEM_SPRITES[]`, `GetItemName()`, `GetItemHealAmount()`, `GetItemDescription()` |
| `data/equip_table.c/h` | `EQUIP_TABLE[]`, `GetEquipData()`, `GetEquipRangeBonus()` |
| `inventory_logic.c/h` | `InventoryAdd()`, `InventoryUse()`, `LoadPotionTextures()`, `Inventory_LoadEquipTexture()`, `Inventory_LoadPotionTexture()` |
| `equipment_management.c/h` | `EquipItem()`, `EquipItemSilent()`, `UnequipSlot()`, `IsEquipSlotOccupied()`, `IsTwoHandedEquipped()`, `IsWeaponDualWieldable()`, `IsDualWielding()`, `AddEquipToInventory()`, `RemoveEquipFromInventory()` |

---

### 3. Split renderer.c **[DONE v0.0.12]**

**Current State**: `renderer.c` (516 lines) handles ALL rendering in a single `RenderGame()` function.

**Impact**: Hard to maintain, hard to test, growing larger with each feature.

**Effort**: 3–4 hours

**Proposed Split**:
| New File | Contents |
|----------|----------|
| `world_renderer.c/h` | Map tiles, pickups, monsters, player, shopkeeper, projectile, damage numbers, float messages (everything inside `BeginMode2D`) |
| `hud_renderer.c/h` | HP bar, EXP bar, level, floor info, gold, state text, combat hints, level-up overlay |
| `overlay_renderer.c/h` | Orchestrates inventory UI, shop UI, inspector, tile info panel |

---

### 4. Remove Static State from shop_ui.c **[DONE v0.0.12]**

**Current State**: `shop_ui.c` uses `static int s_selection` and `static ShopSection s_section`.

**Impact**: Violates the stateless systems rule. If multiple shops existed (future feature), state would be shared incorrectly.

**Effort**: 1 hour

**Fix**: Add `ShopUIState` struct to `GameWorld` or pass as parameter (like `InventoryUIState`):
```c
typedef struct {
    int selection;
    ShopSection section;
} ShopUIState;
```

---

## Medium Priority

### 5. Add Error Propagation

**Current State**: Most functions return `void` or `bool` without detailed error codes. `InitGame()` returns `false` on failure but doesn't distinguish between failure modes.

**Impact**: Hard to diagnose failures, especially in automated testing or CI.

**Effort**: 2–3 hours

**Proposed**: Add error enum:
```c
typedef enum {
    ERR_NONE = 0,
    ERR_MAP_LOAD_FAILED,
    ERR_MAP_GENERATE_FAILED,
    ERR_PLAYER_SPAWN_FAILED,
    ERR_TILESET_LOAD_FAILED,
    ERR_ENTITY_POOL_EXHAUSTED,
} GameError;

GameError InitGame(GameWorld* game, const char* tmxFile);
```

---

### 6. Add Save/Load System

**Current State**: No save/load — game state is lost on exit.

**Impact**: Players must complete the entire run in one session.

**Effort**: 4–6 hours

**Proposed**:
1. Serialize `GameWorld` to binary file (or JSON for debuggability)
2. Key fields to save: player stats, inventory, equipment, current floor, map seed
3. Add save/load UI to pause menu
4. Handle ECS entity reconstruction on load

---

### 7. Procedural Map Variety

**Current State**: `procedural.c` generates rooms connected by corridors. All floors look similar except for biome tile changes.

**Impact**: Repetitive dungeon layouts.

**Effort**: 6–8 hours

**Ideas**:
- Cave generation (cellular automata)
- Maze sections
- Large open rooms with pillars
- River/lava crossings
- Secret rooms (hidden behind breakable walls)
- Room templates (pre-designed rooms inserted randomly)

---

### 8. Monster Variety Behaviors

**Current State**: All monsters use the same AI decision tree (hunt/wander/attack). Different monster types only differ in stats and attack type.

**Impact**: Combat feels samey across monster types.

**Effort**: 4–6 hours

**Ideas**:
- **Goblin**: Flee when low HP, call for help
- **Skeleton**: Reassemble after death (revive once)
- **Dragon**: Breath attack (area damage), fly over obstacles
- **Shadow**: Teleport behind player, phase through walls
- **Bat**: Erratic movement, dodge bonus
- **Ogre**: Stomp attack (area knockback)

---

### 9. Add Crafting System

**Current State**: No crafting — items are found or bought.

**Impact**: Limited item progression agency.

**Effort**: 6–8 hours

**Proposed**:
1. Add `STATE_CRAFTING` game state
2. Add crafting recipes (combine items → new item)
3. Add crafting UI
4. Integrate with inventory system

---

## Low Priority

### 10. Add Minimap Enhancements

**Current State**: Minimap shows explored tiles and player position.

**Ideas**:
- Show monster positions (red dots)
- Show item positions (yellow dots)
- Show stair position (green dot)
- Show shop position (gold dot)
- Zoom levels
- Ping system

---

### 11. Add Difficulty Settings

**Current State**: Single difficulty — all players face the same challenge.

**Proposed**:
- Easy: +50% HP, -25% monster damage
- Normal: Current balance
- Hard: -25% HP, +25% monster damage, fewer potions
- Nightmare: Permadeath with no revive, double XP

---

### 12. Add Achievement System

**Proposed**:
- Track milestones (first kill, floor 5, floor 10, win, etc.)
- Store in local file
- Display notifications
- Add to stats tab in inventory

---

### 13. Add Tutorial

**Current State**: Controls screen lists key bindings but no interactive tutorial.

**Proposed**:
- Guided first floor with tutorial prompts
- Highlight relevant keys when context-appropriate
- Explain combat, inventory, and floor transitions

---

## Performance Optimizations

### 14. Render Filter **[DONE v0.0.12]**

**Current State**: `RenderSystem_DrawMonsters()` iterates ALL entities and checks visibility per entity.

**Proposed**: Pre-filter visible monsters into a cached list, updated only when:
- Monster spawns/dies
- Visibility changes (FOW update)
- Floor transition

**Effort**: ~15 lines of code
**Expected Improvement**: Skip invisible entities entirely during render loop

### 15. Animation Cache **[DONE v0.0.12]**

**Current State**: `World_UpdateMonsterAnimations()` iterates ALL entities every frame.

**Proposed**: Combine with render filter — only animate visible monsters.

**Effort**: ~5 lines of code
**Expected Improvement**: Skip animation updates for off-screen monsters

### 16. Texture Atlas **[DONE v0.0.12]**

**Current State**: Each equipment/potion/monster has its own texture file. Many small textures.

**Proposed**: Combine related sprites into atlas textures. Reduce draw calls and texture binds.

**Effort**: 4–6 hours (art + code)

### 17. Frustum Culling

**Current State**: All tiles in the map are iterated for rendering, even those outside the camera viewport.

**Proposed**: Calculate visible tile range from camera position and zoom, only render tiles within that range.

**Effort**: 1–2 hours
**Expected Improvement**: Significant for large maps (100×100 = 10,000 tiles, only ~100 visible at zoom 4)

---

## Architecture Improvements

### 18. Event System **[DONE v0.0.12]**

**Current State**: Systems directly call each other. Combat system calls `GainExperience()`, `SpatialHash_Remove()`, `DamageNumber_Spawn()`, etc.

**Proposed**: Add a simple event bus:
```c
typedef enum { EVT_MONSTER_KILLED, EVT_PLAYER_LEVEL_UP, EVT_ITEM_PICKED_UP, ... } EventType;
void EventBus_Publish(EventType type, void* data);
void EventBus_Subscribe(EventType type, void (*callback)(void* data));
```

**Benefits**: Decouples systems, makes it easier to add new reactions to events (achievements, analytics, sound effects).

### 19. Configuration System **[DONE v0.0.12]**

**Current State**: All tuning values are `#define` macros in `game_balance.h`. Changing values requires recompilation.

**Proposed**: Load balance values from a JSON/INI file at startup. Allows hot-reloading during development.

### 20. Entity Serialization

**Current State**: No way to save/load entity state.

**Proposed**: Add serialization functions:
```c
void Entity_Serialize(const World* w, EntityId e, FILE* fp);
EntityId Entity_Deserialize(World* w, FILE* fp);
```

This would enable save/load, networking, and entity prefab systems.

---

## Code Quality

### 21. Add Static Analysis

**Proposed**: Integrate `cppcheck` or `clang-tidy` into CI pipeline.

**Checks to enable**:
- Null pointer dereference
- Buffer overflow
- Uninitialized variables
- Memory leaks (for the few heap allocations)

### 22. Add Code Coverage

**Proposed**: Use `gcov`/`lcov` to measure test coverage.

**Target**: 80%+ line coverage for game logic (excluding rendering code).

### 23. Reduce Header Coupling

**Current State**: `game.h` includes many headers, creating a wide dependency fan-out.

**Proposed**: Use forward declarations more aggressively. Move includes from headers to `.c` files where possible.

### 24. Add Doxygen Comments

**Current State**: No formal documentation comments in source code.

**Proposed**: Add Doxygen-style comments to all public functions:
```c
/**
 * @brief Spawn a monster entity at the given position.
 * @param gw    Game world pointer (must not be NULL)
 * @param type  Monster type (must be < MONSTER_TYPE_COUNT)
 * @param x     Tile X coordinate
 * @param y     Tile Y coordinate
 * @param floor Current floor number (for stat scaling)
 * @return EntityId of spawned monster, or ENTITY_NONE on failure
 */
EntityId SpawnerSystem_SpawnMonster(GameWorld* gw, MonsterType type, int x, int y, int floor);
```

---

## Testing Improvements

### 25. Add Integration Tests

**Current State**: 28 unit tests covering formulas, data tables, and spatial hash. No integration tests.

**Proposed**: Add tests that exercise full gameplay scenarios:
- Complete combat flow (attack → damage → death → XP → level-up)
- Floor descent (save/restore player state)
- Equipment cycle (equip → unequip → re-equip)
- Inventory overflow (pickup when full → drop on floor)
- Full floor clear (kill all monsters → stairs appear)

### 26. Add Fuzz Testing

**Proposed**: Fuzz test input validation functions with random inputs:
```c
for (int i = 0; i < 10000; i++) {
    int slot = GetRandomValue(-100, 100);
    int count = GetRandomValue(-10, 30);
    // Should never crash
    Validate_InventorySlot(slot, count);
}
```

### 27. Add Performance Regression Tests

**Proposed**: Benchmark critical paths and fail if they regress:
```c
static bool test_ai_performance(void) {
    // Spawn 20 monsters
    clock_t start = clock();
    AISystem_ProcessAll(&gw, 0);
    clock_t end = clock();
    double ms = (double)(end - start) / CLOCKS_PER_SEC * 1000;
    CHECK(ms < 50.0, "AI processing under 50ms for 20 monsters");
}
```

---

## Feature Ideas

### 28. Procedural Story

Instead of fixed story text, generate story elements based on:
- Biome visited
- Monsters killed
- Equipment found
- Player choices

### 29. Boss Fights

Special monsters on floors 5 and 10:
- Unique mechanics (phases, special attacks)
- Boss health bar UI
- Guaranteed legendary loot

### 30. Multiplayer (Co-op)

Two players on the same map:
- Split screen or shared camera
- Turn-based with simultaneous input
- Shared inventory or individual

### 31. Meta-Progression

Persistent unlocks between runs:
- New starting equipment
- New character classes
- Unlockable biomes
- Bestiary (monster info unlocked by killing)

### 32. Modding Support

Allow players to create custom:
- Monster types (JSON definitions)
- Equipment items
- Biomes
- Maps (TMX format already supported)
