// test_runner.c — Lightweight C test harness for Heroes of Taboo
//
// Runs all unit tests and returns 0 on success, non-zero on failure.
// Build: see Makefile test target
// Run:   ./test_runner

#include "game_balance.h"
#include "equipment_bonus.h"
#include "inventory.h"
#include "validation.h"
#include "world.h"
#include "ecs.h"
#include "resources.h"
#include "systems/world_monster.h"
#include "systems/spatial_hash.h"
#include "data/monster_data.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define TEST_PASS()  printf("  PASS  %s\n", __func__)
#define TEST_FAIL(msg) do { printf("  FAIL  %s: %s\n", __func__, msg); return false; } while(0)
#define CHECK(cond, msg) do { if (!(cond)) TEST_FAIL(msg); } while(0)
#define CHECK_EQ(a, b, msg) do { if ((a) != (b)) { printf("  FAIL  %s: %s (got %d, expected %d)\n", __func__, msg, (int)(a), (int)(b)); return false; } } while(0)

// =============================================================================
// Stat calculation tests
// =============================================================================

static bool test_xp_curve(void)
{
    CHECK_EQ(calc_xp_for_level(1),  90,  "XP for level 1");
    CHECK_EQ(calc_xp_for_level(2),  130, "XP for level 2");
    CHECK_EQ(calc_xp_for_level(10), 450, "XP for level 10");
    TEST_PASS();
    return true;
}

static bool test_max_hp(void)
{
    CHECK_EQ(calc_max_hp(3),  45,  "max HP with CON=3");
    CHECK_EQ(calc_max_hp(0),  30,  "max HP with CON=0");
    CHECK_EQ(calc_max_hp(10), 80,  "max HP with CON=10");
    CHECK_EQ(calc_max_hp(-1), 25,  "max HP with CON=-1 (should handle gracefully)");
    TEST_PASS();
    return true;
}

static bool test_melee_damage(void)
{
    // attack + str * STR_MELEE_MULT (2)
    CHECK_EQ(calc_melee_damage(5, 3),  11, "melee: atk=5, str=3");
    CHECK_EQ(calc_melee_damage(0, 0),   0, "melee: atk=0, str=0");
    CHECK_EQ(calc_melee_damage(10, 5), 20, "melee: atk=10, str=5");
    TEST_PASS();
    return true;
}

static bool test_ranged_damage(void)
{
    // dex * DEX_RANGED_MULT (1.5)
    CHECK_EQ(calc_ranged_damage(3), 4, "ranged: dex=3 -> 4.5 cast to 4");
    CHECK_EQ(calc_ranged_damage(4), 6, "ranged: dex=4 -> 6.0");
    CHECK_EQ(calc_ranged_damage(0), 0, "ranged: dex=0");
    TEST_PASS();
    return true;
}

static bool test_throw_damage(void)
{
    // attack + dex * DEX_THROW_MULT (2)
    CHECK_EQ(calc_throw_damage(5, 3),  11, "throw: atk=5, dex=3");
    CHECK_EQ(calc_throw_damage(0, 5),  10, "throw: atk=0, dex=5");
    TEST_PASS();
    return true;
}

static bool test_magic_damage_and_resist(void)
{
    CHECK_EQ(calc_magic_damage(4, 10), 14, "magic damage: atk=4, int=10");
    CHECK_EQ(calc_magic_resist(3),      9, "magic resist: int=3 -> 3*3=9");
    CHECK_EQ(calc_magic_resist(10),    30, "magic resist: int=10");
    TEST_PASS();
    return true;
}

static bool test_damage_after_defense(void)
{
    CHECK_EQ(calc_damage_after_defense(10, 5), 5, "10 dmg - 5 def = 5");
    CHECK_EQ(calc_damage_after_defense(1,  5), 1, "1 dmg - 5 def => min 1");
    CHECK_EQ(calc_damage_after_defense(5,  5), 1, "5 dmg - 5 def => min 1");
    CHECK_EQ(calc_damage_after_defense(0,  0), 1, "0 dmg - 0 def => min 1");
    CHECK_EQ(calc_damage_after_defense(20, 0), 20, "20 dmg - 0 def = 20");
    TEST_PASS();
    return true;
}

