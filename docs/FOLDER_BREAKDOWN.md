# Folder-by-Folder Breakdown

**Heroes of Taboo** — v0.0.12 | June 5, 2026

---

## Root Directory

```
Heros_of_Taboo/
├── premake5.lua           Main build configuration
├── raylib_premake5.lua    Raylib submodule build configuration
├── engine/                Low-level engine modules
├── game/                  All game code
├── tests/                 Unit test suite (48 tests)
├── docs/                  Documentation suite
├── resources/             Game assets (tilesets, sprites, audio, maps, balance.json)
└── logs/                  Runtime debug logs + crash reports
```

---

## engine/

Core engine modules with zero game-specific knowledge.

| File | Lines | Responsibility |
|------|-------|---------------|
| `ecs.h` | 70 | ECS header: `World` struct, 10 component types, ID typedefs, bitmask helpers, `World_Get*` accessors (all `static inline`) |
| `ecs.c` | 56 | Entity lifecycle: `World_Init()`, `World_CreateEntity()`, `World_DestroyEntity()`, `World_IsAlive()`, `World_HasComponents()`. Free-list entity recycling. |
| `resources.h` | 30 | Texture cache: `Resources_LoadTexture()`, `Resources_UnloadAll()`, `Resources_LoadTextureFromImage()`. Path-keyed hash cache (64 slots). |
| `resources.c` | 120 | Resource manager implementation. Linear scan cache lookup. |
| `audio.h/c` | 180 | Music context switching, sound categories, volume control |
| `cJSON.h/c` | 2800 | Vendored single-file JSON parser. Used by config system. |

---

## game/include/

Shared headers that multiple game modules depend on.

| File | Lines | Responsibility |
|------|-------|---------------|
| `game_balance.h` | 240 | Centralized constants (~60 `#define` macros) + 12 `static inline` formula functions. All formulas read from config with `#define` fallback. |
| `equipment_bonus.h` | 30 | Equipment stat API: `Apply/Remove/Recalculate` — all idempotent |
| `floor_init.h` | 18 | Floor initialization: `Floor_InitNewFloor(GameWorld*)` |
| `validation.h` | 50 | Pure validation predicates: `Validate_EntityId`, `Validate_TilePos`, `Validate_EquipSlot`, `Validate_InventorySlot`, `Validate_EquipType`, `Validate_ItemType`, `Validate_MonsterType`, `Validate_StatIndex`, `Validate_Floor`, `Clamp_Int` |
| `config.h` | 14 | `Config_Load/Reload/Unload`, `Config_GetInt/Float/Bool`, `Config_LoadFromString` (testing) |

---

## game/src/

All game logic, organized by concern.

### Root Files

| File | Lines | Responsibility |
|------|-------|---------------|
| `main.c` | 267 | Entry point. Scene dispatcher (menu/settings/story/game). Calls `Config_Load` at startup, `Config_Unload` at exit. F3 = reload config. |
| `game.h` | 25 | Game orchestration header |
| `game.c` | 450 | `InitGame()`, `UpdateGame()`, `DescendFloor()`, `CleanupGame()`. Floor transition protocol, event bus subscriber setup. |
| `world.h` | 149 | `GameWorld` struct (god object), `DamageNumberPool`, `FloatMsgPool`, `Projectile`. All 50+ game state fields. |
| `world.c` | 88 | `GameWorld_Init()`, `GameWorld_RefreshVisibleMonsters()`, `DamageNumber_Spawn/UpdateAll`, `FloatMsg_Spawn/UpdateAll` |
| `game_types.h` | 135 | All enums: `AttackType`, `Direction`, `MonsterType`, `ItemType`, `EquipSlot`, `EquipCategory`, `EquipType`, `GameState`, `Projectile` |
| `components.h` | 91 | All 10 ECS component structs + bitmask defines |
| `inventory.h` | 35 | Umbrella header: constants, `InventorySlot`, `InventorySubState`, `InventoryTab`. Includes all 4 sub-headers. |
| `equipment_bonus.c` | 62 | Stat apply/remove/recalculate with `LOG_WARNING` guards |
| `floor_init.c` | 82 | Shared floor setup: blocking map, monster spawn, pickups, FOW, camera. With validation warnings. |
| `validation.c` | 60 | Implements all validation predicates |
| `game_audio.c` | 95 | Audio context registration and sound playback |
| `debug_log.h` | 40 | `DebugCategory` bitmask enum, `DebugLog()` variadic macro, `DebugLog_SetFilter()`, `DebugLog_InitCrashHandler()` |
| `debug_log.c` | 150 | Debug log file output, timestamped archiving, `SetUnhandledExceptionFilter` crash handler with register dump |
| `event_bus.h` | 85 | `EventType` enum (9 types), 8 payload structs, `EventCallback` typedef, `EventBus_*` API |
| `event_bus.c` | 35 | File-scope subscriber table (8 per event), synchronous dispatch |
| `config.c` | 80 | cJSON-based JSON loader. `Config_Load/Reload/Unload`, `Config_GetInt/Float/Bool`. `LoadFileText` → `cJSON_Parse` flow. |
| `atlas.h` | 18 | `AtlasEntry` struct, `Atlas_BuildAll/Destroy`, `Atlas_GetMonster/Equip/Item` |
| `atlas.c` | 175 | Runtime atlas packer: loads sprites via `LoadImage`, packs into 3 atlases (monsters/equipment/items), creates `Texture2D` |

