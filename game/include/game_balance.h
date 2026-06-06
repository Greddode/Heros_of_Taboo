#ifndef GAME_BALANCE_H
#define GAME_BALANCE_H

#include "config.h"

// =============================================================================
// game_balance.h — Centralized game balance constants and formulas
//
// All stat calculations, damage formulas, XP curves, and magic numbers live
// here.  Every macro and inline function documents its contract and units.
// Values are read from balance.json at runtime via Config_GetXxx(), falling
// back to the #define default when the file or key is missing.
// =============================================================================

#define PROCEDURAL_MAP_WIDTH    80
#define PROCEDURAL_MAP_HEIGHT   50

#define DEFAULT_MAX_FLOORS      10
#define DEFAULT_START_FLOOR      1

#define PLAYER_BASE_STR          3
#define PLAYER_BASE_DEX          3
#define PLAYER_BASE_INT          3
#define PLAYER_BASE_CON          3
#define PLAYER_BASE_LCK          2
#define PLAYER_BASE_ATTACK       5
#define PLAYER_BASE_DEFENSE      1
#define PLAYER_BASE_LEVEL        1
#define PLAYER_SPAWN_X           1
#define PLAYER_SPAWN_Y           1

#define STAT_CAP_STR            99
#define STAT_CAP_DEX            99
#define STAT_CAP_INT            99
#define STAT_CAP_CON            99
#define STAT_CAP_LCK            99
#define STAT_CAP_DEFAULT       20
#define STAT_CAP_UNLIMITED    999

#define STAT_POINTS_PER_LEVEL    2
#define LEVEL_UP_FLASH_SECONDS   3.0f

#define HP_BASE_CONSTANT        30
#define HP_PER_CON               5

#define XP_BASE                 50
#define XP_PER_LEVEL            40

#define STR_MELEE_MULT           2
#define DEX_RANGED_MULT          1.5f
#define DEX_THROW_MULT           2
#define MAGIC_RESIST_MULT        3
#define CRIT_MULT                2
#define MEGA_CRIT_THRESHOLD    100
#define MEGA_CRIT_CHANCE        50
#define DUAL_WIELD_OFFHAND_MULT 0.5f
#define MIN_DAMAGE               1

#define DODGE_PER_DEX            2
#define DODGE_CAP_PCT           60

#define THROW_RANGE_BASE         3
#define THROW_RANGE_PER_DEX_DIV  3

#define WAIT_HEAL_BASE           1
#define WAIT_HEAL_INTEL_DIV      2
#define WAIT_COOLDOWN            0.08f

#define SHADOW_SPAWN_WAITS      15
#define SHADOW_DOUBLE_MOVE_MIN  25
#define SHADOW_DOUBLE_MOVE_FAST 35

#define POTION_INT_SCALE         0.02f
#define POTION_HEAL_DENOM    10000

#define HIT_FLASH_DURATION       0.15f
#define SPRINT_ANIM_DURATION     0.30f
#define DEFAULT_MELEE_COOLDOWN   0.15f

#define AI_HUNT_TURNS            4
#define AI_WANDER_RANGE_EXTRA    8
#define AI_NEIGHBOR_COUNT        4

#define DEFAULT_CAMERA_ZOOM      4.0f

#define UI_BASE_WIDTH         1024.0f
#define UI_BASE_HEIGHT         768.0f
#define UI_MIN_AUTO_SCALE        0.75f
#define UI_GUI_SCALE_MIN         1.0f
#define UI_GUI_SCALE_MAX         4.0f

#define DEFAULT_SCROLL_STEP      20

#define GOLD_RARITY_BONUS_COMMON      0
#define GOLD_RARITY_BONUS_UNCOMMON   50
#define GOLD_RARITY_BONUS_RARE      150
#define GOLD_RARITY_BONUS_LEGENDARY 500
#define GOLD_SELL_RATIO             0.5f

#define POTION_SELL_SMALL           8
#define POTION_SELL_MEDIUM         20
#define POTION_SELL_LARGE          40

#define MP_BASE            10
#define MP_PER_INT          2
#define MP_REGEN_PER_TURN   1
#define MP_BONUS_ON_KILL    3

#define CON_HP_SCALE         5
#define STR_ATTACK_SCALE      2
#define FLOOR_DR_BASE        8.0f
#define FLOOR_DR_PER_FLOOR   4.0f

static inline int calc_xp_for_level(int level)
{
    return Config_GetInt("xp", "base", XP_BASE) + level * Config_GetInt("xp", "per_level", XP_PER_LEVEL);
}

static inline int calc_max_hp(int con)
{
    return Config_GetInt("hp", "base_constant", HP_BASE_CONSTANT) + con * Config_GetInt("hp", "per_con", HP_PER_CON);
}

static inline int calc_melee_damage(int attack, int str)
{
    return attack + str * Config_GetInt("combat", "str_melee_mult", STR_MELEE_MULT);
}

static inline int calc_ranged_damage(int dex)
{
    return (int)((float)dex * Config_GetFloat("combat", "dex_ranged_mult", DEX_RANGED_MULT));
}

static inline int calc_throw_damage(int attack, int dex)
{
    return attack + dex * Config_GetInt("combat", "dex_throw_mult", DEX_THROW_MULT);
}

static inline int calc_magic_damage(int attack, int intel)
{
    return attack + intel;
}

static inline int calc_magic_resist(int intel)
{
    return intel * Config_GetInt("combat", "magic_resist_mult", MAGIC_RESIST_MULT);
}

static inline int calc_damage_after_defense(int damage, int defense)
{
    int result = damage - defense;
    return (result < Config_GetInt("combat", "min_damage", MIN_DAMAGE)) ? Config_GetInt("combat", "min_damage", MIN_DAMAGE) : result;
}

static inline int calc_dodge_chance(int dex)
{
    int pct = dex * Config_GetInt("combat", "dodge_per_dex", DODGE_PER_DEX);
    int cap = Config_GetInt("combat", "dodge_cap_pct", DODGE_CAP_PCT);
    return (pct > cap) ? cap : pct;
}

static inline int calc_throw_range(int dex)
{
    int base = Config_GetInt("combat", "throw_range_base", THROW_RANGE_BASE);
    int range = base + dex / Config_GetInt("combat", "throw_range_per_dex_div", THROW_RANGE_PER_DEX_DIV);
    return (range < base) ? base : range;
}

static inline int calc_wait_heal(int intel)
{
    return Config_GetInt("wait", "heal_base", WAIT_HEAL_BASE) + intel / Config_GetInt("wait", "heal_intel_div", WAIT_HEAL_INTEL_DIV);
}

static inline int calc_potion_heal(int maxHp, int healPercent, int intel)
{
    float intMult = 1.0f + (float)intel * POTION_INT_SCALE;
    int heal = (maxHp * healPercent * (int)(intMult * 100)) / POTION_HEAL_DENOM;
    return (heal < 1) ? 1 : heal;
}

#endif