static bool test_dodge_chance(void)
{
    CHECK_EQ(calc_dodge_chance(0),   0, "dodge: dex=0");
    CHECK_EQ(calc_dodge_chance(5),  10, "dodge: dex=5 -> 5*2=10");
    CHECK_EQ(calc_dodge_chance(30), 60, "dodge: dex=30 -> capped at 60");
    CHECK_EQ(calc_dodge_chance(50), 60, "dodge: dex=50 -> capped at 60");
    TEST_PASS();
    return true;
}

static bool test_throw_range(void)
{
    CHECK_EQ(calc_throw_range(0), 3, "throw range: dex=0 -> min 3");
    CHECK_EQ(calc_throw_range(6), 5, "throw range: dex=6 -> 3+2=5");
    CHECK_EQ(calc_throw_range(9), 6, "throw range: dex=9 -> 3+3=6");
    TEST_PASS();
    return true;
}

static bool test_wait_heal(void)
{
    CHECK_EQ(calc_wait_heal(0), 1, "wait heal: intel=0 -> 1");
    CHECK_EQ(calc_wait_heal(4), 3, "wait heal: intel=4 -> 1+2=3");
    CHECK_EQ(calc_wait_heal(10), 6, "wait heal: intel=10 -> 1+5=6");
    TEST_PASS();
    return true;
}

static bool test_potion_heal(void)
{
    int result;

    // intel=3 => intMult = 1.0 + 3*0.02 = 1.06
    // heal = (100 * 25 * 106) / 10000 = 265000/10000 = 26
    result = calc_potion_heal(100, 25, 3);
    CHECK(result >= 25 && result <= 27, "potion heal range");

    // intel=0 => intMult = 1.0
    // heal = (100 * 50 * 100) / 10000 = 500000/10000 = 50
    result = calc_potion_heal(100, 50, 0);
    CHECK_EQ(result, 50, "potion heal: maxHp=100, 50%, intel=0");

    // intel=50 => intMult = 2.0
    // heal = (100 * 75 * 200) / 10000 = 1500000/10000 = 150
    result = calc_potion_heal(100, 75, 50);
    CHECK_EQ(result, 150, "potion heal: high intel doubles effect");

    // Small HP, should be at least 1
    result = calc_potion_heal(1, 25, 0);
    CHECK(result >= 1, "potion heal: minimum 1 HP");

    TEST_PASS();
    return true;
}

// =============================================================================
// Inventory / equipment data tests
// =============================================================================

static bool test_equip_data_table(void)
{
    // Validate bounds
    const EquipData* none = GetEquipData(EQUIP_NONE);
    CHECK(none != NULL && none->type == EQUIP_NONE, "EQUIP_NONE entry exists");

    const EquipData* last = GetEquipData(EQUIP_COUNT - 1);
    CHECK(last != NULL, "last equip entry exists");

    const EquipData* oob = GetEquipData(EQUIP_COUNT);
    CHECK(oob == NULL, "out-of-bounds returns NULL");

    const EquipData* oob2 = GetEquipData(-1);
    CHECK(oob2 == NULL, "negative index returns NULL");

    // Verify a known item
    const EquipData* knife = GetEquipData(EQUIP_SURVIVAL_KNIFE);
    CHECK(knife != NULL, "survival knife exists");
    CHECK_EQ(knife->bonusAttack, 2, "survival knife +2 ATK");
    CHECK_EQ(knife->slot, EQUIP_SLOT_WEAPON, "knife is weapon slot");

    // Verify a two-handed weapon
    const EquipData* hammer = GetEquipData(EQUIP_WAR_HAMMER);
    CHECK(hammer != NULL, "war hammer exists");
    CHECK(hammer->twoHanded, "war hammer is two-handed");

    // Verify a ranged weapon
    const EquipData* bow = GetEquipData(EQUIP_SIMPLE_BOW);
    CHECK(bow != NULL, "simple bow exists");
    CHECK(bow->isRanged, "simple bow is ranged");
    CHECK_EQ(GetEquipRangeBonus(EQUIP_SIMPLE_BOW), 4, "simple bow range=4");

    // Verify range bonus for non-ranged weapon
    CHECK_EQ(GetEquipRangeBonus(EQUIP_SURVIVAL_KNIFE), 0, "knife range=0");

    TEST_PASS();
    return true;
}

