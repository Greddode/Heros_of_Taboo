# Heroes of Taboo — Project Overview

**Version**: v0.0.10 | **Last Updated**: June 5, 2026 | **Language**: C99 | **Engine**: Raylib

---

## What Is Heroes of Taboo?

Heroes of Taboo is a turn-based roguelike dungeon crawler written in pure C using the Raylib framework. The player descends through procedurally generated dungeon floors, fights monsters in tactical grid-based combat, collects loot and equipment, and attempts to escape alive.

The game features a complete Entity-Component-System (ECS) architecture, spatial hashing for O(1) monster lookups, a challenge-rating-based difficulty scaling system, and a comprehensive unit test suite.

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
```

---

## Key Features

| Feature | Description |
|---------|-------------|
| **Turn-based combat** | Melee, ranged, magic, and throw attacks with dodge/crit mechanics |
| **Dual wielding** | Single-handed weapons in off-hand slot with follow-up strikes |
| **Mega-crits** | Chance to double crits exceeding 100 damage |
| **Procedural dungeons** | BSP room generation with biome-aware tile placement |
| **Challenge Rating** | Monster difficulty scales with floor number and CR budget |
| **Equipment system** | 30 equipment types across 5 slots (head, chest, weapon, off-hand, accessory) |
| **Inventory** | 16-slot potion bag + 16-slot equipment bag with overflow-to-floor safety |
| **Fog of war** | Bresenham ray-based visibility with explored/seen/unseen states |
| **Floating damage numbers** | Animated damage text above hit targets |
| **Floating status messages** | Animated text for events (level up, floor clear, escape) |
| **Shop system [NOT TESTED]** | Rare shop rooms with buy/sell interface |
| **Audio system** | Context-switching music (menu/game) + randomized SFX categories |
| **UI scaling** | Auto-scaled HUD with configurable GUI scale |
| **Minimap** | Toggle-able full-map overlay |

---

## Technical Stack

| Layer | Technology |
|-------|-----------|
| **Language** | C99 (strict — no C11/C23 features) |
| **Graphics/Audio** | Raylib (OpenGL 3.3, GLFW backend) |
| **Build** | Premake5 → GNU Make (Windows: w64devkit) |
| **Testing** | Custom C test harness (28 unit tests, <2s runtime) |
| **Architecture** | Structure-of-Arrays ECS with bitmask component ownership |
| **Map format** | Tiled TMX (XML parser) + procedural fallback |
| **Resource management** | Path-keyed texture cache (64 slots max) |

---

## Project Statistics

| Metric | Value |
|--------|-------|
| Source files (.c) | 34 |
| Header files (.h) | 30 |
| Total lines of code | ~12,000 |
| Unit tests | 28 |
| ECS component types | 10 |
| Monster types | 11 |
| Equipment types | 30 |
| Potion types | 3 |
| Ability types | 5 |
| Max entities | 128 |
| Map dimensions | 100×100 tiles |

---

## Version History

| Version | Date | Focus |
|---------|------|-------|
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
├── engine/          ECS core, resource manager, low-level audio
├── game/
│   ├── include/     Shared headers (game_balance, equipment_bonus, floor_init, validation)
│   └── src/         All game logic
│       ├── systems/ ECS systems (AI, combat, input, movement, render, spawner, etc.)
│       ├── data/    Static data tables (monsters, loot, equipment, biomes, abilities)
│       ├── map/     TMX loader, procedural generation, FOW, blocking map
│       └── ui/      Menu, inventory, shop, inspector, minimap, text screens
├── tests/           Unit test runner (28 tests)
├── docs/            Documentation suite
├── resources/       Tilesets, sprites, audio, TMX maps
└── AGENTS.md        AI agent rules and architecture reference
```

---

## Non-Negotiable Constraints

These rules apply to every contribution:

1. **C99 only** — no VLAs, designated initializers, or `_Generic`
2. **Raylib types directly** — no custom wrappers around `Texture2D`, `Vector2`, etc.
3. **Textures via `Resources_LoadTexture()`** — never call `LoadTexture()` directly
4. **Components in `components.h`** — all component structs in one canonical file
5. **Stateless systems** — no `static` local variables in system `.c` files
6. **Entity 0 is `ENTITY_NONE`** — reserved sentinel, never allocate
7. **ECS iteration checks masks** — always use `World_HasComponents()` before accessing
8. **Idempotent functions** — `EquipmentBonus_Apply/Remove` safe to call multiple times
9. **All 28 tests must pass** before any commit
10. **No new runtime dependencies** beyond Raylib

---

## Related Documentation

- [Architecture & Design Patterns](ARCHITECTURE.md)
- [Folder-by-Folder Breakdown](FOLDER_BREAKDOWN.md)
- [API Reference](API_REFERENCE.md)
- [Data Flow & Control Flow](DATA_FLOW.md)
- [Core Systems & Interactions](SYSTEMS.md)
- [Onboarding Guide](ONBOARDING.md)
- [Adding New Features](NEW_FEATURES.md)
- [Potential Improvements](IMPROVEMENTS.md)
- [Profiling Report](profilingreport.md)
