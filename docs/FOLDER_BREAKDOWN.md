# Folder-by-Folder Breakdown

**Heroes of Taboo** — v0.0.10 | June 5, 2026

---

## Root Directory

```
Heros_of_Taboo/
├── AGENTS.md              AI agent rules, architecture reference, constraints
├── changelog.md           Feature history (v0.0.4 → v0.0.10, latest first)
├── compile_flags.txt      Clang/LSP configuration for code intelligence
├── premake5.lua           Main build configuration (Lua script for Premake5)
├── raylib_premake5.lua    Raylib submodule build configuration
├── engine/                Low-level engine modules
├── game/                  All game code
├── tests/                 Unit test suite
├── docs/                  Documentation suite
└── resources/             Game assets (tilesets, sprites, audio, maps)
```

---

## engine/

Low-level infrastructure with zero game-specific knowledge.

| File | Lines | Responsibility |
|------|-------|---------------|
| `ecs.h` | 70 | ECS core: `World` struct, `EntityId` typedef, component bitmask defines, 9 typed accessor functions (inline), `World_AddComponent`/`World_RemoveComponent` (inline), debug assertion macros |
| `ecs.c` | 56 | Entity lifecycle: `World_Init`, `World_CreateEntity` (free-list recycling), `World_DestroyEntity` (zero + free-list push), `World_IsAlive`, `World_HasComponents` |
| `resources.h` | 9 | Texture cache API: `Resources_LoadTexture(path)`, `Resources_UnloadAll()` |
| `resources.c` | 46 | Path-keyed texture cache (64 slots max), linear scan for duplicates, returns pointer to cached `Texture2D` |
| `audio.h` | 28 | Audio system API: music context registration/switching, sound category registration/playback, volume get/set |
| `audio.c` | ~200 | Audio implementation: scans directories for audio files, manages music playback with crossfade, random sound selection per category |

**Key Design Decisions**:
- Entity pool is fixed at 128 slots — no dynamic allocation
- Texture cache is a flat array with linear scan — simple but O(n) lookup
- Audio uses directory-based registration — each "category" maps to a folder of sound files

---

## game/include/

Shared headers that multiple game modules depend on. These are the "public API" of the game layer.

| File | Lines | Responsibility |
|------|-------|---------------|
| `game_balance.h` | 263 | **Centralized constants**: ~60 `#define` macros for all tuning values (player stats, damage formulas, XP curves, dodge/crit, throw range, wait heal, potion healing, AI behavior, camera, UI scale, gold pricing, MP/abilities, challenge rating). 12 `static inline` formula functions. |
| `equipment_bonus.h` | 30 | Equipment stat modification API: `EquipmentBonus_Apply()`, `EquipmentBonus_Remove()`, `EquipmentBonus_Recalculate()`. All idempotent. |
| `floor_init.h` | 18 | Floor initialization API: `Floor_InitNewFloor(GameWorld*)`. Shared between `InitGame()` and `DescendFloor()`. |
| `validation.h` | 40 | Pure validation predicates (no side effects): `Validate_InventorySlot`, `Validate_EquipType`, `Validate_ItemType`, `Validate_MonsterType`, `Validate_StatIndex`, `Validate_Floor`, `Clamp_Int`. |

**Key Design Decisions**:
- `game_balance.h` is a leaf module — no dependencies on game types or ECS
- All formula functions are `static inline` — zero function call overhead
- Validation functions are pure predicates — no logging, no state mutation

---

## game/src/

All game logic, organized by concern.

### Root Files

