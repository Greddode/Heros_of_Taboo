# Changelog

## ALPHA-0.0.6 — Rebalance & Inventory Update

---

### For Players

- **Sprint movement** — hold SHIFT + direction to slide to the nearest obstacle. Sprint stops at room/hallway boundaries; press sprint again to bypass. All monsters process AI between each step.
- **Health potions** — pick up potions from the ground (press I to open inventory). Three tiers: Small (12 HP, floors 1–2), Big (48 HP, floors 3–5), Large (128 HP, floors 6+). Use, Drop, or Drop All from the action menu.
- **Potion sprites** — each potion type has its own sprite on the map. Stacked potions show a quantity badge.
- **Click to inspect** — left-click a potion tile to see its contents in the info panel.
- **9-slice UI panels** — stats, combat log, monster info, and inventory now use framed texture backgrounds instead of solid rectangles.
- **Wait healing scales** — resting (period/space) heals `1 + (level / 5) * 2` HP (1 at levels 1–4, 3 at 5–9, 5 at 10–14, etc.).
- **6 new monster types** — Bat, Demon Eye, Dragon, Floating Eye, Fungal Myconid, Warp Skull added across 3 difficulty tiers (Early/Mid/Late).
- **Monster rebalance** — floor stat scaling reduced from `GetRandomValue(3,6)` to `GetRandomValue(1,3)`; spawn weights are flat for consistent rarity.

### For Developers

#### Codebase Refactor (`game/src/core/`)

- `game.c` (1352 → 594 lines) split into focused modules:
  - **`renderer.c`** — `Draw9Slice()`, `RenderGame()` (map, entities, HUD)
  - **`inventory.c`** — item metadata, `InventoryAdd/Use`, potion texture loading, `Inventory_Render()` overlay
  - **`map_helpers.c`** — `IsInRoom()`, `RevealFOW()`, `BuildBlockingMap()`, `SpawnEntitiesFromObjects()`, `SpawnEscapeTile()`, `SpawnShadow()`
- `game.h` now includes `inventory.h`, `renderer.h`, `map_helpers.h` and exports only core functions (`InitGame`, `CleanupGame`, `HandleInput`, `UpdateGame`, `DescendFloor`).
- `premake5.lua` uses wildcard `files {"**.c", "**.h"}` — no manual Makefile updates needed.

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
- `Monster_InitTemplates()` — 3-tier configuration: Early (floors 1+), Mid (floors 3+), Late (floors 6+). All base EXP doubled.
- `Monster_Spawn()` — scales level by `GetRandomValue(1,3)` per floor.
- Sprite paths moved from `resources/sprite_animations/idle/` to `resources/sprites/monsters/`.

#### Inventory System (`game/src/core/inventory.c`, `inventory.h`)

- `ItemType` enum: `ITEM_NONE`, `ITEM_SMALL_HP_POTION`, `ITEM_BIG_HP_POTION`, `ITEM_LARGE_HP_POTION`.
- `InventorySlot` struct with `type` and `quantity`; max 16 slots, stacking by type.
- `InventoryAdd()` / `InventoryUse()` — add stacks, consume items with HP restoration.
- `Inventory_Render()` — full overlay: item list with selection, info panel with description/quantity/heal amount, action menu popup.
- `LoadPotionTextures()` / `UnloadPotionTextures()` — load 3 potion sprites and 2 UI frame textures.

#### 9-Slice UI (`game/src/core/renderer.c`)

- `Draw9Slice(tex, dest, l, t, r, b)` — draws a 9-sliced texture without corner distortion.
- `UI_Flat_Frame01a.png` (96×64, 16px corners) for stats panel, inventory, combat log.
- `UI_Flat_FrameSlot01b.png` (32×32, 8px corners) for info popups, action menus.
- All UI functions fall back to solid-colour rectangles when textures are missing.

#### Combat Log (`game/src/ui/combat_log.c`, `combat_log.h`)

- `CombatLog_Render()` now accepts optional `Texture2D bgTex` and `int sliceMargin` for 9-sliced background.

#### New/Modified Files

