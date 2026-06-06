# Core Systems & Interactions

**Heroes of Taboo** — v0.0.10 | June 5, 2026

---

## Table of Contents

1. [System Overview](#system-overview)
2. [System Execution Order](#system-execution-order)
3. [Input System](#input-system)
4. [Movement System](#movement-system)
5. [Combat System](#combat-system)
6. [AI System](#ai-system)
7. [Spawner System](#spawner-system)
8. [Render Pipeline](#render-pipeline)
9. [Player System](#player-system)
10. [Spatial Hash](#spatial-hash)
11. [World Monster Queries](#world-monster-queries)
12. [Ability System](#ability-system)
13. [System Interaction Matrix](#system-interaction-matrix)
14. [Shared State Access Patterns](#shared-state-access-patterns)

---

## System Overview

The game is composed of **stateless systems** that operate on the `GameWorld` struct. Each system is a collection of functions that read and mutate ECS components and GameWorld fields through a single `GameWorld*` parameter.

| System | File | Trigger | Frequency |
|--------|------|---------|-----------|
| Input | `input_system.c` | Player turn | Once per player action |
| Movement | `movement_system.c` | Player move key | Once per movement |
| Combat | `combat_system.c` | Attack/throw/ranged | Once per attack |
| AI | `ai_system.c` | Enemy turn | Once per enemy turn |
| Spawner | `spawner_system.c` | Floor init | Once per floor |
| Render | `render_system.c` + `renderer.c` | Every frame | 60 FPS |
| Player | `player_system.c` + `player.c` | Game init / level-up | On spawn / on XP gain |
| Spatial Hash | `spatial_hash.c` | Spawn/move/death | On position change |
| World Monster | `world_monster.c` | Queries / animation | On demand / every frame |
| Ability | `ability_system.c` | (stub) | Not yet active |
| Atlas | `atlas.c` | Startup | Once per session |
| Event Bus | `event_bus.c` | At publish points | On event |
| Config | `config.c` | Startup / F3 | Once per session / on demand |
| Debug Log | `debug_log.c` | At log points | On every log call |
| Crash Handler | `debug_log.c` | On unhandled exception | On crash |

---

## System Execution Order

### During Player Turn

```
main loop (60 FPS)
  │
  ├─ InputSystem_Inventory()      ← if STATE_INVENTORY
  │    └─ Handles tab switching, item selection, equip/use, stat allocation
  │
  ├─ InputSystem_PlayerTurn()     ← if STATE_PLAYER_TURN
  │    ├─ Read keyboard
  │    ├─ Movement keys → MovementSystem_PlayerMove()
  │    │    ├─ MovementSystem_IsWalkable()
  │    │    ├─ Update CPosition
  │    │    ├─ RevealFOW()
  │    │    ├─ SpawnerSystem_ListPotionsAt() → InventoryAdd()
  │    │    ├─ SpawnerSystem_CollectEquipAt() → AddEquipToInventory()
  │    │    └─ state = STATE_ENEMY_TURN
  │    │
  │    ├─ F key → CombatSystem_PlayerMeleeAttack()
  │    │    ├─ World_FindMonsterAt() (spatial hash)
  │    │    ├─ Damage calculation (game_balance.h formulas)
  │    │    ├─ SpatialHash_Remove() on death
  │    │    ├─ GainExperience() → AllocateStatPoint()
  │    │    └─ state = STATE_ENEMY_TURN
  │    │
  │    ├─ T key → CombatSystem_PlayerThrowWeapon()
  │    ├─ Space → wait turn (heal + shadow check)
  │    ├─ I key → state = STATE_INVENTORY
  │    ├─ M key → state = STATE_MAP
  │    └─ E key → interact / inspect
  │
  └─ UpdateGame()
       ├─ DamageNumber_UpdateAll()
       ├─ FloatMsg_UpdateAll()
       ├─ Decrement animation timers
       ├─ Update hit flash timers
       ├─ Update player sprite animation
       ├─ World_UpdateMonsterAnimations()
       └─ If STATE_ENEMY_TURN → process enemy turn (see below)
```

### During Enemy Turn

```
UpdateGame() → state == STATE_ENEMY_TURN
  │
  ├─ Wait for enemyTurnCooldown (0.15s)
  │
  ├─ If currently animating:
  │    ├─ Wait for projectile to finish
  │    ├─ Wait for monster animation to finish
  │    └─ Done → state = STATE_PLAYER_TURN
  │
  ├─ AISystem_ProcessAll(game, timeWaited)
  │    │
  │    ├─ Batch-fetch all monster pointers (optimization)
  │    │
  │    ├─ For each alive monster:
  │    │    ├─ AI_GetActiveAbility() → determine attack type
  │    │    ├─ Calculate distance to player
  │    │    ├─ Check line of sight (Bresenham through blocking map)
  │    │    │
  │    │    ├─ DECISION:
  │    │    │    ├─ In attack range + LOS → attack
  │    │    │    │    ├─ Melee: move adjacent, deal damage
  │    │    │    │    ├─ Ranged: fire projectile
  │    │    │    │    └─ Magic: fire magic projectile
  │    │    │    │
  │    │    │    ├─ In detection range + LOS → hunt
  │    │    │    │    ├─ Record lastSeenX/Y
  │    │    │    │    ├─ MoveToward() → greedy pathfinding
  │    │    │    │    └─ SpatialHash_Move()
  │    │    │    │
  │    │    │    ├─ huntTurns > 0 → continue to lastSeen
  │    │    │    │    └─ Decrement huntTurns
  │    │    │    │
  │    │    │    └─ Else → wander randomly
  │    │    │         └─ Pick random adjacent walkable tile
  │    │    │
  │    │    └─ Shadow special: double-move after timeWaited threshold
  │    │
  │    └─ Return false if player died
  │
  ├─ Check floor clear (all monsters dead)
  │    ├─ Floor >= maxFloors → SpawnEscapeTile()
  │    └─ Floor < maxFloors → "Floor Clear!" message
  │
  └─ Set animation timers, transition to STATE_PLAYER_TURN
```

---

## Input System

**File**: `game/src/systems/input_system.c` (429 lines)

### Responsibilities
- Read keyboard state during player turn and inventory state
- Translate key presses into game actions
- Manage inventory UI navigation

### Key Bindings (Player Turn)

| Key | Action | System Called |
|-----|--------|--------------|
| W/↑ | Move up | `MovementSystem_PlayerMove(DIR_UP)` |
| S/↓ | Move down | `MovementSystem_PlayerMove(DIR_DOWN)` |
| A/← | Move left | `MovementSystem_PlayerMove(DIR_LEFT)` |
| D/→ | Move right | `MovementSystem_PlayerMove(DIR_RIGHT)` |
| F | Attack facing direction | `CombatSystem_PlayerMeleeAttack()` |
| T | Throw weapon | `CombatSystem_PlayerThrowWeapon()` |
| Space | Wait (heal) | Direct: heal + shadow check + enemy turn |
| I | Open inventory | `state = STATE_INVENTORY` |
| M / Z | Open minimap | `state = STATE_MAP` |
| E | Interact/inspect | Tile inspection or shop entry |

### Key Bindings (Inventory)

| Key | Action |
|-----|--------|
| I / ESC | Close inventory |
| Q / E | Switch tab (left/right) |
| W / S | Navigate items |
| Enter | Open action menu / confirm |
| ESC | Back from action menu |
| Mouse wheel | Scroll inventory list |
| A / D | Switch stats columns |
| Enter (stats) | Allocate stat point |

### Dependencies
- `game.h` (FOG_RADIUS, game state)
- `game_audio.h` (play sounds on actions)
- `map_helpers.h` (RevealFOW)
- `inventory.h` (equip/unequip/use)
- `equipment_bonus.h` (stat recalculation)
- `spawner_system.h` (pickup collection)
- `ai_system.h` (ability queries)
- `combat_system.h` (attack functions)
- `world_monster.h` (monster queries)
- `player.h` (stat allocation)
- `movement_system.h` (movement)

---

## Movement System

**File**: `game/src/systems/movement_system.c` (126 lines)

### Responsibilities
- Validate tile walkability
- Execute player movement with side effects
- Collect pickups on arrival
- Check stairs/escape conditions

### Walkability Check

```
MovementSystem_IsWalkable(gw, x, y):
  1. In bounds? (0 <= x < map.width, 0 <= y < map.height)
  2. Not blocked? (blocking[y][x] == 0)
  3. No monster? (World_FindMonsterAt(x, y) == ENTITY_NONE)
```

### Movement Side Effects

When the player successfully moves to a new tile:

1. **Position update**: Save prevX/prevY, set new x/y, set facingDir
2. **Turn tracking**: Increment turnCount, set enemyTurnCooldown
3. **Animation**: Set animTimer = MOVE_ANIM_DURATION
4. **FOW**: Call RevealFOW() to update visibility
5. **State transition**: Set state = STATE_ENEMY_TURN
6. **Pickup collection**: List and collect potions/equipment at new tile
7. **Stairs check**: If on stair tile → DescendFloor()
8. **Escape check**: If on escape tile → state = STATE_WIN

### Bump Attack

If the target tile has a monster (not walkable due to monster), the input system calls `CombatSystem_PlayerMeleeAttack()` instead of moving.

---

## Combat System

**File**: `game/src/systems/combat_system.c` (427 lines)

### Responsibilities
- Resolve melee, ranged, and throw attacks
- Calculate damage using `game_balance.h` formulas
- Handle dodge, critical hits, mega-crits
- Manage dual-wield off-hand follow-up
- Handle monster death (XP, gold, loot drops, spatial hash cleanup)
- Create projectile animations for ranged/throw

### Damage Pipeline

```
Raw Damage → Defense Reduction → Dodge Check → Crit Check → Mega-Crit Check → Apply
```

1. **Raw damage**: `calc_melee_damage(attack, str)` or `calc_ranged_damage(dex)` or `calc_throw_damage(attack, dex)`
2. **Defense**: `calc_damage_after_defense(raw, defense)` → `max(raw - defense, 1)`
3. **Dodge**: `calc_dodge_chance(target.dex)` → percentage roll → if dodged, no damage
4. **Crit**: `attacker.lck` → percentage roll → if crit, `damage *= CRIT_MULT (2)`
5. **Mega-crit**: If `damage >= 100` → 50% chance → `damage *= 2`
6. **Apply**: `target.hp -= damage`

### Death Handling

When a monster's HP reaches 0:

1. Set `CStats.alive = false`
2. `SpatialHash_Remove()` from grid
3. Decrement `aliveMonsterCount`
4. `GainExperience()` → may trigger level-up
5. Add gold (base + rarity bonus from equipment)
6. MP bonus on kill (`MP_BONUS_ON_KILL`)
7. `DropMonsterEquipment()` → random chance from template
8. Spawn death FloatMsg

### Dual-Wield Follow-Up

After a successful melee attack, if `IsDualWielding()`:
1. Calculate off-hand damage: `raw * DUAL_WIELD_OFFHAND_MULT (0.5)`
2. Apply defense reduction
3. Apply to same target
4. Spawn additional DamageNumber

---

## AI System

**File**: `game/src/systems/ai_system.c` (432 lines)

### Responsibilities
- Process all monster AI decisions each enemy turn
- Implement hunt, wander, flank, kite, and ranged behaviors
- Handle shadow monster special double-move
- Execute monster attacks (melee, ranged, magic)

### AI Decision Tree

```
For each alive monster with COMP_AI:
  │
  ├─ Calculate Manhattan distance to player
  ├─ Check line of sight (Bresenham through blocking map)
  │
  ├─ IF distance <= attackRange AND has LOS:
  │    └─ ATTACK
  │         ├─ Melee: move to adjacent tile, deal damage
  │         ├─ Ranged: fire projectile (arrow/spore/rock)
  │         └─ Magic: fire magic projectile (wave animation)
  │
  ├─ IF distance <= detectionRange AND has LOS:
  │    └─ HUNT
  │         ├─ Record lastSeenX/Y = player position
  │         ├─ Set huntTurns = AI_HUNT_TURNS (4)
  │         └─ MoveToward(player) → greedy pathfinding
  │
  ├─ IF huntTurns > 0:
  │    └─ CONTINUE HUNT
  │         ├─ MoveToward(lastSeenX/Y)
  │         └─ Decrement huntTurns
  │
  ├─ IF distance <= detectionRange + AI_WANDER_RANGE_EXTRA:
  │    └─ WANDER
  │         └─ Pick random adjacent walkable tile
  │
  └─ ELSE: idle (no action)
```

### Performance Optimization

`AISystem_ProcessAll()` pre-fetches all monster pointers into a local array before the main loop. This avoids repeated `World_GetPosition()` / `World_GetStats()` / `World_GetAI()` calls through the AI processing chain, reducing pointer chasing from O(n²) to O(n).

### Shadow Monster

The shadow is a special monster that spawns after the player waits too many turns (`SHADOW_SPAWN_WAITS = 15`). It has special behavior:

- After `SHADOW_DOUBLE_MOVE_MIN (25)` wait turns: moves twice per turn
- After `SHADOW_DOUBLE_MOVE_FAST (35)` wait turns: moves twice with increased speed
- Configured via `SpawnerSystem_ConfigureShadow()` based on player level

---

## Spawner System

**File**: `game/src/systems/spawner_system.c` (799 lines)

### Responsibilities
- Create monster ECS entities from templates
- Implement CR-budget floor spawning
- Manage pickup entities (spawn, collect, list, add, reduce)
- Spawn shop NPCs
- Assign meme name variants to monsters

### CR-Budget Spawning

```
SpawnMonstersForFloor(game):
  │
  ├─ Calculate floor budget: Monster_GetFloorBudget(floor)
  │    └─ FLOOR_DR_BASE + floor * FLOOR_DR_PER_FLOOR
  │
  ├─ Get biome monster pool: BiomeDef.monsterPool[]
  │
  ├─ While budget > 0:
  │    ├─ Pick random room (not player's room)
  │    ├─ Pick random monster from biome pool
  │    ├─ Calculate CR: Monster_CalcCR(template, floor)
  │    ├─ If CR <= remaining budget:
  │    │    ├─ Find valid spawn tile in room
  │    │    ├─ SpawnerSystem_SpawnMonster(type, x, y, floor)
  │    │    └─ budget -= CR
  │    └─ Else: try different monster
  │
  └─ Stop when budget exhausted or max attempts reached
```

### Monster Spawning

```
SpawnerSystem_SpawnMonster(gw, type, x, y, floor):
  │
  ├─ Get MonsterTemplate for type
  ├─ World_CreateEntity() → get EntityId
  │
  ├─ Add COMP_POSITION → set (x, y, prevX, prevY, facingDir)
  ├─ Add COMP_STATS → copy template stats
  │    └─ Scale stats by floor: effective_floor = min(floor, maxLevel)
  │    └─ attack += (effective_floor - template.level) * STR_ATTACK_SCALE
  │    └─ hp += (effective_floor - template.level) * CON_HP_SCALE
  │
  ├─ Add COMP_SPRITE_ANIM → load sprite texture, set frame count/speed
  ├─ Add COMP_AI → set type, attackType, detectionRange, attackRange
  ├─ Add COMP_NAME → set template name (or meme variant)
  ├─ Add COMP_HIT_FLASH → timer = 0
  │
  ├─ Assign equipment:
  │    ├─ Random weapon from weaponPool (if any)
  │    ├─ Random armor from armorPool (if any)
  │    └─ Apply equipment bonuses to stats
  │
  ├─ Add COMP_ABILITIES → set weapon ability
  │
  ├─ SpatialHash_Add(gw, e, x, y)
  ├─ gw->aliveMonsterCount++
  │
  └─ Return EntityId
```

### Pickup Management

Pickups are ECS entities with `COMP_POSITION | COMP_PICKUP | COMP_NAME`.

| Operation | Function | Description |
|-----------|----------|-------------|
| Spawn | `SpawnerSystem_SpawnPickups()` | Create pickup entities from TMX objects |
| Find | `SpawnerSystem_FindPickupAt()` | Find any pickup at tile |
| List | `SpawnerSystem_ListPotionsAt()` | List potions without removing |
| List | `SpawnerSystem_ListEquipAt()` | List equipment without removing |
| Collect | `SpawnerSystem_CollectPickupsAt()` | Remove potions, return types/quantities |
| Collect | `SpawnerSystem_CollectEquipAt()` | Remove equipment, return types/quantities |
| Reduce | `SpawnerSystem_ReduceEquipAt()` | Decrease stack quantity |
| Add | `SpawnerSystem_AddPotionAt()` | Create or stack potion on map |
| Add | `SpawnerSystem_AddEquipAt()` | Create or stack equipment on map |

---

## Render Pipeline

**Files**: `renderer.c` (516 lines) + `render_system.c` (299 lines)

### Two-Phase Rendering

The renderer operates in two coordinate spaces:

**Phase 1: World-Space** (inside `BeginMode2D(camera)`)
- Map tiles with FOW overlay
- Pickup entities (potions, equipment)
- Monsters with animation interpolation
- Player with sprite animation
- Shopkeeper
- Projectile animations
- Floating damage numbers
- Floating status messages

**Phase 2: Screen-Space** (after `EndMode2D()`)
- HUD panel (HP bar, EXP bar, level, floor)
- Gold display
- Inspector panel (monster info)
- Tile info panel (pickup info)
- State text ("Your turn", "Enemy turn...", "GAME OVER")
- Combat hints ("[F] Attack", "[T] Throw Weapon")
- Level-up overlay
- Inventory overlay
- Shop overlay

### Animation Interpolation

Movement is interpolated between `prevX/prevY` and `x/y` using a time factor:

```c
float t = 1.0f - (animTimer / animDuration);
float pixelX = prevX * tw * (1-t) + x * tw * t;
float pixelY = prevY * th * (1-t) + y * th * t;
```

This creates smooth sliding movement between tiles at 60 FPS, even though the game is turn-based.

---

## Player System

**Files**: `player_system.c` (66 lines) + `player.c` (64 lines)

### Player Spawning

`PlayerSystem_Spawn()` creates the player entity with all components:

| Component | Initial Values |
|-----------|---------------|
| `COMP_POSITION` | (1, 1), facing DIR_DOWN |
| `COMP_STATS` | Base stats from `game_balance.h`, maxHp from CON, full HP |
| `COMP_SPRITE_ANIM` | Player sprite sheet, row 6, 4 frames, 0.5s speed |
| `COMP_FALLBACK_COLOR` | (50, 200, 255, 255) cyan |
| `COMP_NAME` | "Player" |
| `COMP_PLAYER_TAG` | (tag only, no data) |
| `COMP_HIT_FLASH` | timer = 0 |
| `COMP_ABILITIES` | ABILITY_PUNCH, maxMp from INT |

### Experience & Level-Up

```
GainExperience(game, amount):
  ├─ ps->exp += amount
  └─ While ps->exp >= ps->expToNext:
       ├─ ps->exp -= ps->expToNext
       └─ ApplyLevelUp(game):
            ├─ ps->level++
            ├─ ps->statPoints += STAT_POINTS_PER_LEVEL (2)
            ├─ ps->expToNext = ExpForLevel(ps->level)
            ├─ game->levelUpTimer = LEVEL_UP_FLASH_SECONDS (3.0)
            ├─ GameAudio_PlayLevelUpSound()
            └─ FloatMsg_Spawn("Level N!")
```

### Stat Allocation

```
AllocateStatPoint(game, stats, statIdx):
  ├─ Check statPoints > 0
  ├─ Determine cap (STAT_CAP_DEFAULT=20 or STAT_CAP_UNLIMITED=999)
  ├─ Increment stat (STR/DEX/INT/CON/LCK)
  ├─ Decrement statPoints
  └─ Recalculate maxHp from CON (preserve HP ratio)
```

---

## Spatial Hash

**File**: `spatial_hash.c` (32 lines)

### Design

The spatial hash is a simple 2D array (`monsterGrid[MAP_HEIGHT][MAP_WIDTH]`) where each cell holds an `EntityId` or `ENTITY_NONE`. This provides O(1) tile-to-entity lookup.

### Limitations

- **One entity per tile**: If two monsters occupy the same tile, the second overwrites the first. This is acceptable because the game prevents monsters from stacking.
- **No multi-entity queries**: Cannot find "all monsters at tile" — only returns one.

### Invariant Maintenance

The grid MUST be kept in sync with monster positions. Every spawn, move, and death must update the grid. Failure to do so causes:
- Ghost monsters (grid says monster exists but entity is dead)
- Invisible monsters (entity exists but grid says empty)
- Collision bugs (movement system thinks tile is empty)

---

## World Monster Queries

**File**: `world_monster.c` (53 lines)

### FindMonsterAt

```c
EntityId World_FindMonsterAt(GameWorld* gw, int x, int y, EntityId exclude) {
    EntityId e = gw->monsterGrid[y][x];           // O(1) grid lookup
    if (e == ENTITY_NONE || e == exclude) return ENTITY_NONE;
    if (!gw->ecs.alive[e]) return ENTITY_NONE;    // Validate alive
    if (!has(POSITION | STATS | AI)) return ENTITY_NONE;  // Validate components
    if (has(PLAYER_TAG)) return ENTITY_NONE;       // Not the player
    if (GetStats(e)->alive) return e;              // Stats confirm alive
    return ENTITY_NONE;
}
```

### CountAliveMonsters

Returns the cached `aliveMonsterCount` — O(1) instead of O(n) scan.

### UpdateMonsterAnimations

Iterates all entities with `COMP_STATS | COMP_SPRITE_ANIM | COMP_AI` (excluding player). Ticks sprite frame timers and hit flash timers.

---

## Ability System

**File**: `ability_system.c` (13 lines)

### Current State: STUB

Both functions are declared but do nothing:

```c
bool AbilitySystem_Use(GameWorld* gw, EntityId caster, EntityId target, AbilityType type) {
    return false;  // Not implemented
}

void AbilitySystem_TickCooldowns(GameWorld* gw, EntityId entity) {
    // No-op
}
```

The `CAbilities` component exists and is populated on the player and some monsters, but no system currently reads or modifies it during gameplay. The infrastructure is in place for future ability implementation.

---

## Event Bus

**File**: `event_bus.h/c`

### Design

A synchronous publish/subscribe event bus with 9 event types and fixed subscriber table (max 8 per event). File-scope singleton (same pattern as `game_audio.c`). Events fire immediately at publish time — no queue or flush step needed.

### Event Types

| Event | Payload | Published From |
|-------|---------|---------------|
| `EVT_MONSTER_KILLED` | id, type, pos, xp, gold, dropped equip | combat_system.c (4 death blocks) |
| `EVT_MONSTER_SPAWNED` | id, type, pos | spawner_system.c (both paths) |
| `EVT_PLAYER_LEVEL_UP` | old level, new level, stat points | player.c ApplyLevelUp |
| `EVT_PLAYER_DAMAGED` | damage, hp remaining | ai_system.c (3 attack points) |
| `EVT_ITEM_PICKED_UP` | pos, equip/potion, type id, qty | movement_system.c |
| `EVT_ITEM_EQUIPPED` | equip type | equipment_management.c EquipItem |
| `EVT_ITEM_UNEQUIPPED` | equip type | equipment_management.c UnequipSlot |
| `EVT_FLOOR_DESCENDED` | old floor, new floor | game.c DescendFloor |
| `EVT_GOLD_GAINED` | amount, total gold | shop_ui.c sell points |

### Current Subscribers

- `EVT_MONSTER_KILLED` → DebugLog handler
- `EVT_PLAYER_LEVEL_UP` → DebugLog handler
- `EVT_FLOOR_DESCENDED` → DebugLog handler

---

## JSON Configuration

**Files**: `config.h/c`, `resources/balance.json`

### Design

File-scope cJSON singleton. Loads `balance.json` at startup via raylib's `LoadFileText()`. All `game_balance.h` inline formulas call `Config_GetInt/Float/Bool("section", "key", DEFAULT_MACRO)`. Missing file or keys silently return the `#define` fallback. F3 reloads config at runtime in DEBUG builds.

### Config Hierarchy

15 sections in `balance.json`: player, hp, xp, combat, wait, ai, camera, ui, gold, mp, shadow, atlas, potion, throw, magic.

---

## Texture Atlas

**File**: `atlas.h/c`

### Design

Runtime atlas builder. At startup, loads all monster/equipment/potion sprites via `LoadImage()`, packs them row-by-row into 3 atlases (1024px wide), converts to `Texture2D`. All draw calls use shared atlas textures with per-sprite source rectangles — reducing N individual texture binds to 3 atlases.

### Atlases

| Atlas | Contents | Entries |
|-------|----------|---------|
| Monster | All 11 monster spritesheets | 11 |
| Equipment | All 25+ equipment icons | 25 |
| Item | 3 potion sprites | 3 |

---

## Render Filter & Animation Cache

**Files**: `world.h` (GameWorld fields), `world.c` (GameWorld_RefreshVisibleMonsters), `render_system.c`, `world_monster.c`

### Design

`GameWorld.visibleMonsters[MAX_ENTITIES]` pre-filters visible monsters once per frame via `GameWorld_RefreshVisibleMonsters()`. Both `RenderSystem_DrawMonsters()` and `World_UpdateMonsterAnimations()` iterate the cached array instead of scanning all 128 entities. Cache refreshed at render start and after `RevealFOW()`.

---

## Debug Logging & Crash Handler

**Files**: `debug_log.h/c`

### Design

Category-filtered debug logging with bitmask enum (8 categories + DEBUG_ALL). Output to `logs/latest/debug.log` with automatic archiving to `logs/old/` on restart. `SetUnhandledExceptionFilter` crash handler writes register dump to `logs/crash/crash_TIMESTAMP.txt` on access violations.

### Log Hierarchy

```
logs/
├── latest/
│   └── debug.log       ← current session
├── old/
│   └── debug_YYYY-MM-DD_HH-MM-SS.log  ← archived
└── crash/
    └── crash_YYYY-MM-DD_HH-MM-SS.txt  ← crash reports
```

---

## System Interaction Matrix

This matrix shows which systems call functions from other systems:

| Caller → Callee | Input | Move | Combat | AI | Spawner | Render | Player | Spatial | WorldMon |
|-----------------|-------|------|--------|----|---------|--------|--------|---------|----------|
| **Input** | — | ✓ | ✓ | | ✓ | | ✓ | | ✓ |
| **Movement** | | — | ✓ | | ✓ | | | | ✓ |
| **Combat** | | | — | | ✓ | | ✓ | ✓ | ✓ |
| **AI** | | | | — | | | | ✓ | ✓ |
| **Spawner** | | | | | — | | | ✓ | |
| **Render** | | | | | ✓ | — | | | |
| **Player** | | | | | | | — | | |
| **Spatial** | | | | | | | | — | |
| **WorldMon** | | | | | | | | ✓ | — |

Key interactions:
- **Input → Movement → Combat**: Moving into a monster triggers a bump attack
- **Input → Combat → Spatial**: Killing a monster removes it from the spatial hash
- **AI → Spatial**: Monster movement updates the spatial hash
- **Combat → Player**: Killing a monster grants XP, may trigger level-up
- **Spawner → Spatial**: Spawning adds monsters to the spatial hash
- **Render → Spawner**: Renderer queries pickup data for display

---

## Shared State Access Patterns

### GameWorld Fields Accessed by Multiple Systems

| Field | Writers | Readers |
|-------|---------|---------|
| `state` | Input, Movement, AI, game.c | Input, UpdateGame, Render, main.c |
| `animTimer` | Movement, game.c | game.c, renderer.c |
| `monsterAnimTimer` | AI, game.c | game.c, renderer.c |
| `projectile` | Combat, AI | game.c, renderer.c |
| `monsterGrid` | Spawner, AI, Combat | WorldMonster, Movement, Combat, AI |
| `aliveMonsterCount` | Spawner, Combat | game.c, WorldMonster |
| `visibility` | map_helpers (RevealFOW) | render_system, renderer, AI |
| `blocking` | map_helpers (BuildBlockingMap) | Movement, AI, Combat, map_helpers |
| `inventory` | Movement, inventory.c | inventory_ui, renderer, input |
| `equipped` | inventory.c | inventory_ui, renderer, input, Combat |
| `gold` | Combat | renderer, shop_ui |
| `currentFloor` | game.c (DescendFloor) | floor_init, spawner, renderer |
| `visibleMonsters` | World_Renderer, map_helpers | render_system, world_monster |
| `shopSelection` | shop_ui | shop_ui |
| `damageNumbers` | Combat, inventory.c | game.c (update), renderer.c |
| `floatMsgs` | Combat, Movement, game.c, inventory.c | game.c (update), renderer.c |

### ECS Component Access Patterns

| Component | Writers | Readers |
|-----------|---------|---------|
| `CPosition` | PlayerSystem, Movement, AI | All systems, renderer |
| `CStats` | PlayerSystem, Combat, EquipmentBonus, Player | All systems, renderer, UI |
| `CSpriteAnim` | PlayerSystem, Spawner, game.c, WorldMonster | renderer, render_system |
| `CAI` | Spawner, AI | AI, Combat, Spawner, renderer |
| `CPickup` | Spawner | Movement, renderer, Spawner |
| `CName` | PlayerSystem, Spawner | renderer, inspector, Combat |
| `CHitFlash` | Combat, WorldMonster | renderer, render_system |
| `CAbilities` | PlayerSystem, inventory.c | AI, AbilitySystem (future) |
| `COMP_PLAYER_TAG` | PlayerSystem | AI, Combat, render_system, WorldMonster |
