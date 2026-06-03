# Changelog

All notable changes to this project will be documented in this file.

## [v0.0.9] - 2026-06-02

### Bug Fixes
- **Tile tearing** — `TileToScreen` now snaps to whole pixels, preventing sub-pixel positions during player movement.
- **Inspector word wrap** — fixed buffer overflow (256→512), single-word overflow dropped silently, and `maxWidth` corrected to account for left/right padding.
- **Ranged AI priority** — ranged monsters now: retreat if adjacent, flank into cardinal line if diagonal, advance if out of range, or hold position.
- **Magic defense formula** — changed from `intel * 3` to `defense / 2 + intel * 2`, making both DEF and INT meaningful against magic attacks.
- **Map close keys** — pressing M or Z while map is open already closes it (verified, no change needed).
- **HP scaling with CON** — equipping/unequipping CON gear or spending a stat point on CON now grants HP equal to the max-HP increase, instead of only capping down.
- **Ranged weapons** — all 5 bows no longer grant ATK; their former ATK values redistributed to DEX. Descriptions updated accordingly.

## [Latest] - 2026-06-02

### Codebase Refactoring (Tasks 1–3)
- **game_balance.h** — centralized all stat formulas, damage calculations, dodge, XP curves, UI scale, camera constants, player base stats, and game tuning parameters into a single documented header with 30 macros and 12 inline functions.
- **equipment_bonus.c/h** — extracted duplicated equip/unequip stat-bonus apply/remove logic from inventory.c, game.c, and input_system.c into idempotent `EquipmentBonus_Apply`/`Remove`/`Recalculate` functions.
- **floor_init.c/h** — extracted duplicated per-floor setup (blocking map, monster templates, spawning, stairs, state/timer reset, visibility, FOW, camera) from `InitGame` and `DescendFloor` into `Floor_InitNewFloor`. Each function shrinks by ~30 lines.
- **Unit tests** — added 16 deterministic unit tests covering all game_balance formulas (XP, HP, damage, dodge, throw range, wait heal, potion heal), equipment data table validation, item data validation, and equipment bonus apply/remove/recalculate with edge cases (NULL world, missing CStats, EQUIP_NONE). Runnable via `make test` and returns 0 on pass.
- Replaced ~160 lines of literal magic numbers and duplicated blocks across 7 files with named constants and inline formula functions.

## [Latest] - 2026-06-01

### Engine Revamp
- **Audio Rewrite**: Complete audio system refactor with new context-based music system and categorized sound effects
  - Replaced hardcoded menu/game tracks with flexible `MusicContext` system
  - Implemented `SoundCategory` for better sound organization
  - Added lazy-loading for audio resources
  - Renamed audio functions with `Audio_` prefix for clarity

- **ECS Cleanup**: 
  - Added warning log when entity pool exhausted
  - Minor ECS robustness improvements

- **Combat Features**:
  - Added ranged weapon attacks with line-of-sight detection
  - Implemented weapon-throwing mechanic that unequips the weapon
  - Added 5 new ranged weapons: Simple Bow, Dwarven Bow, Elven Bow, Greatbow, Crossbow
  - Ranged weapons have varying attack ranges and stat bonuses
  - Improved monster AI with flanking and kiting behaviors

- **Balance Pass**:
  - Adjusted monster experience values (reduced by ~20-40%)
  - Added `maxFloor` and `maxLevel` caps to monster templates
  - Changed attack types: Floating Eye (melee→magic), Fungal Myconid (melee→ranged), Dragon (melee→ranged)
  - Increased floating eye intellect for magic-focused behavior
  - Updated loot table tier minimum floors (1→3, 1→5, 4→7)
  - Modified experience scaling curve (20+level*10 → 50+level*40)
  - Fixed HP restoration on floor descent to use ratio instead of full heal

- **UI Improvements**:
  - Added dungeon map UI (press M or Z to view)
  - Added combat action hints (F for attack, T for throw)
  - Fixed inventory scroll bounds checking
  - Added "Map" game state