static bool test_item_data_table(void)
{
    CHECK(strcmp(GetItemName(ITEM_NONE), "") == 0, "ITEM_NONE name is empty");
    CHECK(strcmp(GetItemName(ITEM_SMALL_HP_POTION), "Small HP Potion") == 0, "small potion name");
    CHECK_EQ(GetItemHealAmount(ITEM_SMALL_HP_POTION), 25, "small potion heals 25%");
    CHECK_EQ(GetItemHealAmount(ITEM_MEDIUM_HP_POTION), 50, "medium potion heals 50%");
    CHECK_EQ(GetItemHealAmount(ITEM_LARGE_HP_POTION), 75, "large potion heals 75%");

    // Out of bounds
    CHECK_EQ(GetItemHealAmount(ITEM_COUNT), 0, "OOB item heal is 0");
    CHECK_EQ(GetItemHealAmount(-1), 0, "negative item heal is 0");

    TEST_PASS();
    return true;
}

// =============================================================================
// Equipment bonus tests (with ECS)
// =============================================================================

static bool test_equipment_bonus_apply_remove(void)
{
    GameWorld gw;
    GameWorld_Init(&gw);

    // Create a test entity with ONLY stats
    EntityId e = World_CreateEntity(&gw.ecs);
    CHECK(e != ENTITY_NONE, "entity creation");

    World_AddComponent(&gw.ecs, e, COMP_STATS);
    CStats* s = World_GetStats(&gw.ecs, e);
    s->str   = 0;
    s->dex   = 0;
    s->intel = 0;
    s->con   = 3;
    s->lck   = 0;
    s->attack  = 0;
    s->defense = 0;
    s->maxHp   = calc_max_hp(3);
    s->hp      = s->maxHp;

    // --- Apply ---
    EquipmentBonus_Apply(&gw.ecs, e, EQUIP_SURVIVAL_KNIFE);
    CHECK_EQ(s->attack, 2,  "after apply knife: atk=2");
    CHECK_EQ(s->defense, 0, "after apply knife: def=0");
    CHECK_EQ(s->maxHp, calc_max_hp(3), "maxHp unchanged (no CON bonus from knife)");

    // Apply accessory with STR bonus
    EquipmentBonus_Apply(&gw.ecs, e, EQUIP_RING_OF_STRENGTH);
    CHECK_EQ(s->str, 3, "after apply ring: str=3");
    CHECK_EQ(s->attack, 2, "attack unchanged by ring");

    // Apply chest piece with CON+DEF
    EquipmentBonus_Apply(&gw.ecs, e, EQUIP_LEATHER_VEST);
    CHECK_EQ(s->defense, 2, "after apply vest: def=2");
    CHECK_EQ(s->con, 5,     "after apply vest: con=5 (base 3+2)");
    CHECK_EQ(s->maxHp, calc_max_hp(5), "maxHp recalculated after CON change");

    // --- Remove ---
    EquipmentBonus_Remove(&gw.ecs, e, EQUIP_SURVIVAL_KNIFE);
    CHECK_EQ(s->attack, 0, "after remove knife: atk back to 0");

    EquipmentBonus_Remove(&gw.ecs, e, EQUIP_RING_OF_STRENGTH);
    CHECK_EQ(s->str, 0, "after remove ring: str back to 0");

    EquipmentBonus_Remove(&gw.ecs, e, EQUIP_LEATHER_VEST);
    CHECK_EQ(s->defense, 0, "after remove vest: def=0");
    CHECK_EQ(s->con, 3,     "after remove vest: con back to 3");
    CHECK_EQ(s->maxHp, calc_max_hp(3), "maxHp recalculated");

    // --- Idempotent: remove non-equipped item should be safe ---
    EquipmentBonus_Remove(&gw.ecs, e, EQUIP_NONE);
    EquipmentBonus_Remove(&gw.ecs, e, EQUIP_STEEL_HELM);
    CHECK_EQ(s->attack, 0, "stats unchanged after spurious removes");

    // --- Apply NULL / invalid ---
    EquipmentBonus_Apply(NULL, e, EQUIP_SURVIVAL_KNIFE);
    EquipmentBonus_Apply(&gw.ecs, ENTITY_NONE, EQUIP_SURVIVAL_KNIFE);
    CHECK_EQ(s->attack, 0, "stats unchanged after apply to NULL world/entity");

    TEST_PASS();
    return true;
}

