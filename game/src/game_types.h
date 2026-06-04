#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include "raylib.h"

#define MAP_WIDTH 100
#define MAP_HEIGHT 100
#define PROJECTILE_ANIM_DURATION 0.25f
#define MOVE_ANIM_DURATION 0.15f

// -- Attack type identifiers -------------------------------------------------
typedef enum {
    ATTACK_MELEE = 0,
    ATTACK_RANGED,
    ATTACK_MAGIC,
    ATTACK_THROW
} AttackType;

// Direction an entity can move
typedef enum {
    DIR_NONE = 0,
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} Direction;

// -- Monster type identifiers ------------------------------------------------
typedef enum {
    MONSTER_FLOATING_EYE,
    MONSTER_FUNGAL_MYCONID,
    MONSTER_OGRE,
    MONSTER_SHADOW,
    MONSTER_BAT,
    MONSTER_DEMON_EYE,
    MONSTER_DRAGON,
    MONSTER_GOBLIN,
    MONSTER_SKELETON,
    MONSTER_WARP_SKULL,
    MONSTER_RANGER_GOBLIN,
    MONSTER_TYPE_COUNT
} MonsterType;

// -- Item types --------------------------------------------------------------
typedef enum {
    ITEM_NONE = 0,
    ITEM_SMALL_HP_POTION,
    ITEM_MEDIUM_HP_POTION,
    ITEM_LARGE_HP_POTION,
    ITEM_COUNT
} ItemType;

// -- Equipment types ---------------------------------------------------------
typedef enum {
    EQUIP_SLOT_HEAD = 0,
    EQUIP_SLOT_CHEST,
    EQUIP_SLOT_WEAPON,
    EQUIP_SLOT_OFF_HAND,
    EQUIP_SLOT_ACCESSORY
} EquipSlot;

typedef enum {
    EQUIP_CAT_ARMOR = 0,
    EQUIP_CAT_WEAPON,
    EQUIP_CAT_ACCESSORY,
    EQUIP_CAT_COUNT
} EquipCategory;

typedef enum {
    EQUIP_NONE = 0,
    // Armor (Head)
    EQUIP_LEATHER_CAP,
    EQUIP_IRON_HELM,
    EQUIP_STEEL_HELM,
    // Armor (Chest)
    EQUIP_LEATHER_VEST,
    EQUIP_CHAIN_MAIL,
    EQUIP_PLATE_MAIL,
    // Weapons
    EQUIP_SURVIVAL_KNIFE,
    EQUIP_DAGGER,
    EQUIP_IRON_SWORD,
    EQUIP_STEEL_SWORD,
    EQUIP_WAR_HAMMER,
    // Off-hand
    EQUIP_WOODEN_SHIELD,
    EQUIP_IRON_SHIELD,
    EQUIP_STEEL_SHIELD,
    // Accessories
    EQUIP_RING_OF_STRENGTH,
    EQUIP_AMULET_OF_WARDING,
    EQUIP_BOOTS_OF_SWIFTNESS,
    EQUIP_RING_OF_THE_HAWK,
    EQUIP_SAGES_PENDANT,
    EQUIP_LUCKY_CHARM,
    EQUIP_BERSERKER_BAND,
    // Ranged weapons (two-handed, EQUIP_SLOT_WEAPON)
    EQUIP_SIMPLE_BOW,
    EQUIP_DWARVEN_BOW,
    EQUIP_ELVEN_BOW,
    EQUIP_GREATBOW,
    EQUIP_CROSSBOW,
    EQUIP_BAND_OF_GROWTH,
    EQUIP_COUNT
} EquipType;

// -- Game state --------------------------------------------------------------
typedef enum {
    STATE_PLAYER_TURN,
    STATE_ENEMY_TURN,
    STATE_GAME_OVER,
    STATE_WIN,
    STATE_INVENTORY,
    STATE_MAP
} GameState;

// -- Projectile --------------------------------------------------------------
typedef struct {
    bool active;
    float sx, sy;
    float ex, ey;
    Color color;
    int tileSX, tileSY;
    int tileEX, tileEY;
    AttackType attackType;
    int startFrame;
    int startRow;
    int animFrameCount;
    Texture2D* throwTex;
    float      throwRotation;
    int        projectileVisual;
} Projectile;

#endif