- **Monster AI Enhancements**:
  - Implemented `MoveAwayFrom` for ranged kiting behavior
  - Implemented `MoveFlank` for better tactical positioning
  - Magic damage calculation now uses intelligence only (removed defense subtraction)
  - Different monster types now spawn with specialized behaviors

- **Player System**:
  - Changed player spawn position from (0,0) to (1,1)
  - Improved player animation system
  - Added fallback color component for better rendering

- **Map Generation**:
  - Added filtering to prevent stairs at room-to-corridor junctions
  - Improved tileset loading with fallback support

- **Gameplay Features**:
  - Added "meme names" for monsters with rarity-based replacement
  - New game state for map viewing
  - Improved potion/equipment tile inspection display

- **Other Fixes**:
  - Fixed typo: "earily Quite" → "eerily quiet"
  - Refactored input system for better code organization
  - Improved movement system with dedicated `MovementSystem_PlayerMove` function
  - Fixed shadow spawn to only trigger once per floor
  - Audio system now properly initializes before game starts

---

For earlier versions, see git history.

---

## Historical (pre-refactor, preserved for reference)


## ALPHA-0.0.8 G�� Equipment & Stat System

---

### For Players

- **Equipment system** G�� 17 equipment types across 3 categories (Armor, Weapon, Accessory) with 5 equip slots (Head, Chest, Weapon, Off-hand, Accessory). Each item provides stat bonuses (ATK, DEF, STR, DEX, INT, CON, LCK). Two-handed weapons (War Hammer) block the off-hand slot.
- **Inventory UI tabs** G�� inventory screen now has 3 tabs (Inventory / Equipment / Stats) switchable with Q/E. Inventory tab shows potions and unequipped gear; Equipment tab shows equipped items with Unequip/Drop/Back actions; Stats tab displays base stats, derived stats, and unspent stat points.
- **Stat point allocation** G�� gain +2 stat points per level. Allocate to STR (damage), DEX (dodge), MGC (potion healing), CON (max HP), or LCK (crit chance, loot rarity). Max HP is now derived from CON (30 + CON x 5).
- **Loot system overhaul** G�� unified loot table with 4 rarity tiers: Common (Tier 1), Uncommon (Tier 2), Rare (Tier 3, floor 4+), Legendary (Tier 4, floor 6+). Luck and floor depth boost higher tier drops. Equipment drops from monsters on death (20% base + LCK x 2, max 50%).
- **Equipment on the ground** G�� equipment items now spawn on dungeon floors with their own sprites. Stacked items show a loot pile icon with quantity badge. Click to inspect.
- **Monster equipment drops** G�� defeating a monster has a chance to drop a random accessory (filtered by floor availability).
- **Level-up notification** G�� gold "LEVEL UP!" overlay with "+3 Stat Points!" subtitle, fades out over 3 seconds. Level-up sound effect added.
- **Potion heal scales with INT** G�� potion healing is now `base% x (1 + INT x 0.02)`, making Intelligence stat meaningful for sustain.
- **Potion rarity on ground** G�� floor-placed potions now roll randomly (50% small, 30% medium, 20% large) instead of being floor-locked.
- **GUI Scale setting** G�� adjustable UI scale (1.0x to 4.0x) in the Settings menu. All UI elements scale proportionally.
- **Updated controls screen** G�� documents Q/E tab switching, sprint controls, mouse click inspect, and equipment management.
- **Scrollable menus** G�� main menu, story, credits, and controls screens now scroll when content exceeds the viewport.
- **Starting equipment** G�� player now starts with a Survival Knife (+2 ATK) instead of bare fists.

### For Developers

#### Equipment System (`game/src/core/inventory.c`, `inventory.h`)

- `EquipType` enum: 17 equipment types G�� 3 head armors, 3 chest armors, 5 weapons, 3 shields, 7 accessories.
- `EquipSlot` enum: `HEAD`, `CHEST`, `WEAPON`, `OFF_HAND`, `ACCESSORY`.
- `EquipData` struct: name, description, sprite path, category, slot, stat bonuses (ATK/DEF/STR/DEX/INT/CON/LCK), `twoHanded` flag.
- `EquipItem()` / `EquipItemSilent()` G�� equip with stat bonus application, CON-derived max HP recalculation.
- `UnequipSlot()` G�� reverts stat bonuses, moves item to `equipInventory[]` (preserved across floors).
- `AddEquipToInventory()` / `RemoveEquipFromInventory()` G�� manage unequipped gear storage.
- Equipment sprite loading: `LoadPotionTextures()` now also loads all equipment sprites from `resources/sprites/items/equipment/`.

