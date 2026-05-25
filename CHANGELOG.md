# Changelog

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