### Equipment & Inventory

| File | Contents |
|------|----------|
| `equipment_management.h/c` | `EquipItem`, `EquipItemSilent`, `UnequipSlot`, `IsEquipSlotOccupied`, `IsTwoHandedEquipped`, `IsWeaponDualWieldable`, `IsDualWielding`, `AddEquipToInventory`, `RemoveEquipFromInventory` |
| `inventory_logic.h/c` | `InventoryAdd`, `InventoryUse`, `LoadPotionTextures`, `Inventory_LoadEquipTexture`, `Inventory_LoadPotionTexture`, `Inventory_GetWeaponAbility` |

---

## game/src/systems/

### ECS Systems

| File | Lines | Responsibility |
|------|-------|---------------|
| `input_system.c` | 429 | Keyboard input → game actions (movement, attack, inventory, shop) |
| `movement_system.c` | 152 | Player movement + walkability + pickup collection + stairs/escape checks. With `LOG_WARNING` guards. |
| `combat_system.c` | 482 | Full damage pipeline (raw → defense → dodge → crit → mega → apply). Death handling: `World_DestroyEntity`, spatial hash cleanup, loot drops. `LOG_WARNING`/`LOG_ERROR` guards. |
| `ai_system.c` | 477 | Monster AI decision tree (attack/hunt/wander/idle). Pre-fetched pointers for performance. Spatial hash desync check. `LOG_ERROR` guards. |
| `render_system.c` | 293 | `RenderSystem_DrawMap()` (tiles + FOV), `RenderSystem_DrawMonsters()` (uses visible cache + atlas) |
| `spawner_system.c` | 855 | CR-budget monster spawning, pickup ECS entity management (9 operations). `LOG_ERROR` on pool exhaustion. Uses atlas for sprite textures. |
| `player_system.c` | 66 | Player entity creation with all 8 components |
| `player.c` | 72 | XP/level-up/stat allocation logic. Publishes `EVT_PLAYER_LEVEL_UP`. |
| `ability_system.c` | 13 | STUB — not yet implemented |
| `spatial_hash.c` | 50 | O(1) tile→entity grid. `LOG_WARNING` on overwrite/desync/mismatch. |

### Render Pipelines

| File | Lines | Responsibility |
|------|-------|---------------|
| `renderer.c` | 43 | Thin orchestrator: `Draw9Slice`, `RenderGame()` dispatches to 3 sub-renderers |
| `world_renderer.c` | 295 | Everything inside `BeginMode2D`: map, pickups (atlas), monsters (atlas), player, shopkeeper, projectile, damage numbers, float messages |
| `hud_renderer.c` | 128 | HP bar, EXP bar, level, floor info, gold, state text, combat hints, level-up overlay |
| `overlay_renderer.c` | 98 | Inspector panel, tile info panel, inventory overlay, shop overlay |

---

## game/src/data/

| File | Lines | Responsibility |
|------|-------|---------------|
| `monster_data.c` | 379 | 11 monster templates + CR calculation + floor budget |
| `loot_data.c` | 100 | 4-tier loot table with weights |
| `equip_data.h` | 35 | `EquipData` full struct definition + `ItemRarity` enum + pricing functions |
| `equip_data.c` | 30 | `Equip_GetPowerScore/BasePrice/SellPrice` |
| `biome_data.c` | 50 | `BIOME_GOBLIN_DEN` definition + floor selection |
| `ability_data.c` | 16 | Ability metadata (name, MP cost, cooldown) |
| `item_data.c` | 55 | `ITEM_NAMES/HEALS/DESCS/SPRITES[]`, `GetItemName/HealAmount/Description/SpritePath` |
| `equip_table.c` | 83 | `EQUIP_TABLE[30]`, `GetEquipData`, `GetEquipRangeBonus` |

---

## tests/

| File | Lines | Tests | Coverage |
|------|-------|-------|----------|
| `test_runner.c` | 890 | 48 | Stats, equipment, validation, CR, spatial hash, counters, event bus, config |

---

## docs/

10 documentation files covering architecture, systems, folder breakdown, API reference, data flow, onboarding, new features, and improvement proposals.

---

## resources/

| Path | Contents |
|------|----------|
| `tilesets/` | Tileset PNG + TSX data |
| `sprites/` | Player, 11 monster sprites, equipment icons, potion icons, UI frames |
| `audio/` | Music tracks + SFX organized by category |
| `balance.json` | 15-section JSON config (overrides game_balance.h defaults) |
| `map.tmx` | Optional Tiled map (fallback: procedural generation) |
