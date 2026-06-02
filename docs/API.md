# Public API Reference — Heroes of Taboo

All functions are C99-compatible, single-threaded, and non-reentrant unless noted.

---

## New Modules (Tasks 1–6)

### `game_balance.h` — Centralized Balance Constants & Formulas

**Include:** `#include "game_balance.h"`

#### Macros (read-only, compile-time constants)

| Macro | Value | Purpose |
|-------|-------|---------|
| `PROCEDURAL_MAP_WIDTH/HEIGHT` | 80 / 50 | Procedural dungeon tile dimensions |
| `DEFAULT_MAX_FLOORS` | 10 | Floors before escape tile appears |
| `DEFAULT_START_FLOOR` | 1 | Initial floor index |
| `PLAYER_BASE_STR/DEX/INT/CON/LCK` | 3/3/3/3/2 | Player base stats before equipment |
| `PLAYER_BASE_ATTACK/DEFENSE` | 5 / 1 | Player base attack/defense |
| `STAT_CAP_STR/DEX/INT/CON/LCK` | 99 | Stat point allocation caps |
| `STAT_POINTS_PER_LEVEL` | 2 | Stat points awarded on level-up |
| `HP_BASE_CONSTANT` / `HP_PER_CON` | 30 / 5 | maxHp = 30 + CON × 5 |
| `XP_BASE` / `XP_PER_LEVEL` | 50 / 40 | xpRequired = 50 + level × 40 |
| `STR_MELEE_MULT` | 2 | Melee: attack + STR × 2 |
| `DEX_RANGED_MULT` | 1.5 | Ranged: DEX × 1.5 |
| `DEX_THROW_MULT` | 2 | Throw: attack + DEX × 2 |
| `MAGIC_RESIST_MULT` | 3 | Magic resist: INT × 3 |
| `CRIT_MULT` / `MIN_DAMAGE` | 2 / 1 | Critical multiplier, minimum damage |
| `DODGE_PER_DEX` / `DODGE_CAP_PCT` | 2 / 60 | dodge% = DEX × 2, capped at 60% |
| `THROW_RANGE_BASE` / `THROW_RANGE_PER_DEX_DIV` | 3 / 3 | throw range = 3 + DEX/3 |
| `WAIT_HEAL_BASE` / `WAIT_HEAL_INTEL_DIV` | 1 / 2 | wait heal = 1 + INT/2 |
| `POTION_INT_SCALE` / `POTION_HEAL_DENOM` | 0.02 / 10000 | Potion heal formula parameters |
| `HIT_FLASH_DURATION` | 0.15f | Hit flash effect seconds |
| `DEFAULT_CAMERA_ZOOM` | 4.0f | Camera zoom on floor start |
| `UI_BASE_WIDTH/HEIGHT` | 1024/768 | UI scaling reference |
| `DEFAULT_SCROLL_STEP` | 20 | UI scroll pixels before scale |

#### Inline Functions

| Function | Returns | Contract |
|----------|---------|----------|
| `calc_xp_for_level(level)` | `int` | Total XP to reach level. level >= 1. |
| `calc_max_hp(con)` | `int` | Derived max HP from CON. |
| `calc_melee_damage(attack, str)` | `int` | Raw melee damage before defense. |
| `calc_ranged_damage(dex)` | `int` | Raw ranged damage before defense. |
| `calc_throw_damage(attack, dex)` | `int` | Raw throw damage before defense. |
| `calc_magic_damage(attack, intel)` | `int` | Raw magic damage before resist. |
| `calc_magic_resist(intel)` | `int` | Magic damage reduction. |
| `calc_damage_after_defense(damage, defense)` | `int` | Applies defense; result >= 1. |
| `calc_dodge_chance(dex)` | `int` | Dodge % (0–60). |
| `calc_throw_range(dex)` | `int` | Throw range in tiles (>= 3). |
| `calc_wait_heal(intel)` | `int` | HP healed on wait. |
| `calc_potion_heal(maxHp, pct, intel)` | `int` | Actual HP healed by potion. |

---

### `equipment_bonus.h` — Equipment Stat Bonus Management

**Include:** `#include "equipment_bonus.h"`

| Function | Side Effects | Preconditions |
|----------|-------------|---------------|
| `EquipmentBonus_Apply(World*, EntityId, EquipType)` | Adds item's stat bonuses to entity's CStats. Recalculates maxHp. | Safe no-op when `world == NULL`, `unit == ENTITY_NONE`, `type == EQUIP_NONE`, or entity lacks `COMP_STATS`. |
| `EquipmentBonus_Remove(World*, EntityId, EquipType)` | Subtracts item's stat bonuses from entity's CStats. Recalculates maxHp. | Same safe no-op guarantees. |
| `EquipmentBonus_Recalculate(World*, EntityId)` | Recalc maxHp from CON, clamp hp <= maxHp. | Safe no-op on NULL/ENTITY_NONE/no CStats. |

All three functions are **idempotent** — calling Apply twice with the same item doubles the bonus (caller is responsible for tracking equipped state).

---

### `floor_init.h` — Floor Setup

**Include:** `#include "floor_init.h"`

| Function | Side Effects | Preconditions |
|----------|-------------|---------------|
| `Floor_InitNewFloor(GameWorld*)` | Builds blocking map, inits monster templates, spawns entities/monsters/pickups, sets stair positions, resets turn/anim timers, clears visibility and runs FOV, sets camera zoom. | `game` must be non-NULL with a valid `game->map`. Player entity should already exist. |