#### Inventory Tab UI (`game/src/core/inventory.c`)

- `InventoryTab` enum: `INV_TAB_INVENTORY`, `INV_TAB_EQUIPMENT`, `INV_TAB_STATS`.
- Tab bar rendering with `Draw9Slice` marker texture, yellow underline for active tab.
- Inventory tab: combined potion + equipment list with `[Slot] Name` format for gear.
- Equipment tab: 5 slot rows with equipped item name, sprite, and action menu (Unequip/Drop/Back).
- Stats tab: two-column layout G�� left column shows base stats (STR/DEX/INT/CON/LCK), HP, ATK, DEF, EXP; right column shows stat point allocation with Enter/Space to allocate.
- `Inspector_Render()` unified for both monster and item inspection, showing equipment sprites, descriptions, and stat bonuses.

#### Stat System (`game/src/entity/entity.h`, `player.c`)

- `Entity` struct: added `str`, `dex`, `intel`, `con`, `lck` core stats and `statPoints` counter.
- `AllocateStatPoint()` G�� increments one stat, decrements `statPoints`, recalculates max HP from CON.
- `GainExperience()` G�� triggers level-ups, awards +2 stat points per level.
- `ExpForLevel()` G�� base formula: `20 + level * 10`.
- Combat formula updated: player damage = `attack + STR * 2 - monster.defense`. Crit chance = LCK%. Monster dodge = DEX * 2% (max 60%).

#### Loot System (`game/src/entity/spawner.c`)

- Unified `LootEntry` struct: `LootCategory` (POTION/EQUIP), `typeId`, `tier` (1G��4), `baseWeight`.
- `LOOT_TABLE[]` G�� 23 entries across 4 rarity tiers with minimum floor requirements (Tier 3: floor 4+, Tier 4: floor 6+).
- Tier selection: effective weight = `baseWeight + LCK * 2 * tier + floor * 3 * tier`.
- Item selection within tier uses base weights for variety.
- Equipment placed on map tiles with stacking support and loot pile sprite for multiples.

#### Shadow Scaling Rework (`game/src/core/map_helpers.c`)

- Shadow monster now scales stats via exponential formula: `scale = 1.12^(level-1)`.
- Stats derived from scaled core attributes: `HP = 10 + CON * 5`, `ATK = 4 + STR * 2`, `DEF = 1 + CON/2`, `EXP = 10 + LCK * 3`.

#### FOW Optimization (`game/src/core/map_helpers.c`)

- Bresenham ray-casting for line-of-sight within fog radius.
- Wall adjacency: cardinal neighbors always checked; diagonal 8-dir check only on corner tiles (tiles with an adjacent wall) to peek around corners.

#### GUI Scale (`game/src/core/game.c`, `game/src/ui/menu.c`)

- `GetUIScale()` / `SetGuiScale()` / `GetGuiScale()` G�� global UI scale multiplier (1.0G��4.0).
- All menu screens, HUD elements, inventory, and inspector panels scale proportionally.
- Settings menu: new "GUI Scale" slider with 0.25x increments.

#### Audio (`game/src/core/audio.c`, `audio.h`)

- `PlayLevelUpSound()` G�� loads and plays random `.wav` from `resources/audio/sounds/levelup/`.

#### Controls Update (`game/src/ui/text_data.c`)

- Controls screen text re-encoded (XOR key `0xAB`, 886 bytes) with Q/E tab switching, sprint, inventory, and mouse click documentation.

#### New/Modified Files

