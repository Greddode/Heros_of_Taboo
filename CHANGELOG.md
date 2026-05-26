# Changelog

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