| File | Lines | Responsibility |
|------|-------|---------------|
| `main.c` | 255 | **Entry point**: Window initialization, scene dispatcher (MENU/SETTINGS/CONTROLS/STORY/CREDITS/GAME/EXIT), main loop, camera interpolation, input routing |
| `game.h` | 25 | Game state bridge header: `InitGame`, `CleanupGame`, `UpdateGame`, `DescendFloor`, `SetGuiScale`, `GetGuiScale` |
| `game.c` | 407 | **Game orchestration**: `InitGame()` (load map, spawn player, init inventory, call Floor_InitNewFloor), `UpdateGame()` (timer decrements, animation updates, enemy turn processing, floor-clear checks), `DescendFloor()` (save/restore player, regenerate map), `CleanupGame()` (unload resources), `GetUIScale()` (auto-scale calculation) |
| `world.h` | 133 | **GameWorld struct definition**: ECS world + all game state (spatial hash, damage numbers, float messages, map, tilesets, turn state, floor progress, animation timers, projectile, FOW/blocking, camera, inventory/equipment, minimap). Pool structs: `DamageNumber`, `DamageNumberPool`, `FloatMsg`, `FloatMsgPool`. |
| `world.c` | 70 | GameWorld initialization, damage number spawning/updating, float message spawning/updating (variadic printf-style) |
| `game_types.h` | 135 | Core type definitions: `AttackType` enum (4 values), `Direction` enum (5 values), `MonsterType` enum (11 types), `ItemType` enum (4 types), `EquipSlot` enum (5 slots), `EquipCategory` enum (3 categories), `EquipType` enum (30 types), `GameState` enum (7 states), `Projectile` struct |
| `components.h` | 91 | All 10 ECS component structs: `CPosition`, `CStats`, `CSpriteAnim`, `CFallbackColor`, `CAI`, `CPickup`, `CName`, `CHitFlash`, `CAbilities`. Component bitmask defines (10 bits). |
| `inventory.h` | 84 | Inventory/equipment API: `EquipData` struct (17 fields), `InventorySlot` struct, `InventorySubState`/`InventoryTab` enums, 18 public functions for item/equipment management |
| `inventory.c` | 341 | **Monolithic inventory module**: Static data tables (item names, heals, descriptions, sprites, equipment table with 30 entries), potion add/use logic, equipment equip/unequip/drop logic, dual-wield checks, texture loading, weapon ability mapping |
| `equipment_bonus.c` | 56 | Idempotent stat modification: Apply adds bonuses from EquipData to CStats, Remove subtracts, Recalculate derives maxHp from CON and clamps HP |
| `floor_init.c` | 62 | Per-floor setup: build blocking map, init monster templates, select biome, spawn entities from TMX objects, spawn monsters/pickups, optional shop room, set stairs, reset turn state, clear FOW, reveal FOW, position camera |
| `validation.c` | 38 | Pure validation implementations: bounds checking for inventory slots, equip types, item types, monster types, stat indices, floor numbers, and integer clamping |
| `game_audio.h` | 15 | Game-specific audio API: `GameAudio_Init`, `GameAudio_SetContext`, 5 play-sound functions |
| `game_audio.c` | 36 | Audio wrapper: registers 2 music contexts (menu, game) and 5 sound categories (hit, pickup, ranged, magic, levelup), delegates to engine audio |

---

### game/src/systems/

Stateless ECS systems. Each system is a header + implementation pair.

| File | Lines | Responsibility |
|------|-------|---------------|
| `ai_system.h/c` | 9/432 | **Monster AI**: `AISystem_ProcessAll()` iterates all monsters with COMP_AI, decides behavior (hunt, wander, flank, kite, ranged), executes movement/attacks. `AI_GetActiveAbility()` determines monster's attack type. Uses Bresenham line-of-sight, Manhattan distance heuristics, pre-fetched pointers for performance. |
| `combat_system.h/c` | 20/427 | **Combat resolution**: `CombatSystem_PlayerMeleeAttack()` (melee damage, dodge check, crit/mega-crit, dual-wield off-hand follow-up, XP gain, loot drops, death handling), `CombatSystem_PlayerRangedAttack()` (bow attack with projectile), `CombatSystem_PlayerThrowWeapon()` (throw melee weapon). Monster attack resolution is internal to AI system. |
| `input_system.h/c` | 10/429 | **Input handling**: `InputSystem_PlayerTurn()` processes keyboard during player turn (WASD/arrows for movement, F for attack, T for throw, Space for wait, I for inventory, M for map, E for interact/inspect). `InputSystem_Inventory()` handles inventory UI navigation (tabs, selection, equip/use, stat allocation). |
| `movement_system.h/c` | 17/126 | **Movement & collision**: `MovementSystem_IsWalkable()` checks bounds + blocking + monster occupancy, `MovementSystem_PlayerMove()` resolves single-step movement (updates position, triggers FOW reveal, collects pickups, checks stairs/escape, transitions to enemy turn), `MovementSystem_UpdateAnimations()` interpolates entity positions. |
| `render_system.h/c` | 18/299 | **World rendering**: `RenderSystem_DrawMap()` draws tile layers with FOW overlay (visibility 0=skip, 1=full, 2=darkened), `RenderSystem_DrawMonsters()` draws ECS monsters with movement interpolation and sprite animation, `RenderSystem_World()` and `RenderSystem_HUD()` are declared but rendering is done in renderer.c. |
| `renderer.h/c` | 13/516 | **Master render orchestrator**: `RenderGame()` calls all sub-renderers in order (map, pickups, monsters, player, shopkeeper, projectile, damage numbers, float messages, HUD, inspector, tile info, state text, combat hints, level-up overlay, inventory overlay, shop overlay). `Draw9Slice()` for UI panel rendering. `GetUIScale()` for auto-scaling. |
| `spawner_system.h/c` | 49/799 | **Entity spawning**: `SpawnerSystem_SpawnMonster()` creates monster ECS entity from template with floor-scaled stats, equipment, abilities. `SpawnMonstersForFloor()` uses CR-budget system to populate rooms. `SpawnShopRoom()` creates shop NPC. Pickup management: spawn, find, collect, list, add, reduce. Meme name variants for monsters. |
| `player_system.h/c` | 9/66 | **Player spawning**: `PlayerSystem_Spawn()` creates player ECS entity with all components (position, stats, sprite, color, name, player tag, hit flash, abilities). Sets base stats from `game_balance.h` constants. |
| `player.h/c` | 18/64 | **Player progression**: `ExpForLevel()` wraps `calc_xp_for_level()`, `GainExperience()` adds XP and triggers level-ups, `AllocateStatPoint()` distributes stat points with cap enforcement. |
| `ability_system.h/c` | 9/13 | **Ability execution** (STUB): `AbilitySystem_Use()` and `AbilitySystem_TickCooldowns()` are declared but return false / do nothing. Placeholder for future ability system. |
| `spatial_hash.h/c` | 16/32 | **Spatial occupancy grid**: `SpatialHash_Clear()`, `SpatialHash_Add()`, `SpatialHash_Remove()`, `SpatialHash_Move()`. Operates on `GameWorld.monsterGrid[MAP_HEIGHT][MAP_WIDTH]`. |
| `world_monster.h/c` | 11/53 | **Monster queries**: `World_FindMonsterAt()` does O(1) grid lookup with validation, `World_CountAliveMonsters()` returns cached counter, `World_AreAllMonstersDead()` checks counter ≤ 0, `World_UpdateMonsterAnimations()` ticks sprite frames and hit flash timers. |