static bool test_equipment_bonus_recalculate(void)
{
    GameWorld gw;
    GameWorld_Init(&gw);

    EntityId e = World_CreateEntity(&gw.ecs);
    CHECK(e != ENTITY_NONE, "entity creation");

    World_AddComponent(&gw.ecs, e, COMP_STATS);
    CStats* s = World_GetStats(&gw.ecs, e);
    s->con   = 5;
    s->maxHp = 0;
    s->hp    = 100;

    EquipmentBonus_Recalculate(&gw.ecs, e);
    CHECK_EQ(s->maxHp, calc_max_hp(5), "maxHp recalculated from CON");
    CHECK_EQ(s->hp, calc_max_hp(5), "hp clamped to new maxHp");

    // Test with NULL / ENTITY_NONE
    EquipmentBonus_Recalculate(NULL, e);
    EquipmentBonus_Recalculate(&gw.ecs, ENTITY_NONE);
    CHECK_EQ(s->maxHp, calc_max_hp(5), "unchanged after recalculate on NULL");

    TEST_PASS();
    return true;
}

static bool test_equipment_bonus_edge_cases(void)
{
    GameWorld gw;
    GameWorld_Init(&gw);

    // Entity without CStats — Apply/Remove should be no-ops
    EntityId e = World_CreateEntity(&gw.ecs);
    CHECK(e != ENTITY_NONE, "entity creation");

    // Should not crash
    EquipmentBonus_Apply(&gw.ecs, e, EQUIP_SURVIVAL_KNIFE);
    EquipmentBonus_Remove(&gw.ecs, e, EQUIP_SURVIVAL_KNIFE);
    EquipmentBonus_Recalculate(&gw.ecs, e);

    // Apply EQUIP_NONE should be no-op
    EntityId e2 = World_CreateEntity(&gw.ecs);
    World_AddComponent(&gw.ecs, e2, COMP_STATS);
    CStats* s2 = World_GetStats(&gw.ecs, e2);
    int conBefore = s2->con;

    EquipmentBonus_Apply(&gw.ecs, e2, EQUIP_NONE);
    EquipmentBonus_Remove(&gw.ecs, e2, EQUIP_NONE);
    CHECK_EQ(s2->con, conBefore, "EQUIP_NONE apply/remove is no-op");

    TEST_PASS();
    return true;
}

// =============================================================================
// Validation tests
// =============================================================================

static bool test_validate_inventory_slot(void)
{
    CHECK(Validate_InventorySlot(0, 16),         "slot 0 in 16");
    CHECK(Validate_InventorySlot(15, 16),         "slot 15 in 16");
    CHECK(!Validate_InventorySlot(-1, 16),        "slot -1 rejected");
    CHECK(!Validate_InventorySlot(16, 16),        "slot 16 rejected (out of bounds)");
    CHECK(!Validate_InventorySlot(0, 0),          "slot 0 rejected when count=0");
    CHECK(Validate_InventorySlot(0, 1),           "slot 0 in single-element");
    TEST_PASS();
    return true;
}

static bool test_validate_equip_type(void)
{
    CHECK(Validate_EquipType(EQUIP_SURVIVAL_KNIFE), "valid equip type");
    CHECK(Validate_EquipType(EQUIP_COUNT - 1),      "last equip type");
    CHECK(!Validate_EquipType(EQUIP_NONE),           "EQUIP_NONE rejected");
    CHECK(!Validate_EquipType(EQUIP_COUNT),          "out of bounds rejected");
    TEST_PASS();
    return true;
}

static bool test_validate_item_type(void)
{
    CHECK(Validate_ItemType(ITEM_SMALL_HP_POTION), "valid item type");
    CHECK(!Validate_ItemType(ITEM_NONE),            "ITEM_NONE rejected");
    CHECK(!Validate_ItemType(ITEM_COUNT),           "out of bounds rejected");
    TEST_PASS();
    return true;
}

static bool test_validate_monster_type(void)
{
    CHECK(Validate_MonsterType(MONSTER_FLOATING_EYE), "valid monster type");
    CHECK(Validate_MonsterType(0),                    "monster type 0");
    CHECK(Validate_MonsterType(MONSTER_TYPE_COUNT - 1), "last monster type");
    CHECK(!Validate_MonsterType(-1),                  "negative rejected");
    CHECK(!Validate_MonsterType(MONSTER_TYPE_COUNT),  "out of bounds rejected");
    TEST_PASS();
    return true;
}