| File | Change |
|------|--------|
| `game/src/core/inventory.c` | Equipment system, tab UI, stat allocation, loot table |
| `game/src/core/inventory.h` | `EquipType`, `EquipData`, `EquipSlot`, `EquipCategory`, tab enums |
| `game/src/core/game.c` | Stats tab input, equipment action menu, level-up timer, GUI scale |
| `game/src/core/game.h` | Added `equipInventory[]`, `equipMapCount/Types/Tiles/Collected/Quantities`, `levelUpTimer`, `inventoryTab`, `statsSelection/ActiveCol/ScrollCol1/2` |
| `game/src/core/renderer.c` | Equipment sprite rendering on map, loot pile rendering, level-up overlay |
| `game/src/core/map_helpers.c` | Bresenham FOW, corner diagonal adjacency, shadow stat scaling, potion rarity roll |
| `game/src/core/audio.c` | Level-up sound loading and playback |
| `game/src/core/audio.h` | `PlayLevelUpSound()` |
| `game/src/entity/entity.h` | `str/dex/intel/con/lck` stats, `statPoints` field |
| `game/src/entity/entity.c` | STR-based damage, LCK crits, DEX dodge, equipment drops on kill |
| `game/src/entity/player.c` | `AllocateStatPoint()`, `ExpForLevel()`, stat point awards on level-up |
| `game/src/entity/monster.h` | `str/dex/intel/con/lck` core stats in template and instance |
| `game/src/entity/monster.c` | Core stat initialization and floor scaling |
| `game/src/entity/spawner.c` | Unified loot table, tier-based drop system, equipment spawning |
| `game/src/ui/inspector.c` | **New** G�� unified monster/item inspector with equipment display |
| `game/src/ui/inspector.h` | **New** G�� `InspectorType`, `Inspector_Render()` |
| `game/src/ui/menu.c` | GUI scale setting, scrollable menus, story selection cursor |
| `game/src/ui/text_data.c` | Updated controls text (886 bytes), `RenderTextScreen` scroll parameter |
| `game/src/ui/text_data.h` | Updated `s_controls_data` size, `RenderTextScreen` signature |
| `game/src/ui/monster_info.c` | **Removed** G�� replaced by `inspector.c` |
| `game/src/ui/monster_info.h` | **Removed** G�� replaced by `inspector.h` |
| `resources/sprites/items/equipment/` | **New** G�� 22 equipment sprites (armors, weapons, accessories) |
| `resources/sprites/items/loot.png` | **New** G�� loot pile sprite for stacked equipment |
| `resources/sprites/items/health_potions/medium_health_potion.png` | **New** G�� medium potion sprite |
| `resources/sprites/ui/UI_Flat_FrameMarker01a.png` | **New** G�� tab bar marker texture |
| `resources/audio/sounds/levelup/Jump9.wav` | **New** G�� level-up sound effect |

## ALPHA-0.0.7 G�� Ranged Combat & AI Rebalance

---

### For Players

- **Goblin Archer** G�� new ranged monster (floor 2+): fires pixel-line projectiles up to 5 tiles in cardinal directions.
- **Warp Skull magic** G�� Warp Skull now attacks with wave-animated magic projectiles (range 3, cardinal only).
- **Smarter monster AI** G�� monsters remember your last seen position for 4 turns after losing line of sight, chase you around corners.
- **Better wandering** G�� monsters try up to 4 random directions before giving up, and patrol a wider area.
- **Sprint picks up potions** G�� sprinting over a potion tile now correctly collects it.

### For Developers

- `AttackType` enum (`ATTACK_MELEE`, `ATTACK_RANGED`, `ATTACK_MAGIC`) in `entity.h`.
- `MonsterTemplate`/`Monster` structs: new `attackType`, `attackRange` fields.
- `Projectile` struct in `game.h`: active flag, start/end tile coords, attack type, animation frames, color.
- Projectile timers (`projectileTimer`/`projectileDuration`) count down each frame; auto-clear on expiry (0.25s).
- `animatingEnemyTurn` flag G�� delays player turn transition until projectiles and monster movement animations finish.
- `magicAttacksTexture` G�� loaded from `resources/tilesets/magic_attacks.png`, unloaded in `CleanupGame`.
- Ranged/magic sound effects G�� loaded from `resources/audio/sounds/ranged_attack/` and `magic_attack/`.
- AI refactor: `MoveToward()` / `ApplyMove()` helper functions extracted, `Monster_ProcessAllAI` takes `Game*`.
- Hunt memory: `lastSeenX/Y` + `huntTurns` fields, 4-turn chase after LOS lost.
- Multi-attempt wander: up to 4 direction tries, wander range expanded to `detectionRange + 8`.
- Bugfix: restored `alive = true`, `active = true`, `facingRight = true`, `strncpy(name, ...)`, `expValue` in `Monster_Spawn`.
- Bugfix: added missing `attackType`/`attackRange` to Goblin template.