---

### game/src/data/

Static data tables. Read-only after initialization.

| File | Lines | Responsibility |
|------|-------|---------------|
| `monster_data.h/c` | 55/~300 | 11 `MonsterTemplate` entries with stats, sprites, detection range, floor range, spawn weight, attack type, weapon/armor pools, equip drop chance, projectile visual. CR calculation: `Monster_CalcCR()`, `Monster_SnapCRToTier()`, `Monster_GetFloorBudget()`. TMX name mapping. |
| `loot_data.h/c` | 21/~100 | `LOOT_TABLE[]` array of `LootEntry` structs with category (potion/equip), type ID, tier (1-4), and base weight for weighted random selection. |
| `equip_data.h/c` | 17/~60 | `ItemRarity` enum (4 tiers). Pricing functions: `Equip_GetPowerScore()`, `Equip_GetBasePrice()`, `Equip_GetSellPrice()`. |
| `biome_data.h/c` | 24/~80 | `BiomeType` enum (NONE, GOBLIN_DEN). `BiomeDef` struct with floor range, spawn weight, monster pool, tileset path. `GetBiomeData()`, `Biome_SelectForFloor()`. |
| `ability_data.h/c` | 23/~40 | `AbilityType` enum (5 types). `AbilityData` struct with name, description, MP cost, cooldown. `GetAbilityData()` lookup. |

---

### game/src/map/

Map loading, generation, and spatial queries.

| File | Lines | Responsibility |
|------|-------|---------------|
| `tmx/tmx.h/c` | 71/~400 | **TMX parser**: `MapData` struct (tilesets, layers, objects), `LoadTMX()` parses XML, `UnloadTMX()` frees memory, `GetTileGID()` queries tile data, `GetTileSourceRect()` computes sprite rectangle, `FindTilesetForGID()` locates tileset for a GID. |
| `procedural.h/c` | 53/642 | **Dungeon generator**: BSP room placement with padding constraints, corridor carving, biome-aware tile selection (wall/floor/corner variants), stair placement. `GenerateProceduralMap()` returns `MapData*`. `GetGeneratedRooms()` exposes room metadata. Tile GID constants for VelmoraRealms tileset. |
| `map_helpers.h/c` | 20/229 | **Spatial utilities**: `RevealFOW()` casts Bresenham rays from player position (FOG_RADIUS=7), `BuildBlockingMap()` marks walls/obstacles, `SpawnEscapeTile()` places exit on final floor, `SpawnShadow()` creates shadow pursuer, `SpawnEntitiesFromObjects()` converts TMX objects to ECS entities, `IsInRoom()` checks room membership, `TileToScreen()` coordinate conversion. |

