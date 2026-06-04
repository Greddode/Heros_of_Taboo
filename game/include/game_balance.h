#ifndef GAME_BALANCE_H
#define GAME_BALANCE_H

// =============================================================================
// game_balance.h — Centralized game balance constants and formulas
//
// All stat calculations, damage formulas, XP curves, and magic numbers live
// here.  Every macro and inline function documents its contract and units.
// =============================================================================

// --- Map limits (procedural generation) --------------------------------------
// MAP_WIDTH / MAP_HEIGHT  are defined in game_types.h (100 each)
// MAX_INVENTORY_SLOTS, EQUIP_SLOT_COUNT, MAX_POTIONS, MAX_EQUIP_ON_MAP,
// MAX_EQUIPMENT_TYPES are defined in inventory.h
#define PROCEDURAL_MAP_WIDTH    80   // Procedural dungeon tile width
#define PROCEDURAL_MAP_HEIGHT   50   // Procedural dungeon tile height

// --- Floor / dungeon defaults ------------------------------------------------
#define DEFAULT_MAX_FLOORS      10   // Number of floors before escape appears
#define DEFAULT_START_FLOOR      1   // Starting floor index

// --- Player starting stats (before equipment) --------------------------------
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

// --- Stat caps (enforceable by validation later) -----------------------------
#define STAT_CAP_STR            99
#define STAT_CAP_DEX            99
#define STAT_CAP_INT            99
#define STAT_CAP_CON            99
#define STAT_CAP_LCK            99
#define STAT_CAP_DEFAULT       20
#define STAT_CAP_UNLIMITED    999

// --- Level-up tuning ---------------------------------------------------------
#define STAT_POINTS_PER_LEVEL    2   // Stat points awarded on level-up
#define LEVEL_UP_FLASH_SECONDS   3.0f // Duration of on-screen level-up indicator

// --- Derived max HP ----------------------------------------------------------
//   maxHp = HP_BASE_CONSTANT + con * HP_PER_CON
#define HP_BASE_CONSTANT        30
#define HP_PER_CON               5

// --- Experience curve --------------------------------------------------------
//   xpRequired = XP_BASE + level * XP_PER_LEVEL
#define XP_BASE                 50
#define XP_PER_LEVEL            40

// --- Damage formulas ---------------------------------------------------------
//   Melee:   attack + str * STR_MELEE_MULT - defense
#define STR_MELEE_MULT           2

//   Ranged:  dex * DEX_RANGED_MULT - defense
#define DEX_RANGED_MULT          1.5f

//   Throw:   (attack + dex * DEX_THROW_MULT) - defense
#define DEX_THROW_MULT           2

//   Monster magic raw: attack + intel
//   Monster magic resist: playerInt * MAGIC_RESIST_MULT
#define MAGIC_RESIST_MULT        3

//   Critical hit:  damage * CRIT_MULT when LCK% roll succeeds
#define CRIT_MULT                2
#define MEGA_CRIT_THRESHOLD    100   // Minimum crit damage to trigger mega-crit check
#define MEGA_CRIT_CHANCE        50   // Percentage chance to double a crit that exceeds threshold
#define DUAL_WIELD_OFFHAND_MULT 0.5f // Off-hand follow-up damage multiplier
#define MIN_DAMAGE               1   // Minimum damage after all reductions

// --- Dodge / evasion ---------------------------------------------------------
//   dodgePct = dex * DODGE_PER_DEX, capped at DODGE_CAP_PCT
#define DODGE_PER_DEX            2
#define DODGE_CAP_PCT           60   // Maximum dodge percentage

// --- Throw weapon ------------------------------------------------------------
#define THROW_RANGE_BASE         3
#define THROW_RANGE_PER_DEX_DIV  3   // extra range = dex / 3

// --- Wait / rest -------------------------------------------------------------
//   waitHeal = WAIT_HEAL_BASE + intel / WAIT_HEAL_INTEL_DIV
#define WAIT_HEAL_BASE           1
#define WAIT_HEAL_INTEL_DIV      2

//   waitCooldown (seconds before enemy turn after wait)
#define WAIT_COOLDOWN            0.08f

//   Shadow spawn after this many wait turns
#define SHADOW_SPAWN_WAITS      15
//   Shadow double-move thresholds (timeWaited)
#define SHADOW_DOUBLE_MOVE_MIN  25
#define SHADOW_DOUBLE_MOVE_FAST 35

// --- Potion healing ----------------------------------------------------------
//   intMult  = 1.0f + intel * POTION_INT_SCALE
//   healPct  = ITEM_HEALS[type]  (25, 50, 75)
//   healAmount = (maxHp * healPct * (int)(intMult * 100)) / POTION_HEAL_DENOM
#define POTION_INT_SCALE         0.02f
#define POTION_HEAL_DENOM    10000

// --- Animation timers (seconds) ----------------------------------------------
// MOVE_ANIM_DURATION (0.15f) and PROJECTILE_ANIM_DURATION (0.25f)
// are defined in game_types.h
#define HIT_FLASH_DURATION       0.15f
#define SPRINT_ANIM_DURATION     0.30f
#define DEFAULT_MELEE_COOLDOWN   0.15f