## ALPHA-0.0.6 G�� Rebalance & Inventory Update

---

### For Players

- **Sprint movement** G�� hold SHIFT + direction to slide to the nearest obstacle. Sprint stops at room/hallway boundaries; press sprint again to bypass. All monsters process AI between each step.
- **Health potions** G�� pick up potions from the ground (press I to open inventory). Three tiers: Small (12 HP, floors 1G��2), Big (48 HP, floors 3G��5), Large (128 HP, floors 6+). Use, Drop, or Drop All from the action menu.
- **Potion sprites** G�� each potion type has its own sprite on the map. Stacked potions show a quantity badge.
- **Click to inspect** G�� left-click a potion tile to see its contents in the info panel.
- **9-slice UI panels** G�� stats, combat log, monster info, and inventory now use framed texture backgrounds instead of solid rectangles.
- **Wait healing scales** G�� resting (period/space) heals `1 + (level / 5) * 2` HP (1 at levels 1G��4, 3 at 5G��9, 5 at 10G��14, etc.).
- **6 new monster types** G�� Bat, Demon Eye, Dragon, Floating Eye, Fungal Myconid, Warp Skull added across 3 difficulty tiers (Early/Mid/Late).
- **Monster rebalance** G�� floor stat scaling reduced from `GetRandomValue(3,6)` to `GetRandomValue(1,3)`; spawn weights are flat for consistent rarity.

### For Developers

#### Codebase Refactor (`game/src/core/`)

- `game.c` (1352 G�� 594 lines) split into focused modules:
  - **`renderer.c`** G�� `Draw9Slice()`, `RenderGame()` (map, entities, HUD)
  - **`inventory.c`** G�� item metadata, `InventoryAdd/Use`, potion texture loading, `Inventory_Render()` overlay
  - **`map_helpers.c`** G�� `IsInRoom()`, `RevealFOW()`, `BuildBlockingMap()`, `SpawnEntitiesFromObjects()`, `SpawnEscapeTile()`, `SpawnShadow()`
- `game.h` now includes `inventory.h`, `renderer.h`, `map_helpers.h` and exports only core functions (`InitGame`, `CleanupGame`, `HandleInput`, `UpdateGame`, `DescendFloor`).
- `premake5.lua` uses wildcard `files {"**.c", "**.h"}` G�� no manual Makefile updates needed.

#### Sprint System (`game/src/core/game.c`)

- SHIFT+direction: scans in a line to find the farthest unobstructed tile, then steps one-by-one.
- `IsInRoom()` checks room boundaries using `GetGeneratedRooms()`. Room-to-hallway transition stops sprint; `sprintBypassRoom` flag allows re-triggering past the boundary.
- Monster positions snapshotted before sprint and restored as `prevX/Y` for smooth interpolation.
- Sprint animation: fixed 0.30s uniform duration via `animDuration`/`monsterAnimDuration`.
- Input blocked while `animTimer > 0` (prevents mid-animation direction changes).
- Sprint direction uses `IsKeyDown` (hold to continue); normal movement uses `IsKeyPressed`.

#### Monster System (`game/src/entity/monster.c`, `monster.h`)

