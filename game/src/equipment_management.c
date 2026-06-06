#include "equipment_management.h"
#include "equipment_bonus.h"
#include "debug_log.h"
#include "validation.h"
#include "event_bus.h"
#include "game.h"
#include "data/equip_table.h"
#include "systems/spawner_system.h"
#include "systems/world_monster.h"
#include <string.h>

bool EquipItem(GameWorld* game, EquipType type) {
    if (type == EQUIP_NONE) { TraceLog(LOG_WARNING, "EquipItem: EQUIP_NONE"); return false; }
    if (!game) { TraceLog(LOG_WARNING, "EquipItem: NULL game"); return false; }
    if (game->playerEntity == ENTITY_NONE) { TraceLog(LOG_WARNING, "EquipItem: player entity is ENTITY_NONE"); return false; }
    const EquipData* data = GetEquipData(type);
    if (!data) { TraceLog(LOG_WARNING, "EquipItem: unknown equip type [type=%d]", (int)type); return false; }

    int slotIdx = (int)data->slot;
    CStats* ps = World_GetStats(&game->ecs, game->playerEntity);

    if (data->slot == EQUIP_SLOT_WEAPON && IsWeaponDualWieldable(type) &&
        game->equipped[EQUIP_SLOT_WEAPON] != EQUIP_NONE &&
        game->equipped[EQUIP_SLOT_OFF_HAND] == EQUIP_NONE) {
        slotIdx = EQUIP_SLOT_OFF_HAND;
    }

    if (slotIdx == EQUIP_SLOT_OFF_HAND && IsTwoHandedEquipped(game)) {
        return false;
    }

    if (game->equipped[slotIdx] != EQUIP_NONE) {
        UnequipSlot(game, data->slot);
    }

    if (data->twoHanded && game->equipped[EQUIP_SLOT_OFF_HAND] != EQUIP_NONE) {
        UnequipSlot(game, EQUIP_SLOT_OFF_HAND);
    }

    EquipmentBonus_Apply(&game->ecs, game->playerEntity, type);

    game->equipped[slotIdx] = type;

    if (type == EQUIP_BAND_OF_GROWTH)
        game->statCapsRemoved = true;

    if (data->category == EQUIP_CAT_WEAPON && World_HasComponents(&game->ecs, game->playerEntity, COMP_ABILITIES)) {
        CAbilities* a = World_GetAbilities(&game->ecs, game->playerEntity);
        a->abilities[0] = Inventory_GetWeaponAbility(type);
        if (a->count < 1) a->count = 1;
    }

    DebugLog(DEBUG_INVENTORY, "EquipItem: %s slot=%d atk=%d def=%d str=%d dex=%d int=%d con=%d lck=%d",
             data->name, slotIdx, data->bonusAttack, data->bonusDefense, data->bonusStr, data->bonusDex, data->bonusInt, data->bonusCon, data->bonusLck);
    {
        ItemEquippedEvent evt = { type };
        EventBus_Publish(EVT_ITEM_EQUIPPED, &evt);
    }
    return true;
}

void EquipItemSilent(GameWorld* game, EquipType type) {
    if (type == EQUIP_NONE) return;
    const EquipData* data = GetEquipData(type);
    if (!data) return;

    int slotIdx = (int)data->slot;

    if (game->equipped[slotIdx] != EQUIP_NONE) {
        EquipmentBonus_Remove(&game->ecs, game->playerEntity,
                              game->equipped[slotIdx]);
        game->equipped[slotIdx] = EQUIP_NONE;
    }

    EquipmentBonus_Apply(&game->ecs, game->playerEntity, type);

    game->equipped[slotIdx] = type;
}

