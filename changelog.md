# Changelog

All notable changes to this project will be documented in this file.

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
