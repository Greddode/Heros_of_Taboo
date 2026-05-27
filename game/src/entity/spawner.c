#include "spawner.h"
#include "monster.h"
#include <stdlib.h>

// Minimum squared distance from the player's spawn for newly placed entities
#define MIN_PLAYER_DIST 5

// Unified loot entry: can be a potion or equipment
typedef enum {
    LOOT_TYPE_POTION,
    LOOT_TYPE_EQUIP
} LootCategory;

typedef struct {
    LootCategory cat;
    int typeId;        // ItemType for potions, EquipType for equipment
    int tier;          // 1=common, 2=uncommon, 3=rare, 4=legendary
    int baseWeight;
} LootEntry;

// Unified loot table: potions and equipment in one pool by rarity tier
static const LootEntry LOOT_TABLE[] = {
    // Tier 1 (Common) — base weight 15
    { LOOT_TYPE_POTION, (int)ITEM_SMALL_HP_POTION,  1, 15 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_LEATHER_CAP,     1, 12 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_LEATHER_VEST,    1, 12 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_WOODEN_SHIELD,   1, 12 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_DAGGER,          1, 10 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_RING_OF_STRENGTH,1, 10 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_BOOTS_OF_SWIFTNESS,1, 10 },

    // Tier 2 (Uncommon) — base weight 10
    { LOOT_TYPE_POTION, (int)ITEM_MEDIUM_HP_POTION, 2, 12 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_IRON_HELM,       2, 9 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_CHAIN_MAIL,      2, 9 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_IRON_SWORD,      2, 9 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_IRON_SHIELD,     2, 9 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_AMULET_OF_WARDING,2, 7 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_RING_OF_THE_HAWK,2, 7 },

    // Tier 3 (Rare) — base weight 6
    { LOOT_TYPE_POTION, (int)ITEM_LARGE_HP_POTION,  3, 9 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_STEEL_HELM,      3, 6 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_PLATE_MAIL,      3, 6 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_STEEL_SWORD,     3, 6 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_STEEL_SHIELD,    3, 6 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_SAGES_PENDANT,   3, 6 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_LUCKY_CHARM,     3, 5 },

    // Tier 4 (Legendary) — base weight 3
    { LOOT_TYPE_EQUIP,  (int)EQUIP_WAR_HAMMER,      4, 4 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_BERSERKER_BAND,  4, 3 },
};
#define LOOT_TABLE_COUNT (sizeof(LOOT_TABLE) / sizeof(LOOT_TABLE[0]))

static int DistSq(int ax, int ay, int bx, int by) {
    int dx = ax - bx;
    int dy = ay - by;
    return dx * dx + dy * dy;
}