| File | Change |
|------|--------|
| `game/src/core/game.c` | Refactored — core loop only |
| `game/src/core/game.h` | Removed item/inventory/render decls, added module includes |
| `game/src/core/renderer.c` | **New** — `Draw9Slice`, `RenderGame` |
| `game/src/core/renderer.h` | **New** — renderer API |
| `game/src/core/inventory.c` | **New** — item metadata, inventory logic, overlay rendering |
| `game/src/core/inventory.h` | **New** — `ItemType`, `InventorySlot`, inventory API |
| `game/src/core/map_helpers.c` | **New** — `IsInRoom`, `RevealFOW`, `BuildBlockingMap`, spawn helpers |
| `game/src/core/map_helpers.h` | **New** — map helper API |
| `game/src/entity/monster.c` | 6 new monsters, `MonsterTemplate` with `minFloor`/`spawnWeight` |
| `game/src/entity/monster.h` | 6 new `MonsterType` enums, template struct |
| `game/src/entity/spawner.c` | Weighted random selection filtered by `minFloor` |
| `game/src/entity/entity.c` | Potion pickup with inventory stacking |
| `game/src/entity/player.c` | Black combat log text |
| `game/src/ui/combat_log.c` | 9-slice background support |
| `game/src/ui/combat_log.h` | Updated `CombatLog_Render` signature |
| `game/src/ui/monster_info.c` | 9-sliced info panel |
| `game/src/ui/monster_info.h` | Include game.h |
| `resources/sprites/items/health_potions/` | **New** — 3 potion sprites |
| `resources/sprites/monsters/` | **New** — monster sprites |
| `resources/sprites/ui/` | **New** — `UI_Flat_Frame01a`, `UI_Flat_FrameSlot01b` |

## ALPHA-0.0.5 — Story Board & Floors Update

---

### For Players

- **Multi-floor dungeon** — 10 floors to descend. All monsters start at level 1 and gain 3–6 random extra levels per floor deeper (+2 HP, +1 ATK, +0.5 DEF, +5 EXP per level), making each floor progressively more dangerous.
- **Stairs progression** — floors 1–9 have stairs to go deeper; floor 10 is the final floor.
- **Escape tile** — on floor 10, kill all monsters to spawn an escape tile. Step on it to win.
- **Shadow monster** — after 15 idle turns, a shadow spawns at twice your level (min level 10). It has three AI states (slowed, normal, frenzy) and a detection range of 20. Does not spawn on floor 10.
- **Story screen** — new "Story" option on the main menu displays the game's lore text with word wrapping and scrolling support.
- **Monster info panel** — click a monster to inspect its stats on the right side of the screen (clears on movement).
- **HUD update** — shows "Floor: X/10" instead of enemy count.
- **Combat log colors** — yellow for floor cleared, green for escape tile appeared, red for shadow spawn.
- **Credits update** — added "DeepDiveGameStudio - Monster Sprites".

### For Developers

#### Floor System (`game/src/core/game.c`, `game/src/core/game.h`)

- `Game` struct: added `currentFloor`, `maxFloors` (10), `stairX/Y`, `escapeX/Y`, `timeWaited`, `floorClearedNotified`, `escapeSpawned`, `selectedMonsterIdx`.
- `DescendFloor()` — resets `timeWaited` and `escapeSpawned` via `memset`, preserves player stats across floors.
- Floor 10: no stairs, all monster kills → `SpawnEscapeTile()` places GID 47, sets `STATE_WIN`.
- `SpawnShadow()` — spawns shadow at `timeWaited == 15` on floors 1–9 only.

#### Monster Scaling (`game/src/entity/monster.c`)

- All monster templates start at level 1 (base stats unchanged).
- `Monster_ScaleToFloor()` — adds `(floor - 1) * GetRandomValue(3, 6)` extra levels per floor.
- Stat formula per extra level: +2 HP, +1 ATK, +0.5 DEF, +5 EXP.
- `MONSTER_SHADOW` enum with `shadowTurnCounter`; three AI states based on `timeWaited` (< 25 slowed, 25–34 normal, ≥ 35 frenzy).

#### Procudural Generation (`game/src/core/procedural.c`, `game/src/core/procedural.h`)

- `TILE_STAIRS` (137), `TILE_ESCAPE` (47) added to `IsFloorGID()` for collision/walkability.
- `GetStairX/Y()` returns stair position.

#### Story Screen (`game/src/ui/menu.c`, `game/src/ui/menu.h`, `game/src/ui/text_data.c`)