- 10 monster types total (6 new): `MONSTER_BAT`, `MONSTER_DEMON_EYE`, `MONSTER_DRAGON`, `MONSTER_FLOATING_EYE`, `MONSTER_FUNGAL_MYCONID`, `MONSTER_WARP_SKULL`.
- `MonsterTemplate` struct: `minFloor` (1/3/6), `spawnWeight` for weighted random selection.
- `Monster_InitTemplates()` G�� 3-tier configuration: Early (floors 1+), Mid (floors 3+), Late (floors 6+). All base EXP doubled.
- `Monster_Spawn()` G�� scales level by `GetRandomValue(1,3)` per floor.
- Sprite paths moved from `resources/sprite_animations/idle/` to `resources/sprites/monsters/`.

#### Inventory System (`game/src/core/inventory.c`, `inventory.h`)

- `ItemType` enum: `ITEM_NONE`, `ITEM_SMALL_HP_POTION`, `ITEM_BIG_HP_POTION`, `ITEM_LARGE_HP_POTION`.
- `InventorySlot` struct with `type` and `quantity`; max 16 slots, stacking by type.
- `InventoryAdd()` / `InventoryUse()` G�� add stacks, consume items with HP restoration.
- `Inventory_Render()` G�� full overlay: item list with selection, info panel with description/quantity/heal amount, action menu popup.
- `LoadPotionTextures()` / `UnloadPotionTextures()` G�� load 3 potion sprites and 2 UI frame textures.

#### 9-Slice UI (`game/src/core/renderer.c`)

- `Draw9Slice(tex, dest, l, t, r, b)` G�� draws a 9-sliced texture without corner distortion.
- `UI_Flat_Frame01a.png` (96+�64, 16px corners) for stats panel, inventory, combat log.
- `UI_Flat_FrameSlot01b.png` (32+�32, 8px corners) for info popups, action menus.
- All UI functions fall back to solid-colour rectangles when textures are missing.

#### Combat Log (`game/src/ui/combat_log.c`, `combat_log.h`)

- `CombatLog_Render()` now accepts optional `Texture2D bgTex` and `int sliceMargin` for 9-sliced background.

#### New/Modified Files

| File | Change |
|------|--------|
| `game/src/core/game.c` | Refactored G�� core loop only |
| `game/src/core/game.h` | Removed item/inventory/render decls, added module includes |
| `game/src/core/renderer.c` | **New** G�� `Draw9Slice`, `RenderGame` |
| `game/src/core/renderer.h` | **New** G�� renderer API |
| `game/src/core/inventory.c` | **New** G�� item metadata, inventory logic, overlay rendering |
| `game/src/core/inventory.h` | **New** G�� `ItemType`, `InventorySlot`, inventory API |
| `game/src/core/map_helpers.c` | **New** G�� `IsInRoom`, `RevealFOW`, `BuildBlockingMap`, spawn helpers |
| `game/src/core/map_helpers.h` | **New** G�� map helper API |
| `game/src/entity/monster.c` | 6 new monsters, `MonsterTemplate` with `minFloor`/`spawnWeight` |
| `game/src/entity/monster.h` | 6 new `MonsterType` enums, template struct |
| `game/src/entity/spawner.c` | Weighted random selection filtered by `minFloor` |
| `game/src/entity/entity.c` | Potion pickup with inventory stacking |
| `game/src/entity/player.c` | Black combat log text |
| `game/src/ui/combat_log.c` | 9-slice background support |
| `game/src/ui/combat_log.h` | Updated `CombatLog_Render` signature |
| `game/src/ui/monster_info.c` | 9-sliced info panel |
| `game/src/ui/monster_info.h` | Include game.h |
| `resources/sprites/items/health_potions/` | **New** G�� 3 potion sprites |
| `resources/sprites/monsters/` | **New** G�� monster sprites |
| `resources/sprites/ui/` | **New** G�� `UI_Flat_Frame01a`, `UI_Flat_FrameSlot01b` |

## ALPHA-0.0.5 G�� Story Board & Floors Update

---

### For Players