static bool test_validate_stat_index(void)
{
    CHECK(Validate_StatIndex(0),  "stat 0 (STR)");
    CHECK(Validate_StatIndex(4),  "stat 4 (LCK)");
    CHECK(!Validate_StatIndex(-1), "negative rejected");
    CHECK(!Validate_StatIndex(5),  "out of bounds rejected");
    TEST_PASS();
    return true;
}

static bool test_validate_floor(void)
{
    CHECK(Validate_Floor(1),   "floor 1 valid");
    CHECK(Validate_Floor(10),  "floor 10 valid");
    CHECK(!Validate_Floor(0),  "floor 0 rejected");
    CHECK(!Validate_Floor(-1), "negative rejected");
    TEST_PASS();
    return true;
}

static bool test_clamp_int(void)
{
    CHECK_EQ(Clamp_Int(5, 0, 10),   5,  "clamp in range");
    CHECK_EQ(Clamp_Int(-5, 0, 10),  0,  "clamp below min");
    CHECK_EQ(Clamp_Int(15, 0, 10), 10,  "clamp above max");
    CHECK_EQ(Clamp_Int(0, 0, 0),    0,  "clamp single value");
    TEST_PASS();
    return true;
}

// =============================================================================
// Challenge Rating tests
// =============================================================================

static bool test_floor_1_goblin_cr(void)
{
    Monster_InitTemplates();
    const MonsterTemplate* goblin = Monster_GetTemplate(MONSTER_GOBLIN);
    CHECK(goblin != NULL, "goblin template exists");
    float cr = Monster_CalcCR(goblin, 1);
    CHECK_EQ((int)(cr * 100), 100, "floor 1 goblin CR snaps to 1.0 (1.00)");
    TEST_PASS();
    return true;
}

static bool test_floor_budget_increases(void)
{
    float b1 = Monster_GetFloorBudget(1);
    float b2 = Monster_GetFloorBudget(2);
    float b5 = Monster_GetFloorBudget(5);
    CHECK(b1 > 0, "floor 1 budget positive");
    CHECK(b2 > b1, "floor 2 budget > floor 1");
    CHECK(b5 > b2, "floor 5 budget > floor 2");
    CHECK_EQ((int)(b1 * 10), 30, "floor 1 budget = 3.0");
    CHECK_EQ((int)(b5 * 10), 130, "floor 5 budget = 13.0");
    TEST_PASS();
    return true;
}

static bool test_floor_scale_never_below_one(void)
{
    Monster_InitTemplates();
    const MonsterTemplate* goblin = Monster_GetTemplate(MONSTER_GOBLIN);
    CHECK(goblin != NULL, "goblin template exists");
    // minFloor is 1, so floor 0 should clamp scale to 1.0
    float cr0 = Monster_CalcCR(goblin, 0);
    CHECK(cr0 > 0, "CR positive even below min floor");
    TEST_PASS();
    return true;
}
// =============================================================================

static bool test_spatial_hash_basic(void)
{
    GameWorld gw;
    GameWorld_Init(&gw);

    CHECK_EQ(gw.aliveMonsterCount, 0, "initial monster count is 0");

    // spawn something that isn't a monster — should not touch grid
    EntityId e = World_CreateEntity(&gw.ecs);
    CHECK(e != ENTITY_NONE, "entity creation");
    World_AddComponent(&gw.ecs, e, COMP_POSITION);
    World_AddComponent(&gw.ecs, e, COMP_STATS);
    World_GetStats(&gw.ecs, e)->alive = true;

    // No COMP_AI — not a monster, not in grid
    CHECK_EQ(World_FindMonsterAt(&gw, 5, 5, ENTITY_NONE), ENTITY_NONE, "no monster in grid");

    // Create a real monster (with COMP_AI) and add to grid manually
    EntityId m = World_CreateEntity(&gw.ecs);
    CHECK(m != ENTITY_NONE, "monster entity creation");
    World_AddComponent(&gw.ecs, m, COMP_POSITION);
    World_AddComponent(&gw.ecs, m, COMP_STATS);
    World_GetStats(&gw.ecs, m)->alive = true;
    World_AddComponent(&gw.ecs, m, COMP_AI);

    SpatialHash_Add(&gw, m, 10, 20);
    CHECK_EQ(World_FindMonsterAt(&gw, 10, 20, ENTITY_NONE), m, "find monster at (10,20)");
    CHECK_EQ(World_FindMonsterAt(&gw, 10, 21, ENTITY_NONE), ENTITY_NONE, "wrong tile returns none");

    SpatialHash_Remove(&gw, m, 10, 20);
    CHECK_EQ(World_FindMonsterAt(&gw, 10, 20, ENTITY_NONE), ENTITY_NONE, "removed monster not found");

    TEST_PASS();
    return true;
}