---

### `validation.h` — Input Validation

**Include:** `#include "validation.h"`

| Function | Returns | Contract |
|----------|---------|----------|
| `Validate_InventorySlot(slot, slotCount)` | `bool` | slot in [0, slotCount) |
| `Validate_EquipType(type)` | `bool` | type in (EQUIP_NONE, EQUIP_COUNT) |
| `Validate_ItemType(type)` | `bool` | type in (ITEM_NONE, ITEM_COUNT) |
| `Validate_MonsterType(type)` | `bool` | type in [0, MONSTER_TYPE_COUNT) |
| `Validate_StatIndex(statIdx)` | `bool` | statIdx is 0–4 |
| `Validate_Floor(floor)` | `bool` | floor > 0 |
| `Clamp_Int(value, min, max)` | `int` | value clamped to [min, max] |

All validators are **pure functions** — no side effects, no logging, no state mutation. Callers decide how to handle invalid inputs.

---

## Existing Public API Summary

### `game.h`

| Function | Purpose |
|----------|---------|
| `InitGame(GameWorld*, const char* tmxFile)` | Initialize game world from TMX map. Returns false on failure. |
| `UpdateGame(GameWorld*)` | Process one frame of game logic. |
| `DescendFloor(GameWorld*)` | Transition to next floor; preserves player state. |
| `CleanupGame(GameWorld*)` | Free all resources; safe to call after InitGame. |
| `GetUIScale()` / `SetGuiScale(float)` / `GetGuiScale()` | UI scale management (1.0–4.0). |

### `inventory.h`

| Function | Purpose |
|----------|---------|
| `InventoryAdd(GameWorld*, ItemType)` | Add potion to inventory. Returns false if full. |
| `InventoryUse(GameWorld*, int slot)` | Consume potion at slot; applies healing. |
| `EquipItem(GameWorld*, EquipType)` | Equip item with combat log. Returns false on failure. |
| `EquipItemSilent(GameWorld*, EquipType)` | Equip without log (used at game start). |
| `UnequipSlot(GameWorld*, EquipSlot)` | Unequip and return item to inventory. |
| `IsEquipSlotOccupied(GameWorld*, EquipSlot)` | Check if slot has an item. |
| `IsTwoHandedEquipped(GameWorld*)` | Check if weapon slot holds two-handed. |
| `AddEquipToInventory(GameWorld*, EquipType)` | Store unequipped gear. |
| `RemoveEquipFromInventory(GameWorld*, int slot)` | Remove gear from inventory. |
| `GetEquipData(EquipType)` | Lookup EquipData by type (NULL if invalid). |
| `GetEquipRangeBonus(EquipType)` | Ranged weapon attack range. |
| `GetItemName(ItemType)` | Potion display name. |
| `GetItemHealAmount(ItemType)` | Potion heal percentage (25/50/75). |
| `GetItemDescription(ItemType)` | Potion description string. |

### `combat_system.h`

| Function | Purpose |
|----------|---------|
| `CombatSystem_PlayerMeleeAttack(GameWorld*, EntityId, int tx, int ty)` | Attack tile (tx,ty). |
| `CombatSystem_PlayerRangedAttack(GameWorld*, EntityId)` | Ranged attack in facing direction. |
| `CombatSystem_PlayerThrowWeapon(GameWorld*, EntityId)` | Throw equipped melee weapon. |

### World / ECS (`ecs.h`, `world.h`)

| Function | Purpose |
|----------|---------|
| `World_Init(World*)` | Initialize ECS world. |
| `World_CreateEntity(World*)` | Allocate entity; returns ENTITY_NONE if pool full. |
| `World_DestroyEntity(World*, EntityId)` | Mark entity dead, release to free list. |
| `World_HasComponents(World*, EntityId, ComponentMask)` | Check component mask. |
| `World_AddComponent` / `World_RemoveComponent` | Inline mask setters. |
| `World_GetPosition/Stats/Sprite/AI/Name/Pickup/HitFlash/Color` | Inline typed component accessors. |
| `GameWorld_Init(GameWorld*)` | Zero-init and ECS init. |

### Spawner (`spawner_system.h`)

| Function | Purpose |
|----------|---------|
| `SpawnerSystem_SpawnMonsters(GameWorld*, ProceduralRoom*, int count, int px, int py)` | Place monsters from room data. |
| `SpawnerSystem_SpawnPickups(GameWorld*)` | Place potions and equipment on map. |
| `SpawnerSystem_AddPotionAt/EquipAt` | Drop item at tile. |
| `SpawnerSystem_CollectPickupsAt/EquipAt` | Pick up items at tile. |

### AI (`ai_system.h`)

| Function | Purpose |
|----------|---------|
| `AISystem_ProcessAll(GameWorld*, int timeWaited)` | Process one turn of monster AI. Returns false if player died. |

---

## Thread Safety

All functions are **single-threaded** — the game runs on one thread. No function is safe to call from a signal handler or separate thread without external synchronization.

## Preconditions & Error Handling

- Functions with pointer parameters check for NULL and return/are no-ops.
- `EntityId` values must be valid (from `World_CreateEntity`) or `ENTITY_NONE`.
- Callers own the `GameWorld` lifetime — call `CleanupGame` before freeing.
- Equipment/Inventory functions assume the caller manages slot indices correctly; use `validation.h` to check bounds at API boundaries.