// --- AI / monster ------------------------------------------------------------
#define AI_HUNT_TURNS            4
#define AI_WANDER_RANGE_EXTRA    8   // wander if within detection + this range
#define AI_NEIGHBOR_COUNT        4

// --- Camera ----------------------------------------------------------------
#define DEFAULT_CAMERA_ZOOM      4.0f

// --- UI scale ----------------------------------------------------------------
#define UI_BASE_WIDTH         1024.0f
#define UI_BASE_HEIGHT         768.0f
#define UI_MIN_AUTO_SCALE        0.75f
#define UI_GUI_SCALE_MIN         1.0f
#define UI_GUI_SCALE_MAX         4.0f

// --- UI scroll step (pixels before scale) -----------------------------------
#define DEFAULT_SCROLL_STEP      20

// --- Gold / pricing ----------------------------------------------------------
#define GOLD_RARITY_BONUS_COMMON      0
#define GOLD_RARITY_BONUS_UNCOMMON   50
#define GOLD_RARITY_BONUS_RARE      150
#define GOLD_RARITY_BONUS_LEGENDARY 500
#define GOLD_SELL_RATIO             0.5f

// =============================================================================
// Inline formula functions
// =============================================================================

// --- XP curve ----------------------------------------------------------------
// Returns total XP required to reach a given level (starting from level 1).
// level must be >= 1.
static inline int calc_xp_for_level(int level)
{
    return XP_BASE + level * XP_PER_LEVEL;
}

// --- Derived max HP ----------------------------------------------------------
// Returns the maximum HP derived from CON (before equipment HP bonuses).
// con is the raw CON stat after equipment modifiers.
static inline int calc_max_hp(int con)
{
    return HP_BASE_CONSTANT + con * HP_PER_CON;
}

// --- Melee damage ------------------------------------------------------------
// Raw damage before defense subtraction.  Minimum enforced by callers.
// attack: entity's attack stat, str: entity's strength stat.
static inline int calc_melee_damage(int attack, int str)
{
    return attack + str * STR_MELEE_MULT;
}

// --- Ranged damage -----------------------------------------------------------
// Raw damage before defense subtraction.  Minimum enforced by callers.
// dex: entity's dexterity stat.
static inline int calc_ranged_damage(int dex)
{
    return (int)((float)dex * DEX_RANGED_MULT);
}

// --- Throw damage ------------------------------------------------------------
// Raw damage before defense subtraction.  Minimum enforced by callers.
// attack: entity's attack stat, dex: entity's dexterity stat.
static inline int calc_throw_damage(int attack, int dex)
{
    return attack + dex * DEX_THROW_MULT;
}

// --- Monster magic damage ----------------------------------------------------
// Raw magic damage before magic resistance subtraction.
// attack: monster's attack stat, intel: monster's intelligence stat.
static inline int calc_magic_damage(int attack, int intel)
{
    return attack + intel;
}

// --- Magic resistance --------------------------------------------------------
// Damage reduction from magic attacks.
// intel: defending entity's intelligence stat.
static inline int calc_magic_resist(int intel)
{
    return intel * MAGIC_RESIST_MULT;
}

// --- Damage after defense ----------------------------------------------------
// Applies defense reduction and enforces minimum damage.
// damage: raw damage value, defense: target's defense stat.
static inline int calc_damage_after_defense(int damage, int defense)
{
    int result = damage - defense;
    return (result < MIN_DAMAGE) ? MIN_DAMAGE : result;
}

// --- Dodge chance ------------------------------------------------------------
// Returns dodge percentage (0–100) based on dexterity.
// dex: entity's dexterity stat.
static inline int calc_dodge_chance(int dex)
{
    int pct = dex * DODGE_PER_DEX;
    return (pct > DODGE_CAP_PCT) ? DODGE_CAP_PCT : pct;
}

// --- Throw weapon range ------------------------------------------------------
// Returns the throw range in tiles based on dexterity.
// dex: entity's dexterity stat.
static inline int calc_throw_range(int dex)
{
    int range = THROW_RANGE_BASE + dex / THROW_RANGE_PER_DEX_DIV;
    return (range < THROW_RANGE_BASE) ? THROW_RANGE_BASE : range;
}

// --- Wait heal ---------------------------------------------------------------
// Returns HP healed when waiting one turn.
// intel: entity's intelligence stat.
static inline int calc_wait_heal(int intel)
{
    return WAIT_HEAL_BASE + intel / WAIT_HEAL_INTEL_DIV;
}

// --- Potion heal -------------------------------------------------------------
// Returns the actual HP healed by a potion.
// maxHp: entity's max HP, healPercent: potion's heal percentage (25, 50, 75),
// intel: entity's intelligence stat.
static inline int calc_potion_heal(int maxHp, int healPercent, int intel)
{
    float intMult = 1.0f + (float)intel * POTION_INT_SCALE;
    int heal = (maxHp * healPercent * (int)(intMult * 100)) / POTION_HEAL_DENOM;
    return (heal < 1) ? 1 : heal;
}

#endif
