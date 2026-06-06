#ifndef EVENT_BUS_H
#define EVENT_BUS_H

#include "game_types.h"
#include "ecs.h"

typedef enum {
    EVT_MONSTER_KILLED,
    EVT_PLAYER_LEVEL_UP,
    EVT_ITEM_PICKED_UP,
    EVT_ITEM_EQUIPPED,
    EVT_ITEM_UNEQUIPPED,
    EVT_FLOOR_DESCENDED,
    EVT_PLAYER_DAMAGED,
    EVT_MONSTER_SPAWNED,
    EVT_GOLD_GAINED,
    EVT_COUNT
} EventType;

typedef struct {
    EntityId monsterId;
    MonsterType monsterType;
    int x, y;
    int xpValue;
    int goldValue;
    EquipType droppedEquip;
} MonsterKilledEvent;

typedef struct {
    int oldLevel;
    int newLevel;
    int statPointsGranted;
} PlayerLevelUpEvent;

typedef struct {
    int x, y;
    bool isEquip;
    int typeId;
    int quantity;
} ItemPickedUpEvent;

typedef struct {
    EquipType equipType;
} ItemEquippedEvent;

typedef struct {
    EquipType equipType;
} ItemUnequippedEvent;

typedef struct {
    int oldFloor;
    int newFloor;
} FloorDescendedEvent;

typedef struct {
    int damage;
    int hpRemaining;
} PlayerDamagedEvent;

typedef struct {
    EntityId monsterId;
    MonsterType monsterType;
    int x, y;
} MonsterSpawnedEvent;

typedef struct {
    int amount;
    int totalGold;
} GoldGainedEvent;

typedef void (*EventCallback)(void* data);

#define EVT_MAX_SUBSCRIBERS 8

void EventBus_Init(void);
void EventBus_Subscribe(EventType type, EventCallback callback);
void EventBus_Publish(EventType type, void* data);
void EventBus_Clear(void);

#endif