- **Multi-floor dungeon** G�� 10 floors to descend. All monsters start at level 1 and gain 3G��6 random extra levels per floor deeper (+2 HP, +1 ATK, +0.5 DEF, +5 EXP per level), making each floor progressively more dangerous.
- **Stairs progression** G�� floors 1G��9 have stairs to go deeper; floor 10 is the final floor.
- **Escape tile** G�� on floor 10, kill all monsters to spawn an escape tile. Step on it to win.
- **Shadow monster** G�� after 15 idle turns, a shadow spawns at twice your level (min level 10). It has three AI states (slowed, normal, frenzy) and a detection range of 20. Does not spawn on floor 10.
- **Story screen** G�� new "Story" option on the main menu displays the game's lore text with word wrapping and scrolling support.
- **Monster info panel** G�� click a monster to inspect its stats on the right side of the screen (clears on movement).
- **HUD update** G�� shows "Floor: X/10" instead of enemy count.
- **Combat log colors** G�� yellow for floor cleared, green for escape tile appeared, red for shadow spawn.
- **Credits update** G�� added "DeepDiveGameStudio - Monster Sprites".

### For Developers

#### Floor System (`game/src/core/game.c`, `game/src/core/game.h`)

- `Game` struct: added `currentFloor`, `maxFloors` (10), `stairX/Y`, `escapeX/Y`, `timeWaited`, `floorClearedNotified`, `escapeSpawned`, `selectedMonsterIdx`.
- `DescendFloor()` G�� resets `timeWaited` and `escapeSpawned` via `memset`, preserves player stats across floors.
- Floor 10: no stairs, all monster kills G�� `SpawnEscapeTile()` places GID 47, sets `STATE_WIN`.
- `SpawnShadow()` G�� spawns shadow at `timeWaited == 15` on floors 1G��9 only.

#### Monster Scaling (`game/src/entity/monster.c`)

- All monster templates start at level 1 (base stats unchanged).
- `Monster_ScaleToFloor()` G�� adds `(floor - 1) * GetRandomValue(3, 6)` extra levels per floor.
- Stat formula per extra level: +2 HP, +1 ATK, +0.5 DEF, +5 EXP.
- `MONSTER_SHADOW` enum with `shadowTurnCounter`; three AI states based on `timeWaited` (< 25 slowed, 25G��34 normal, G�� 35 frenzy).

#### Procudural Generation (`game/src/core/procedural.c`, `game/src/core/procedural.h`)

- `TILE_STAIRS` (137), `TILE_ESCAPE` (47) added to `IsFloorGID()` for collision/walkability.
- `GetStairX/Y()` returns stair position.

#### Story Screen (`game/src/ui/menu.c`, `game/src/ui/menu.h`, `game/src/ui/text_data.c`)

- New `MENU_STORY` enum option and `SCENE_STORY` scene.
- Story text is XOR-encrypted (key `0xAB`, 1116 bytes) and embedded in the binary via `s_story_data[]`.
- `Story_LoadFile()` decodes into memory, splits by line.
- `Story_BuildVisualLines()` G�� word-wraps source lines to fit screen width; rebuilds on window resize.
- Scrolling with UP/DOWN keys, clamped to visual line count.

#### Combat Log (`game/src/ui/combat_log.c`, `game/src/ui/combat_log.h`)

- Per-message `Color` array instead of global tint for flexible message coloring.

#### New/Modified Files

| File | Change |
|------|--------|
| `resources/story.txt` | Added G�� plaintext story lore |
| `CHANGELOG.md` | This entry |
| `game/src/ui/text_data.c` | Added `s_story_data[1116]` (XOR-encrypted) |
| `game/src/ui/text_data.h` | Added `s_story_data` extern |
| `game/src/ui/menu.c` | Story option, scrolling word-wrapped renderer |
| `game/src/ui/menu.h` | `MENU_STORY`, `Menu_StoryUpdate/Render/ResetStory` |
| `game/src/core/game.c` | Floor descent, shadow/escape spawning, monster info panel |
| `game/src/core/game.h` | Floor/shadow/info fields |
| `game/src/entity/monster.c` | Shadow template, floor scaling, state AI |
| `game/src/entity/monster.h` | `MONSTER_SHADOW`, `shadowTurnCounter`, updated `Monster_ProcessAllAI` |
| `game/src/entity/spawner.c` | Excludes `TILE_STAIRS` from spawn tiles, passes `currentFloor` |
| `game/src/ui/combat_log.c` | Per-message `Color` array support |
| `game/src/ui/combat_log.h` | `Color` array in ring buffer |
| `game/src/ui/monster_info.c` | Monster info panel rendering |
| `game/src/ui/monster_info.h` | Info panel declarations |
| `game/src/core/procedural.c` | `TILE_STAIRS`, `TILE_ESCAPE`, `IsFloorGID` update |
| `game/src/core/procedural.h` | Stair/escape GID constants |
| `game/src/main.c` | `SCENE_STORY` scene handling |

