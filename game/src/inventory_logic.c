#include "inventory_logic.h"
#include "inventory.h"
#include "debug_log.h"
#include "validation.h"
#include "game.h"
#include "resources.h"
#include "game_balance.h"
#include "data/item_data.h"
#include "data/equip_table.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

bool InventoryAdd(GameWorld* game, ItemType type) {
    if (type == ITEM_NONE) { TraceLog(LOG_WARNING, "InventoryAdd: ITEM_NONE"); return false; }
    if (type >= ITEM_COUNT) { TraceLog(LOG_WARNING, "InventoryAdd: type out of range [type=%d]", (int)type); return false; }
    for (int i = 0; i < game->inventorySlotCount; i++) {
        if (game->inventory[i].type == type) {
            game->inventory[i].quantity++;
            return true;
        }
    }
    if (game->inventorySlotCount >= MAX_INVENTORY_SLOTS) return false;
    game->inventory[game->inventorySlotCount].type = type;
    game->inventory[game->inventorySlotCount].quantity = 1;
    game->inventorySlotCount++;
    return true;
}

bool InventoryUse(GameWorld* game, int slot) {
    if (!game) { TraceLog(LOG_WARNING, "InventoryUse: NULL game"); return false; }
    if (game->playerEntity == ENTITY_NONE) { TraceLog(LOG_WARNING, "InventoryUse: player entity is ENTITY_NONE"); return false; }
    if (slot < 0 || slot >= game->inventorySlotCount) { TraceLog(LOG_WARNING, "InventoryUse: slot out of range [slot=%d count=%d]", slot, game->inventorySlotCount); return false; }
    InventorySlot* s = &game->inventory[slot];
    if (s->type == ITEM_NONE || s->quantity <= 0) { TraceLog(LOG_WARNING, "InventoryUse: slot empty [slot=%d]", slot); return false; }

    int healPercent = GetItemHealAmount(s->type);
    if (healPercent > 0) {
        CStats* ps = World_GetStats(&game->ecs, game->playerEntity);
        int oldHP = ps->hp;
        int heal = calc_potion_heal(ps->maxHp, healPercent, ps->intel);
        ps->hp += heal;
        if (ps->hp > ps->maxHp)
            ps->hp = ps->maxHp;
        DebugLog(DEBUG_INVENTORY, "UsePotion: type=%d HP=%d->%d heal=%d intelBonus=%d",
                 (int)s->type, oldHP, ps->hp, ps->hp - oldHP, (int)ps->intel);
        {
            CPosition* pp = World_GetPosition(&game->ecs, game->playerEntity);
            int tw = game->map->tileWidth;
            int th = game->map->tileHeight;
            DamageNumber_Spawn(&game->damageNumbers, heal, pp->x, pp->y, tw, th, GREEN);
        }
    }

    s->quantity--;
    if (s->quantity <= 0) {
        for (int i = slot; i < game->inventorySlotCount - 1; i++)
            game->inventory[i] = game->inventory[i + 1];
        game->inventorySlotCount--;
    }
    return true;
}

Texture2D* Inventory_LoadEquipTexture(EquipType type) {
    const EquipData* d = GetEquipData(type);
    if (!d || !d->spritePath || d->spritePath[0] == '\0') return NULL;
    return Resources_LoadTexture(d->spritePath);
}

Texture2D* Inventory_LoadPotionTexture(ItemType type) {
    if (type <= ITEM_NONE || type >= ITEM_COUNT) return NULL;
    const char* sprite = GetItemSpritePath(type);
    if (!sprite || sprite[0] == '\0') return NULL;
    return Resources_LoadTexture(sprite);
}

AbilityType Inventory_GetWeaponAbility(EquipType weapon) {
    switch (weapon) {
        case EQUIP_NONE:             return ABILITY_PUNCH;
        case EQUIP_SURVIVAL_KNIFE:
        case EQUIP_DAGGER:
        case EQUIP_IRON_SWORD:       return ABILITY_LIGHT_MELEE;
        case EQUIP_STEEL_SWORD:
        case EQUIP_WAR_HAMMER:       return ABILITY_HEAVY_MELEE;
        case EQUIP_SIMPLE_BOW:
        case EQUIP_ELVEN_BOW:        return ABILITY_RANGED;
        default:                     return ABILITY_PUNCH;
    }
}

void LoadPotionTextures(GameWorld* game) {
    (void)game;
    for (int i = 1; i < ITEM_COUNT; i++) {
        const char* sprite = GetItemSpritePath((ItemType)i);
        if (sprite && sprite[0]) {
            Resources_LoadTexture(sprite);
        }
    }
    Resources_LoadTexture("resources/sprites/ui/UI_Flat_Frame01a.png");
    Resources_LoadTexture("resources/sprites/ui/UI_Flat_FrameSlot01b.png");
    Resources_LoadTexture("resources/sprites/ui/UI_Flat_FrameMarker01a.png");
    Resources_LoadTexture("resources/sprites/items/loot.png");
}