void UnequipSlot(GameWorld* game, EquipSlot slot) {
    int slotIdx = (int)slot;
    EquipType oldType = game->equipped[slotIdx];
    if (oldType == EQUIP_NONE) return;

    const EquipData* data = GetEquipData(oldType);
    if (!data) return;

    DebugLog(DEBUG_INVENTORY, "UnequipSlot: %s slot=%d", data->name, slotIdx);
    EquipmentBonus_Remove(&game->ecs, game->playerEntity, oldType);

    game->equipped[slotIdx] = EQUIP_NONE;

    if (oldType == EQUIP_BAND_OF_GROWTH)
        game->statCapsRemoved = false;

    if (game->equipInventoryCount < MAX_INVENTORY_SLOTS) {
        AddEquipToInventory(game, oldType);
    } else {
        CPosition* pp = World_GetPosition(&game->ecs, game->playerEntity);
        int dropX = pp->x, dropY = pp->y;
        if (game->blocking[dropY][dropX] || World_FindMonsterAt(game, dropX, dropY, ENTITY_NONE) != ENTITY_NONE) {
            int nx[] = { 0, 0, -1, 1 };
            int ny[] = { -1, 1, 0, 0 };
            for (int d = 0; d < 4; d++) {
                int tx = pp->x + nx[d], ty = pp->y + ny[d];
                if (tx >= 0 && tx < game->map->width && ty >= 0 && ty < game->map->height &&
                    !game->blocking[ty][tx] && World_FindMonsterAt(game, tx, ty, ENTITY_NONE) == ENTITY_NONE) {
                    dropX = tx; dropY = ty; break;
                }
            }
        }
        SpawnerSystem_AddEquipAt(game, dropX, dropY, oldType, 1);
        FloatMsg_Spawn(game, dropX, dropY, YELLOW, "No room - item dropped!");
    }
    {
        ItemUnequippedEvent evt = { oldType };
        EventBus_Publish(EVT_ITEM_UNEQUIPPED, &evt);
    }
}

bool IsEquipSlotOccupied(const GameWorld* game, EquipSlot slot) {
    return game->equipped[(int)slot] != EQUIP_NONE;
}

bool IsTwoHandedEquipped(const GameWorld* game) {
    EquipType weapon = game->equipped[EQUIP_SLOT_WEAPON];
    if (weapon == EQUIP_NONE) return false;
    const EquipData* data = GetEquipData(weapon);
    return data && data->twoHanded;
}

bool IsWeaponDualWieldable(EquipType type) {
    const EquipData* d = GetEquipData(type);
    if (!d || d->type == EQUIP_NONE) return false;
    return !d->twoHanded && !d->isRanged && d->category == EQUIP_CAT_WEAPON;
}

bool IsDualWielding(const GameWorld* game) {
    if (!game) return false;
    EquipType mainWeapon = game->equipped[EQUIP_SLOT_WEAPON];
    EquipType offWeapon = game->equipped[EQUIP_SLOT_OFF_HAND];
    if (mainWeapon == EQUIP_NONE || offWeapon == EQUIP_NONE) return false;
    const EquipData* md = GetEquipData(mainWeapon);
    if (!md || md->twoHanded || md->isRanged) return false;
    return IsWeaponDualWieldable(offWeapon);
}

bool AddEquipToInventory(GameWorld* game, EquipType type) {
    if (type == EQUIP_NONE) { TraceLog(LOG_WARNING, "AddEquipToInventory: EQUIP_NONE"); return false; }
    if (type >= EQUIP_COUNT) { TraceLog(LOG_WARNING, "AddEquipToInventory: type out of range [type=%d]", (int)type); return false; }
    if (game->equipInventoryCount >= MAX_INVENTORY_SLOTS) { TraceLog(LOG_WARNING, "AddEquipToInventory: inventory full [count=%d]", game->equipInventoryCount); return false; }
    game->equipInventory[game->equipInventoryCount++] = type;
    return true;
}

bool RemoveEquipFromInventory(GameWorld* game, int slot) {
    if (slot < 0 || slot >= game->equipInventoryCount) { TraceLog(LOG_WARNING, "RemoveEquipFromInventory: slot out of range [slot=%d count=%d]", slot, game->equipInventoryCount); return false; }
    for (int i = slot; i < game->equipInventoryCount - 1; i++)
        game->equipInventory[i] = game->equipInventory[i + 1];
    game->equipInventoryCount--;
    return true;
}