// Populate each room with monsters and health potions.
// Uses shuffled floor tile indices so placement is randomised.
void Spawner_Populate(Game* game, const ProceduralRoom* rooms, int roomCount) {
    if (!game || !game->map || roomCount == 0) return;

    int* tiles = game->map->layers[0].data;
    int w = game->map->width;
    int h = game->map->height;

    int px = game->player.ent.x;
    int py = game->player.ent.y;
    int minDist = MIN_PLAYER_DIST * MIN_PLAYER_DIST;

    // Upper bound: each room is at most 10x10 = 100 tiles
    #define ROOM_MAX_FLOORS 256

    for (int r = 0; r < roomCount; r++) {
        // Collect all floor-tile indices inside this room
        int floorTiles[ROOM_MAX_FLOORS];
        int floorCount = 0;

        for (int y = rooms[r].y; y < rooms[r].y + rooms[r].h; y++)
            for (int x = rooms[r].x; x < rooms[r].x + rooms[r].w; x++)
                if (IsFloorGID(tiles[y * w + x]) && tiles[y * w + x] != TILE_STAIRS && floorCount < ROOM_MAX_FLOORS)
                    floorTiles[floorCount++] = y * w + x;

        if (floorCount < 4) continue;

        // Fisher-Yates shuffle so placement order is random
        for (int i = floorCount - 1; i > 0; i--) {
            int j = GetRandomValue(0, i);
            int t = floorTiles[i]; floorTiles[i] = floorTiles[j]; floorTiles[j] = t;
        }

        // --- Monsters: bigger rooms get more enemies ---
        int monsterCount = 1;
        if (floorCount >= 20) monsterCount = 2;
        if (floorCount >= 40) monsterCount = 3;

        if (monsterCount > floorCount) monsterCount = floorCount;

        for (int m = 0; m < monsterCount; m++) {
            int mx = floorTiles[m] % w;
            int my = floorTiles[m] / w;

            // Don't spawn too close to the player
            if (DistSq(mx, my, px, py) < minDist) continue;
            if (Monster_GetAt(mx, my, NULL)) continue;

            // Weighted random monster selection, filtered by floor depth
            MonsterType type = MONSTER_BAT; // fallback
            float totalWeight = 0;
            float monsterWeights[MONSTER_TYPE_COUNT];
            int floor = game->currentFloor;

            for (int t = 0; t < MONSTER_TYPE_COUNT; t++) {
                const MonsterTemplate* tpl = Monster_GetTemplate((MonsterType)t);
                if (!tpl || tpl->spawnWeight <= 0 || floor < tpl->minFloor) {
                    monsterWeights[t] = 0;
                    continue;
                }
                monsterWeights[t] = (float)tpl->spawnWeight;
                totalWeight += monsterWeights[t];
            }

            if (totalWeight > 0) {
                float roll = (float)GetRandomValue(0, (int)(totalWeight * 100)) / 100.0f;
                float acc = 0;
                for (int t = 0; t < MONSTER_TYPE_COUNT; t++) {
                    if (monsterWeights[t] <= 0) continue;
                    acc += monsterWeights[t];
                    if (roll <= acc) {
                        type = (MonsterType)t;
                        break;
                    }
                }
            }

            Monster_Spawn(type, mx, my, floor);
        }

        // --- Unified loot drop: roll for 1-2 items in larger rooms ---
        int lootSlots = (floorCount >= 60) ? 2 : 1;
        for (int li = 0; li < lootSlots; li++) {
            int fi = floorCount - 1 - li;
            if (fi < 0) break;
            int lx = floorTiles[fi] % w;
            int ly = floorTiles[fi] / w;
            if (DistSq(lx, ly, px, py) < minDist) continue;

            // Luck factor: each point of LCK adds a flat rarity bonus
            // Floor factor: deeper floors boost higher tiers
            // Effective weight for each tier: baseWeight + luckBonus + floorBonus
            int floor = game->currentFloor;
            int luck = game->player.ent.lck;

            // Minimum floor requirements per tier:
            // Tier 3 (rare)  = floor 4+
            // Tier 4 (legendary) = floor 6+
            int tierMinFloor[4] = { 1, 1, 4, 6 };

            // Build weighted tier quantities then pick a tier.
            // Higher tiers get bigger floor + luck bonuses.
            // tierThresholds[tier-1] = total effective weight for that tier (all entries summed)
            float tierWeights[4] = { 0, 0, 0, 0 };
            for (int te = 0; te < (int)LOOT_TABLE_COUNT; te++) {
                const LootEntry* entry = &LOOT_TABLE[te];
                int ti = entry->tier - 1; // 0=common, 1=uncommon, 2=rare, 3=legendary

                // Hard floor requirement: skip if below minimum floor for this tier
                if (floor < tierMinFloor[ti]) continue;

                // Luck bonus: +2 per tier per LCK point
                float luckBonus = (float)luck * 2.0f * (float)(ti + 1);
                // Floor bonus: +3 per tier per floor
                float floorBonus = (float)floor * 3.0f * (float)(ti + 1);
                float effectiveWeight = (float)entry->baseWeight + luckBonus + floorBonus;
                if (effectiveWeight < 0) effectiveWeight = 0;
                tierWeights[ti] += effectiveWeight;
            }

            // Roll which tier
            float tierTotal = tierWeights[0] + tierWeights[1] + tierWeights[2] + tierWeights[3];
            if (tierTotal <= 0) continue;

            float tierRoll = (float)GetRandomValue(0, (int)(tierTotal * 100)) / 100.0f;
            int chosenTier = 0;
            float accTier = 0;
            for (int ti = 0; ti < 4; ti++) {
                accTier += tierWeights[ti];
                if (tierRoll <= accTier) {
                    chosenTier = ti + 1;
                    break;
                }
            }
            if (chosenTier == 0) chosenTier = 1; // fallback to common

            // Collect all entries of the chosen tier
            int tierEntryCount = 0;
            int tierEntryIndices[32];
            float tierEntryBaseWeights[32];
            float tierEntryTotal = 0;
            for (int te = 0; te < (int)LOOT_TABLE_COUNT; te++) {
                if (LOOT_TABLE[te].tier == chosenTier && tierEntryCount < 32) {
                    tierEntryIndices[tierEntryCount] = te;
                    float fw = (float)LOOT_TABLE[te].baseWeight;
                    tierEntryBaseWeights[tierEntryCount] = fw;
                    tierEntryTotal += fw;
                    tierEntryCount++;
                }
            }
            if (tierEntryCount == 0) continue;

            // Roll which specific item within tier (base weight only for variety)
            float itemRoll = (float)GetRandomValue(0, (int)(tierEntryTotal * 100)) / 100.0f;
            float accItem = 0;
            int chosenEntryIdx = 0;
            for (int te = 0; te < tierEntryCount; te++) {
                accItem += tierEntryBaseWeights[te];
                if (itemRoll <= accItem) {
                    chosenEntryIdx = tierEntryIndices[te];
                    break;
                }
            }

            const LootEntry* chosen = &LOOT_TABLE[chosenEntryIdx];

            // Place the loot on the map
            if (chosen->cat == LOOT_TYPE_POTION && game->potionCount < MAX_POTIONS) {
                game->potionTiles[game->potionCount][0] = lx;
                game->potionTiles[game->potionCount][1] = ly;
                game->potionCollected[game->potionCount] = false;
                game->potionQuantities[game->potionCount] = 1;
                game->potionTypes[game->potionCount] = (ItemType)chosen->typeId;
                game->potionCount++;
            } else if (chosen->cat == LOOT_TYPE_EQUIP && game->equipMapCount < MAX_EQUIP_ON_MAP) {
                game->equipMapTiles[game->equipMapCount][0] = lx;
                game->equipMapTiles[game->equipMapCount][1] = ly;
                game->equipMapCollected[game->equipMapCount] = false;
                game->equipMapTypes[game->equipMapCount] = (EquipType)chosen->typeId;
                game->equipMapQuantities[game->equipMapCount] = 1;
                game->equipMapCount++;
            }
        }
    }

    #undef ROOM_MAX_FLOORS

    TraceLog(LOG_INFO, "Spawner: %d monsters, %d health potions, %d equipment placed",
             Monster_GetAliveCount(), game->potionCount, game->equipMapCount);
}