## ALPHA-0.0.4 G�� Music Update

---

### For Players

- **Dynamic game music** G�� 5 `.ogg` tracks now play randomly in-game, cycling seamlessly one after another with no gap between tracks.
- **Menu music** G�� a dedicated track plays on the main menu (stops when the game starts).
- **New tileset** G�� replaced the grey placeholder tileset with the VelmoraRealms tileset for a proper dungeon look.
- **Floor tile variation** G�� floors now randomly mix 9 tile variants (GIDs 23, 24, 48G��53) alongside the base floor tile. 75% of tiles use the base floor, 25% use variants.
- **Fixed wall rendering** G�� walls now display correctly around all floor variants (no gaps or misplaced edges).

### For Developers

#### Audio System (`game/src/core/audio.c`)

- Replaced hardcoded single `.mp3` paths with directory-based loading of `.ogg` files:
  - Menu tracks loaded from `resources/audio/music/main_menu/`
  - Game tracks loaded from `resources/audio/music/game/`
- `LoadMusicFromDir()` loads all `.ogg` files from a directory into a `Music` array.
- Game tracks set to `looping = true` for seamless internal looping.
- Track cycling uses `GetMusicTimePlayed()` / `GetMusicTimeLength()` to detect loop boundaries and switch tracks at the exact wrap point G�� no audible gap.
- `PickRandomGameTrack()` avoids repeating the same track twice in a row.
- `ShutdownAudioSystem()` unloads all loaded `Music` streams.
- Old `.mp3` files (`Abnormal Circumstances.mp3`, `Element.mp3`) no longer referenced.

#### Tileset & Rendering

- **`procedural.h`**: All tile GID constants updated to VelmoraRealms indices. `FLOOR_VARIANT_COUNT` and `FLOOR_VARIANTS[]` array added for easy editing.
- **`procedural.c`**: `IsFloorGID()` helper checks all floor variants. `RandomFloorGID()` picks with 75/25 weighting. All wall placement, corner detection, and collision sync functions use `IsFloorGID()` instead of direct `== TILE_FLOOR` comparison.
- **`entity/entity.c`**: `DrawTile()` uses `FindTilesetForGID()` to select the correct texture per tile.
- **`core/game.c`**: `InitGame` loops over all map tilesets to load textures. `CleanupGame` unloads all.
- **`core/game.h`**: `Game::tilesetTexture` G�� `Game::tilesetTextures[]` array.
- **`spawner.c`**: `Spawner_Populate` uses `IsFloorGID()` so entities spawn on any floor variant.

#### Code Cleanup

- Removed redundant comments across all source files (obvious bounds checks, trivially named variables, self-documenting blocks).
- Kept architectural comments (section dividers, complex logic explanations, public API docs).
- `credits.txt` added to `game/src/ui/` G�� decrypted plaintext for easy editing (re-encrypt via `text_data.c` with XOR key `0xAB`).

#### New Files

| File | Purpose |
|------|---------|
| `CHANGELOG.md` | This file G�� release notes per version |
| `resources/tilesets/VelmoraRealms-TileSet.png` | Primary dungeon tileset (448+�400, 28 cols, 700 tiles) |
| `resources/tilesets/VelmoraRealms-TileSet.tsx` | Tiled tileset definition |
| `game/src/ui/credits.txt` | Decrypted credits for editing |

#### Removed

| File | Reason |
|------|--------|
| `resources/tileset_gray.png` | Replaced by VelmoraRealms |
| `resources/tileset_gray.tsx` | Replaced by VelmoraRealms |