static bool test_spatial_hash_move(void)
{
    GameWorld gw;
    GameWorld_Init(&gw);

    EntityId m = World_CreateEntity(&gw.ecs);
    CHECK(m != ENTITY_NONE, "entity creation");
    World_AddComponent(&gw.ecs, m, COMP_POSITION | COMP_STATS | COMP_AI);
    World_GetStats(&gw.ecs, m)->alive = true;

    CPosition* p = World_GetPosition(&gw.ecs, m);
    p->x = 3; p->y = 4;

    SpatialHash_Add(&gw, m, 3, 4);
    CHECK_EQ(World_FindMonsterAt(&gw, 3, 4, ENTITY_NONE), m, "monster at start");

    // Move to (5,6)
    SpatialHash_Move(&gw, m, 3, 4, 5, 6);
    CHECK_EQ(World_FindMonsterAt(&gw, 3, 4, ENTITY_NONE), ENTITY_NONE, "old tile cleared");
    CHECK_EQ(World_FindMonsterAt(&gw, 5, 6, ENTITY_NONE), m, "monster at new tile");

    // Move again to (0,0)
    SpatialHash_Move(&gw, m, 5, 6, 0, 0);
    CHECK_EQ(World_FindMonsterAt(&gw, 5, 6, ENTITY_NONE), ENTITY_NONE, "intermediate tile cleared");
    CHECK_EQ(World_FindMonsterAt(&gw, 0, 0, ENTITY_NONE), m, "monster at (0,0)");

    TEST_PASS();
    return true;
}

static bool test_spatial_hash_exclude(void)
{
    GameWorld gw;
    GameWorld_Init(&gw);

    EntityId a = World_CreateEntity(&gw.ecs);
    EntityId b = World_CreateEntity(&gw.ecs);
    CHECK(a != ENTITY_NONE && b != ENTITY_NONE, "entity creation");
    World_AddComponent(&gw.ecs, a, COMP_POSITION | COMP_STATS | COMP_AI);
    World_AddComponent(&gw.ecs, b, COMP_POSITION | COMP_STATS | COMP_AI);
    World_GetStats(&gw.ecs, a)->alive = true;
    World_GetStats(&gw.ecs, b)->alive = true;

    SpatialHash_Add(&gw, a, 7, 7);
    SpatialHash_Add(&gw, b, 7, 7);  // overwrites — two monsters can't share tile

    // Without exclude, should find whatever is in the grid
    CHECK_EQ(World_FindMonsterAt(&gw, 7, 7, ENTITY_NONE), b, "b occupies tile after overwrite");

    // Excluding b should still find b in the grid (or none if grid is stale)
    if (World_HasComponents(&gw.ecs, b, COMP_AI)) {
        // grid has b — excluding b returns none
        CHECK_EQ(World_FindMonsterAt(&gw, 7, 7, b), ENTITY_NONE, "exclude b returns none");
    }

    TEST_PASS();
    return true;
}

static bool test_alive_monster_counter(void)
{
    GameWorld gw;
    GameWorld_Init(&gw);
    CHECK_EQ(gw.aliveMonsterCount, 0, "initial count zero");

    // Spawn a monster (simulate what SpawnerSystem_SpawnMonster does)
    EntityId m = World_CreateEntity(&gw.ecs);
    CHECK(m != ENTITY_NONE, "entity creation");
    World_AddComponent(&gw.ecs, m, COMP_POSITION | COMP_STATS | COMP_AI);
    World_GetStats(&gw.ecs, m)->alive = true;
    SpatialHash_Add(&gw, m, 0, 0);
    gw.aliveMonsterCount++;

    CHECK_EQ(gw.aliveMonsterCount, 1, "count after spawn");
    CHECK_EQ(World_CountAliveMonsters(&gw), 1, "count function matches");

    // Kill it (simulate combat death)
    World_GetStats(&gw.ecs, m)->alive = false;
    SpatialHash_Remove(&gw, m, 0, 0);
    gw.aliveMonsterCount--;

    CHECK_EQ(gw.aliveMonsterCount, 0, "count after death");
    CHECK(World_AreAllMonstersDead(&gw), "all monsters dead");

    // Reinit clears counter
    GameWorld_Init(&gw);
    CHECK_EQ(gw.aliveMonsterCount, 0, "count after reinit");

    TEST_PASS();
    return true;
}