- New `MENU_STORY` enum option and `SCENE_STORY` scene.
- Story text is XOR-encrypted (key `0xAB`, 1116 bytes) and embedded in the binary via `s_story_data[]`.
- `Story_LoadFile()` decodes into memory, splits by line.
- `Story_BuildVisualLines()` — word-wraps source lines to fit screen width; rebuilds on window resize.
- Scrolling with UP/DOWN keys, clamped to visual line count.

#### Combat Log (`game/src/ui/combat_log.c`, `game/src/ui/combat_log.h`)

- Per-message `Color` array instead of global tint for flexible message coloring.

#### New/Modified Files

| File | Change |
|------|--------|
| `resources/story.txt` | Added — plaintext story lore |
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

## ALPHA-0.0.4 — Music Update

---

### For Players

- **Dynamic game music** — 5 `.ogg` tracks now play randomly in-game, cycling seamlessly one after another with no gap between tracks.
- **Menu music** — a dedicated track plays on the main menu (stops when the game starts).
- **New tileset** — replaced the grey placeholder tileset with the VelmoraRealms tileset for a proper dungeon look.
- **Floor tile variation** — floors now randomly mix 9 tile variants (GIDs 23, 24, 48–53) alongside the base floor tile. 75% of tiles use the base floor, 25% use variants.
- **Fixed wall rendering** — walls now display correctly around all floor variants (no gaps or misplaced edges).

### For Developers

#### Audio System (`game/src/core/audio.c`)

- Replaced hardcoded single `.mp3` paths with directory-based loading of `.ogg` files:
  - Menu tracks loaded from `resources/audio/music/main_menu/`
  - Game tracks loaded from `resources/audio/music/game/`
- `LoadMusicFromDir()` loads all `.ogg` files from a directory into a `Music` array.
- Game tracks set to `looping = true` for seamless internal looping.
- Track cycling uses `GetMusicTimePlayed()` / `GetMusicTimeLength()` to detect loop boundaries and switch tracks at the exact wrap point — no audible gap.
- `PickRandomGameTrack()` avoids repeating the same track twice in a row.
- `ShutdownAudioSystem()` unloads all loaded `Music` streams.
- Old `.mp3` files (`Abnormal Circumstances.mp3`, `Element.mp3`) no longer referenced.

#### Tileset & Rendering

- **`procedural.h`**: All tile GID constants updated to VelmoraRealms indices. `FLOOR_VARIANT_COUNT` and `FLOOR_VARIANTS[]` array added for easy editing.
- **`procedural.c`**: `IsFloorGID()` helper checks all floor variants. `RandomFloorGID()` picks with 75/25 weighting. All wall placement, corner detection, and collision sync functions use `IsFloorGID()` instead of direct `== TILE_FLOOR` comparison.
- **`entity/entity.c`**: `DrawTile()` uses `FindTilesetForGID()` to select the correct texture per tile.
- **`core/game.c`**: `InitGame` loops over all map tilesets to load textures. `CleanupGame` unloads all.
- **`core/game.h`**: `Game::tilesetTexture` → `Game::tilesetTextures[]` array.
- **`spawner.c`**: `Spawner_Populate` uses `IsFloorGID()` so entities spawn on any floor variant.

#### Code Cleanup

- Removed redundant comments across all source files (obvious bounds checks, trivially named variables, self-documenting blocks).
- Kept architectural comments (section dividers, complex logic explanations, public API docs).
- `credits.txt` added to `game/src/ui/` — decrypted plaintext for easy editing (re-encrypt via `text_data.c` with XOR key `0xAB`).

#### New Files

| File | Purpose |
|------|---------|
| `CHANGELOG.md` | This file — release notes per version |
| `resources/tilesets/VelmoraRealms-TileSet.png` | Primary dungeon tileset (448×400, 28 cols, 700 tiles) |
| `resources/tilesets/VelmoraRealms-TileSet.tsx` | Tiled tileset definition |
| `game/src/ui/credits.txt` | Decrypted credits for editing |

#### Removed

| File | Reason |
|------|--------|
| `resources/tileset_gray.png` | Replaced by VelmoraRealms |
| `resources/tileset_gray.tsx` | Replaced by VelmoraRealms |
