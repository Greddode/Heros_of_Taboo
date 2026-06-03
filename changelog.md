# Changelog

All notable changes to this project will be documented in this file.

## [v0.0.10] - 2026-06-03

### Documentation & Agent Enablement

- **AGENTS.md** — Complete rewrite with all components for AI agent development:
  - Current architecture diagram (post-ECS refactor)
  - All 11 non-negotiable constraints consolidated
  - Profiling hot-spots from `docs/profilingreport.md` with optimization recommendations
  - Build setup with C:\raylib\w64devkit tool location
  - 3 common refactoring patterns with code examples
  - Quick reference file location table (organized by category)
  - Module dependency graph (acyclic DAG)
  - Next steps for AI agents with effort estimates (Phase 1–3)
  - Expanded FAQ addressing design decisions

- **docs/VERSIONS.md** — Updated with v0.0.10 tag mapping

- **Test count clarification** — Verified 28 unit tests (up from initial 16), all passing

---

## [v0.0.9] - 2026-06-02

### Bug Fixes
- **Tile tearing** — `TileToScreen` now snaps to whole pixels, preventing sub-pixel positions during player movement.
- **Inspector word wrap** — fixed buffer overflow (256→512), single-word overflow dropped silently, and `maxWidth` corrected to account for left/right padding.
- **Ranged AI priority** — ranged monsters now: retreat if adjacent, flank into cardinal line if diagonal, advance if out of range, or hold position.
- **Magic defense formula** — changed from `intel * 3` to `defense / 2 + intel * 2`, making both DEF and INT meaningful against magic attacks.
- **Map close keys** — pressing M or Z while map is open already closes it (verified, no change needed).
- **HP scaling with CON** — equipping/unequipping CON gear or spending a stat point on CON now grants HP equal to the max-HP increase, instead of only capping down.
- **Ranged weapons** — all 5 bows no longer grant ATK; their former ATK values redistributed to DEX. Descriptions updated accordingly.

---

## [v0.0.8] - 2026-06-02

### Codebase Refactoring (Tasks 1–3)
- **game_balance.h** — centralized all stat formulas, damage calculations, dodge, XP curves, UI scale, camera constants, player base stats, and game tuning parameters into a single documented header. Replaced ~121 lines of literal magic numbers across 5 files.
- **equipment_bonus.c/h** — extracted duplicated equip/unequip stat-bonus apply/remove logic from inventory.c, game.c, and input_system.c into idempotent `EquipmentBonus_Apply`/`Remove`/`Recalculate` functions. Zero duplication.
- **floor_init.c/h** — extracted duplicated per-floor setup (blocking map, monster templates, spawning, stairs, state/timer reset, visibility, FOW, camera) from `InitGame` and `DescendFloor` into shared `Floor_InitNewFloor` function. ~60 lines saved per call site.
- **validation.c/h** — added 7 pure validation predicates (no side effects) for inventory slots, equipment types, item types, monster types, stat indices, and floors. All used in unit tests.
- **Unit tests** — added 28 deterministic unit tests covering all game_balance formulas (XP, HP, damage, dodge, throw range, wait heal, potion heal), equipment/item data table validation, and EquipmentBonus edge cases (NULL world, ENTITY_NONE, missing components, EQUIP_NONE).
- **docs/API.md** — complete public API reference for all modules with signatures, side effects, and preconditions.
- **docs/profilingreport.md** — identified 5 hot-path bottlenecks (World_FindMonsterAt, AISystem_ProcessAll, World_CountAliveMonsters, RenderSystem_DrawMonsters, World_UpdateMonsterAnimations) with complexity analysis and safe optimization recommendations.

---

## [v0.0.7] - 2026-06-01

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

For earlier versions (ALPHA-0.0.1 through ALPHA-0.0.6), see [HISTORY.md](./HISTORY.md).
