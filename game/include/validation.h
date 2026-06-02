#ifndef VALIDATION_H
#define VALIDATION_H

#include "game_types.h"
#include <stdbool.h>

// =============================================================================
// validation.h — Lightweight input validation helpers for API boundaries
//
// All functions are pure predicates or clamps — no side effects, no logging,
// no behaviour changes.  Callers decide how to handle invalid inputs.
// =============================================================================

// --- Inventory & equipment slot validation -----------------------------------
// Returns true if slot is a valid index into an array of slotCount elements.
bool Validate_InventorySlot(int slot, int slotCount);

// Returns true if type is in [EQUIP_NONE+1, EQUIP_COUNT-1].
bool Validate_EquipType(EquipType type);

// Returns true if type is in [ITEM_NONE+1, ITEM_COUNT-1].
bool Validate_ItemType(ItemType type);

// --- Monster validation ------------------------------------------------------
// Returns true if type is in [0, MONSTER_TYPE_COUNT-1].
bool Validate_MonsterType(MonsterType type);

// --- Stat allocation validation ----------------------------------------------
// Returns true if statIdx is 0–4 (STR, DEX, INT, CON, LCK).
bool Validate_StatIndex(int statIdx);

// --- Floor / dungeon validation ----------------------------------------------
// Returns true if floor is positive.
bool Validate_Floor(int floor);

// --- General bounds helpers --------------------------------------------------
// Returns value clamped to [min, max].
int Clamp_Int(int value, int min, int max);

#endif
