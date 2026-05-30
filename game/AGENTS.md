# Step 9 — ECS Player & Field Migration (complete)

## Goal
Remove `Player player` and all 25+ duplicate fields from `Game` struct. Delete sync functions. `Game` becomes a thin bridge holding only UI textures + `GameWorld ecsWorld`.

## Changes
| File | Change |
|------|--------|
| `game.h` | Removed `Player player`, `turnCount`, `camera`, `visibility`, `blocking`, `combatLog`, all `anim*`, `sprint*`, `projectile*`, `currentFloor`, `maxFloors`, `stairX/Y`, `escapeX/Y`, `timeWaited`, `enemyTurnCooldown`, `levelUpTimer`, `floorClearedNotified`, `escapeSpawned`, `animatingEnemyTurn`, `selectedMonsterEntity`, `inventory`, `inventorySlotCount`, `equipped`, `equipInventory`, `equipInventoryCount`. Game now has only: `texUiFrame/Slot/Marker/Loot`, `potionTextures`, `magicAttacksTexture`, `selectedPotionTileX/Y/Active`, `ecsWorld`. |
| `game.c` | Deleted `SyncPlayerToEcs`, `SyncPlayerFromEcs`, `SyncGameWorldFromGame`. Rewrote `UpdateGame`/`DescendFloor`/`InitGame` with ECS accessors + `game->ecsWorld.X`. |
| `renderer.c` | Replaced `game->player.ent.*` with `World_Get*`. All task-2 fields via `gw->`. Fixed `InventoryUI_Render` call. |
| `inventory_ui.c` | Replaced all `game->player.ent.*` → ECS accessors. Replaced all migrated fields → `game->ecsWorld.*`. |
| `inventory_ui.h` | Fixed `InventoryUI_Render` signature from `GameWorld*` to `Game*` (needs `game->texUi*`). |
| `map_helpers.c` | Replaced player pos/level/visibility/blocking/escapeX/Y → ECS + `game->ecsWorld.*`. |
| `player.h` | Removed `#include "entity.h"`. `AllocateStatPoint` takes `CStats*`. Removed `Player` typedef. |
| `player.c` | `AllocateStatPoint(CStats*, int)`. `ApplyLevelUp`/`GainExperience` use ECS accessors. |
| `combat_system.h/c` | `CombatSystem_PlayerMeleeAttack` takes `EntityId`, not `Entity*`. Uses `World_GetStats`. |
| `inspector.c` | `game->selectedMonsterEntity` → `game->ecsWorld.*`. Pre-existing breaks untouched. |
| `main.c` | `game.camera.target` → `game.ecsWorld.camera.target` (camera moved to GameWorld). |

## Task 1-4 fixes (applied May 2026)
| Task | File | Change |
|------|------|--------|
| 1 | `renderer.c` | Fixed CSpriteAnim field: `sa->spriteSheet` → `*sa->tex`, `sa->spriteSheet.width` → `sa->tex->width`, `sa->spriteSheet.height` → `sa->tex->height`, `sa->animFrame` → `sa->frame`. |
| 2 | `game.c` | No change needed (Task 5 changed header to `Game*`, HandleInput already passes `Game*`). |
| 3 | `inspector.h/c`, `inventory_ui.c` | Removed `game->ecsWorld.inventorySelection`. Added `int selectedSlot` param to `Inspector_Render`. Caller passes `ui->selection`. |
| 4 | `player.c` | Deleted `ps->expValue += amount;` from `GainExperience` (expValue is monster-only). |
| 5 | `input_system.h/c`, `inventory_ui.c`, `main.c` | Changed `input_system.h` to `Game*` (with TODO). Moved correct implementations from `inventory_ui.c` to `input_system.c`. Removed duplicate `InputSystem_*` defs from `inventory_ui.c`. Updated `main.c` from `&game.ecsWorld` to `&game`. |

## Remaining pre-existing breaks (not in scope)
- `inventory.c`: Entire file uses old `game->` API. Already excluded from refactoring. Pre-existing break.
- `legacy inventory.c` calls `Inspector_Render` with 6 params instead of 7 — not compiled.

## Key decisions
- `Game*` retained in `inventory_ui.c` signatures (needs `game->texUi*`).
- `AllocateStatPoint` now takes `CStats*` (decoupled from `Entity`).
- `CombatSystem_PlayerMeleeAttack` now takes `EntityId`.
- Player saved/restored via ECS components across floor transitions.
- `InputSystem_*` kept as `Game*` (not `GameWorld*`) with TODO to switch in step 10 — inventory functions need `Game*` for texture access.
