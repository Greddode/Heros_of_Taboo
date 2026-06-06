#include "data/equip_data.h"
#include "game_balance.h"

int Equip_GetPowerScore(const EquipData* d)
{
    if (!d || d->type == 0) return 0;
    return d->bonusAttack * 2 + d->bonusDefense * 2 +
           d->bonusStr + d->bonusDex + d->bonusCon + d->bonusInt + d->bonusLck;
}

int Equip_GetBasePrice(const EquipData* d)
{
    if (!d || d->type == 0) return 0;
    int power = Equip_GetPowerScore(d);
    int bonus;
    switch (d->rarity) {
        case RARITY_COMMON:    bonus = GOLD_RARITY_BONUS_COMMON;    break;
        case RARITY_UNCOMMON:  bonus = GOLD_RARITY_BONUS_UNCOMMON;  break;
        case RARITY_RARE:      bonus = GOLD_RARITY_BONUS_RARE;      break;
        case RARITY_LEGENDARY: bonus = GOLD_RARITY_BONUS_LEGENDARY; break;
        default:               bonus = 0;                           break;
    }
    return power * 10 + bonus;
}

int Equip_GetSellPrice(const EquipData* d)
{
    return (int)((float)Equip_GetBasePrice(d) * GOLD_SELL_RATIO);
}