---

### game/src/ui/

User interface modules. All rendering happens in screen-space (after `EndMode2D()`).

| File | Lines | Responsibility |
|------|-------|---------------|
| `menu.h/c` | 65/~500 | **Main menu system**: `Menu_Update()`/`Menu_Render()` for main menu, settings screen (volume controls), controls screen, story screen (XOR-decrypted text), credits screen, in-game pause menu (`GameMenu_Update()`/`GameMenu_Render()`), in-game settings overlay. |
| `inventory_ui.h/c` | 21/~400 | **Inventory overlay**: `InventoryUIState` struct (selection, scroll, tab, sub-state), `InventoryUI_Init()` resets state, `InventoryUI_Render()` draws 3-tab interface (Inventory/Equipment/Stats) with item lists, stat display, equip/use action menus. |
| `shop_ui.h/c` | 9/185 | **Shop interface**: `ShopUI_HandleInput()` processes buy/sell navigation, `ShopUI_Render()` draws shop panel. Buy section sells Band of Growth, sell section lists player inventory with prices. Uses static state for selection. |
| `inspector.h/c` | 14/~150 | **Info panel**: `Inspector_Render()` draws monster stats (name, HP, attack, defense, level) or item info in top-right corner. `InspectorType` enum (MONSTER, ITEM). |
| `map_ui.h/c` | 9/~100 | **Minimap overlay**: `MapUI_Update()` handles M/Z toggle, `MapUI_Render()` draws scaled-down map with player position marker and explored tiles. |
| `text_data.h/c` | 10/~100 | **Encrypted text**: Static byte arrays for XOR-encrypted story (1116 bytes), controls (997 bytes), credits (553 bytes). `RenderTextScreen()` decrypts and displays with scrolling. |

---

## tests/

| File | Lines | Responsibility |
|------|-------|---------------|
| `test_runner.c` | 761 | **28 unit tests** covering: XP curve, max HP, melee/ranged/throw/magic damage, damage after defense, dodge chance, throw range, wait heal, potion heal, equipment data table, item data table, equipment bonus apply/remove/recalculate/edge cases, 7 validation tests, 3 CR tests, 5 spatial hash tests. Custom test harness with `CHECK`/`CHECK_EQ` macros. |

---

## docs/

| File | Responsibility |
|------|---------------|
| `PROJECT_OVERVIEW.md` | High-level project summary |
| `ARCHITECTURE.md` | Architecture explanation and design patterns |
| `FOLDER_BREAKDOWN.md` | This file — folder-by-folder breakdown |
| `API_REFERENCE.md` | Complete API documentation for all public functions |
| `DATA_FLOW.md` | Data flow and control flow explanation |
| `SYSTEMS.md` | Core systems and how they interact |
| `ONBOARDING.md` | New contributor onboarding guide |
| `NEW_FEATURES.md` | Starting points for adding new features |
| `IMPROVEMENTS.md` | Potential improvements and refactors |
| `API.md` | Legacy API reference (pre-documentation suite) |
| `profilingreport.md` | Performance profiling report with hot-spot analysis |
| `VERSIONS.md` | Git tag mapping for version history |

---

## resources/

| Directory | Contents |
|-----------|----------|
| `tilesets/` | VelmoraRealms tileset PNG + TSX definition (448×400, 28 columns, 700 tiles) |
| `sprites/player.png` | Player sprite sheet (4 frames × multiple rows for directions) |
| `sprites/items/equipment/` | Equipment icons organized by type (armors, weapons, accessories) |
| `sprites/items/health_potions/` | Potion sprites (small, medium, large) |
| `sprites/items/loot.png` | Generic loot pile sprite for stacked equipment |
| `sprites/ui/` | UI frame textures (9-slice panels, slots, markers) |
| `sprites/monsters/` | Monster sprite sheets (per monster type) |
| `audio/music/main_menu/` | Menu music tracks |
| `audio/music/game/` | In-game music tracks |
| `audio/sounds/hit/` | Hit sound effects (randomized) |
| `audio/sounds/pickup/` | Pickup sound effects |
| `audio/sounds/ranged_attack/` | Ranged attack sounds |
| `audio/sounds/magic_attack/` | Magic attack sounds |
| `audio/sounds/levelup/` | Level-up sound effects |
| `map.tmx` | Optional hand-authored TMX map (fallback to procedural) |