static bool test_game_world_init_clears_grid(void)
{
    GameWorld gw;
    GameWorld_Init(&gw);

    // After init, grid should be cleared (all ENTITY_NONE)
    CHECK_EQ(World_FindMonsterAt(&gw, 50, 50, ENTITY_NONE), ENTITY_NONE, "init clears grid");

    // Add something, reinit, verify cleared
    EntityId m = World_CreateEntity(&gw.ecs);
    World_AddComponent(&gw.ecs, m, COMP_POSITION | COMP_STATS | COMP_AI);
    World_GetStats(&gw.ecs, m)->alive = true;
    SpatialHash_Add(&gw, m, 30, 40);
    CHECK_EQ(World_FindMonsterAt(&gw, 30, 40, ENTITY_NONE), m, "monster present");

    GameWorld_Init(&gw);
    CHECK_EQ(World_FindMonsterAt(&gw, 30, 40, ENTITY_NONE), ENTITY_NONE, "reinit clears grid");

    TEST_PASS();
    return true;
}

// =============================================================================
// Test runner
// =============================================================================

typedef bool (*TestFunc)(void);

static struct {
    const char* name;
    TestFunc fn;
} g_tests[] = {
    // Stat calculations
    {"xp_curve",               test_xp_curve},
    {"max_hp",                 test_max_hp},
    {"melee_damage",           test_melee_damage},
    {"ranged_damage",          test_ranged_damage},
    {"throw_damage",           test_throw_damage},
    {"magic_damage_and_resist", test_magic_damage_and_resist},
    {"damage_after_defense",   test_damage_after_defense},
    {"dodge_chance",           test_dodge_chance},
    {"throw_range",            test_throw_range},
    {"wait_heal",              test_wait_heal},
    {"potion_heal",            test_potion_heal},

    // Inventory / equipment data
    {"equip_data_table",       test_equip_data_table},
    {"item_data_table",        test_item_data_table},

    // Equipment bonus logic
    {"equip_bonus_apply_remove", test_equipment_bonus_apply_remove},
    {"equip_bonus_recalculate",  test_equipment_bonus_recalculate},
    {"equip_bonus_edge_cases",   test_equipment_bonus_edge_cases},

    // Validation
    {"validate_inventory_slot",  test_validate_inventory_slot},
    {"validate_equip_type",      test_validate_equip_type},
    {"validate_item_type",       test_validate_item_type},
    {"validate_monster_type",    test_validate_monster_type},
    {"validate_stat_index",      test_validate_stat_index},
    {"validate_floor",           test_validate_floor},
    {"clamp_int",                test_clamp_int},

    // Challenge Rating
    {"floor_1_goblin_cr",        test_floor_1_goblin_cr},
    {"floor_budget_increases",   test_floor_budget_increases},
    {"floor_scale_never_below_one", test_floor_scale_never_below_one},

    // Spatial hash / performance
    {"spatial_hash_basic",       test_spatial_hash_basic},
    {"spatial_hash_move",        test_spatial_hash_move},
    {"spatial_hash_exclude",     test_spatial_hash_exclude},
    {"alive_monster_counter",    test_alive_monster_counter},
    {"game_world_init_clears_grid", test_game_world_init_clears_grid},
};

int main(void)
{
    int passed = 0;
    int failed = 0;
    int total = (int)(sizeof(g_tests) / sizeof(g_tests[0]));

    printf("=== Heroes of Taboo Unit Tests ===\n");
    printf("Total: %d tests\n\n", total);

    for (int i = 0; i < total; i++) {
        printf("[%2d/%2d] %s\n", i + 1, total, g_tests[i].name);
        if (g_tests[i].fn()) {
            passed++;
        } else {
            failed++;
        }
    }

    printf("\n=== Results: %d passed, %d failed, %d total ===\n",
           passed, failed, total);

    return (failed > 0) ? 1 : 0;
}
