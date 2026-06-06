# Heroes of Taboo — Project Overview

**Version**: v0.0.12 | **Last Updated**: June 5, 2026 | **Language**: C99 | **Engine**: Raylib

---

## What Is Heroes of Taboo?

Heroes of Taboo is a turn-based roguelike dungeon crawler written in pure C using the Raylib framework. The player descends through procedurally generated dungeon floors, fights monsters in tactical grid-based combat, collects loot and equipment, and attempts to escape alive.

The game features a complete Entity-Component-System (ECS) architecture, spatial hashing for O(1) monster lookups, a challenge-rating-based difficulty scaling system, a comprehensive unit test suite, and a modern event bus with JSON configuration support.

---

## Core Gameplay Loop

```
Player enters floor
  → Procedural dungeon generated (rooms + corridors, biome-aware)
  → Monsters spawned (CR-budget system, floor-scaled stats)
  → Player explores (fog of war, visibility rays)
  → Combat occurs (melee, ranged, magic, throw, dual-wield)
  → Loot collected (potions, equipment, gold)
  → Player levels up (stat points, XP curve)
  → Find Stairs → descend to next floor
  → After max floors → escape tile spawns → win condition
  → Events published throughout (event bus)
```

---

## Key Features

| Feature | Description |
|---------|-------------|
| **Turn-based combat** | Melee, ranged, magic, and throw attacks with dodge/crit/mega-crit mechanics |
| **Dual wielding** | Single-handed weapons in off-hand slot with follow-up strikes |
| **Procedural dungeons** | BSP room generation with biome-aware tile placement |
| **Challenge Rating** | Monster difficulty scales with floor number and CR budget |
| **Equipment system** | 30 equipment types across 5 slots (head, chest, weapon, off-hand, accessory) |
| **Inventory** | 16-slot potion bag + 16-slot equipment bag |
| **Fog of war** | Bresenham ray-based visibility with explored/seen/unseen states |
| **Shop system** | Buy/sell interface in rare shop rooms |
| **Audio system** | Context-switching music (menu/game) + randomized SFX |
| **UI scaling** | Auto-scaled HUD with configurable GUI scale |
| **Minimap** | Toggle-able full-map overlay |
| **Event bus** | 9 event types, synchronous dispatch, extensible subscriber system |
| **JSON config** | `resources/balance.json` with runtime reload (F3 key) |
| **Debug logging** | Category-filtered file output with timestamped archives |
| **Crash reports** | Automatic exception capture with register dump |
| **Render filter** | Pre-filtered visible monster cache for render + animation |
| **Texture atlas** | Runtime-packed monster/equipment/potion atlases |
| **Error checking** | LOG_WARNING/LOG_ERROR guards across all ECS systems |

---

## Technical Stack

| Layer | Technology |
|-------|-----------|
| **Language** | C99 (strict — no C11/C23 features) |
| **Graphics/Audio** | Raylib (OpenGL 3.3, GLFW backend) |
| **Build** | Premake5 → GNU Make (Windows: w64devkit) |
| **Testing** | Custom C test harness (48 unit tests) |
| **Architecture** | Structure-of-Arrays ECS with bitmask component ownership |
| **Map format** | Tiled TMX (XML parser) + procedural fallback |
| **Resource management** | Path-keyed texture cache (64 slots) + runtime atlas packing |
| **JSON parsing** | cJSON (vendored single-file C library) |
| **Config** | `balance.json` with #define fallback defaults |

---

## Project Statistics

| Metric | Value |
|--------|-------|
| Source files (.c) | 44 |
| Header files (.h) | 38 |
| Total lines of code | ~14,500 |
| Unit tests | 48 |
| ECS component types | 10 |
| Monster types | 11 |
| Equipment types | 30 |
| Potion types | 3 |
| Max entities | 128 |
| Map dimensions | 100×100 tiles |
| Event types | 9 |
| Config sections | 15 |

---

## Version History

| Version | Date | Focus |
|---------|------|-------|
| v0.0.12 | 2026-06-05 | Crash handler, render filter, animation cache, texture atlas, event bus, JSON config, split inventory/renderer, shop state fix |
| v0.0.11 | 2026-06-05 | Debug logging system, error checking across all systems, extended validation, spatial hash guards |
| v0.0.10 | 2026-06-03 | Equipment rebalance, mega-crits, floating damage, dual wield, shop, abilities |
| v0.0.9 | 2026-06-03 | Bug fixes (7 critical), spatial hash, cached counter, AI batching |
| v0.0.8 | 2026-06-02 | Refactoring (formulas, equipment bonus, floor init, tests, profiling, API docs) |
| ALPHA-0.0.7 | 2026-06-01 | Ranged combat + AI rebalance |
| ALPHA-0.0.6 | earlier | Sprint + inventory system |
| ALPHA-0.0.5 | earlier | Multi-floor dungeon + story |
| ALPHA-0.0.4 | earlier | Music + tileset update |

---

## Directory Structure (High Level)

```
Heros_of_Taboo/
├── engine/          ECS core, resource manager, audio, cJSON
├── game/
│   ├── include/     game_balance.h, equipment_bonus.h, floor_init.h, validation.h
│   └── src/         All game logic
│       ├── systems/ ECS systems (AI, combat, input, movement, render, spawner, etc.)
│       ├── data/    Static data tables (monsters, loot, equipment, biomes, items)
│       ├── map/     TMX loader, procedural generation, FOW, blocking map
│       └── ui/      Menu, inventory, shop, inspector, minimap, text screens
│       ├── atlas.h/c            Texture atlas builder
│       ├── config.h/c           JSON configuration loader
│       ├── debug_log.h/c        Debug logging with crash handler
│       └── event_bus.h/c        Event publish/subscribe bus
├── tests/           Unit test runner (48 tests)
├── docs/            Documentation suite
├── resources/       Tilesets, sprites, audio, TMX maps, balance.json
└── logs/            Runtime debug logs + crash reports
```

---

## Non-Negotiable Constraints

1. **C99 only** — no VLAs, designated initializers, or `_Generic`
2. **Raylib types directly** — no custom wrappers around `Texture2D`, `Vector2`, etc.
3. **Textures via `Resources_LoadTexture()`** — never call `LoadTexture()` directly
4. **Components in `components.h`** — all component structs in one canonical file
5. **Stateless systems** — no `static` local variables in system `.c` files
6. **Entity 0 is `ENTITY_NONE`** — reserved sentinel, never allocate
7. **ECS iteration checks masks** — always use `World_HasComponents()` before accessing
8. **No dynamic allocation in systems** — use fixed arrays
9. **All 48 tests must pass** before any commit
10. **No new runtime dependencies** beyond Raylib + cJSON (vendored)

---

## Related Documentation

- [Architecture & Design Patterns](ARCHITECTURE.md)
- [Core Systems & Interactions](SYSTEMS.md)
- [Folder-by-Folder Breakdown](FOLDER_BREAKDOWN.md)
- [API Reference](API_REFERENCE.md)
- [Data Flow & Control Flow](DATA_FLOW.md)
- [Onboarding Guide](ONBOARDING.md)
- [Adding New Features](NEW_FEATURES.md)
- [Potential Improvements](IMPROVEMENTS.md)
