# API Reference

**Heroes of Taboo** — v0.0.10 | June 5, 2026

Complete reference for all public types, functions, and constants.

---

## Table of Contents

1. [Engine Layer](#engine-layer)
2. [Shared Types](#shared-types)
3. [ECS Components](#ecs-components)
4. [Game Balance](#game-balance)
5. [World & Game State](#world--game-state)
6. [Game Bridge](#game-bridge)
7. [Equipment Bonus](#equipment-bonus)
8. [Floor Initialization](#floor-initialization)
9. [Validation](#validation)
10. [Inventory & Equipment](#inventory--equipment)
11. [Systems](#systems)
12. [Data Modules](#data-modules)
13. [Map Modules](#map-modules)
14. [UI Modules](#ui-modules)
15. [Audio](#audio)

---

## Engine Layer

### ECS Core (`engine/ecs.h`)

#### Types

```c
typedef uint32_t EntityId;
typedef uint32_t ComponentMask;

#define ENTITY_NONE    0        // Reserved sentinel — never allocate
#define MAX_ENTITIES   128      // Fixed entity pool size
```

#### struct World

The ECS world containing all entity data.

```c
typedef struct World {
    ComponentMask masks[MAX_ENTITIES];    // Bitmask per entity
    bool          alive[MAX_ENTITIES];    // Alive flag per entity
    int           count;                  // Next entity ID to allocate
    int           freeList[MAX_ENTITIES]; // Recycled entity IDs
    int           freeCount;              // Number of recycled IDs

    // Parallel component arrays (structure-of-arrays)
    CPosition      positions[MAX_ENTITIES];
    CStats         stats[MAX_ENTITIES];
    CSpriteAnim    sprites[MAX_ENTITIES];
    CFallbackColor colors[MAX_ENTITIES];
    CAI            ais[MAX_ENTITIES];
    CPickup        pickups[MAX_ENTITIES];
    CName          names[MAX_ENTITIES];
    CHitFlash      hitFlashes[MAX_ENTITIES];
    CAbilities     abilities[MAX_ENTITIES];
} World;
```

#### Functions

| Function | Signature | Description |
|----------|-----------|-------------|
| `World_Init` | `void World_Init(World* w)` | Initialize world. Sets count=1 (slot 0 reserved), clears all masks and alive flags. |
| `World_CreateEntity` | `EntityId World_CreateEntity(World* w)` | Allocate a new entity. Pops from free list or increments count. Returns `ENTITY_NONE` if pool exhausted. Zeros all components. |
| `World_DestroyEntity` | `void World_DestroyEntity(World* w, EntityId e)` | Destroy entity. Zeros components, clears mask, pushes ID to free list. No-op if `e == ENTITY_NONE` or out of bounds. |
| `World_IsAlive` | `bool World_IsAlive(const World* w, EntityId e)` | Returns true if entity is alive and in bounds. |
| `World_HasComponents` | `bool World_HasComponents(const World* w, EntityId e, ComponentMask mask)` | Returns true if entity is alive AND has ALL bits in mask set. |
| `World_AddComponent` | `static inline void World_AddComponent(World* w, EntityId e, ComponentMask mask)` | Sets bits in entity's mask. `masks[e] \|= mask`. |
| `World_RemoveComponent` | `static inline void World_RemoveComponent(World* w, EntityId e, ComponentMask mask)` | Clears bits in entity's mask. `masks[e] &= ~mask`. |

#### Component Accessors

All return a pointer to the component data for entity `e`. In `DEBUG` builds, each asserts: `e != ENTITY_NONE`, `e < w->count`, `w->alive[e]`, and the entity has the required component bit set.

| Function | Returns | Required Bit |
|----------|---------|-------------|
| `World_GetPosition(w, e)` | `CPosition*` | `COMP_POSITION` |
| `World_GetStats(w, e)` | `CStats*` | `COMP_STATS` |
| `World_GetSprite(w, e)` | `CSpriteAnim*` | `COMP_SPRITE_ANIM` |
| `World_GetColor(w, e)` | `CFallbackColor*` | `COMP_FALLBACK_COLOR` |
| `World_GetAI(w, e)` | `CAI*` | `COMP_AI` |
| `World_GetPickup(w, e)` | `CPickup*` | `COMP_PICKUP` |
| `World_GetName(w, e)` | `CName*` | `COMP_NAME` |
| `World_GetHitFlash(w, e)` | `CHitFlash*` | `COMP_HIT_FLASH` |
| `World_GetAbilities(w, e)` | `CAbilities*` | `COMP_ABILITIES` |

---

### Resource Manager (`engine/resources.h`)

| Function | Signature | Description |
|----------|-----------|-------------|
| `Resources_LoadTexture` | `Texture2D* Resources_LoadTexture(const char* path)` | Load texture by path. Returns cached pointer if already loaded, otherwise loads via Raylib `LoadTexture()` and caches. Returns pointer to null texture if cache full (64 max) or load fails. **Never call `LoadTexture()` directly.** |
| `Resources_UnloadAll` | `void Resources_UnloadAll(void)` | Unload all cached textures and reset cache. Called during `CleanupGame()`. |

---

### Audio System (`engine/audio.h`)

#### Types

```c
typedef int AudioMusicContext;
typedef int AudioSoundCategory;
#define AUDIO_INVALID (-1)
```

#### Functions

| Function | Signature | Description |
|----------|-----------|-------------|
| `InitAudioSystem` | `void InitAudioSystem(void)` | Initialize Raylib audio device. |
| `UpdateMusicSystem` | `void UpdateMusicSystem(void)` | Call every frame to advance music playback. |
| `ShutdownAudioSystem` | `void ShutdownAudioSystem(void)` | Close audio device and free resources. |
| `Audio_RegisterMusicContext` | `AudioMusicContext Audio_RegisterMusicContext(const char* name, const char* dirPath)` | Register a music context from a directory of tracks. Returns context ID. |
| `Audio_RegisterSoundCategory` | `AudioSoundCategory Audio_RegisterSoundCategory(const char* name, const char* dirPath)` | Register a sound category from a directory of SFX files. Returns category ID. |
| `Audio_SetMusicContext` | `void Audio_SetMusicContext(AudioMusicContext ctx)` | Switch active music context (crossfade). |
| `Audio_GetMusicContext` | `AudioMusicContext Audio_GetMusicContext(void)` | Get current active music context. |
| `Audio_PlaySound` | `void Audio_PlaySound(AudioSoundCategory cat)` | Play a random sound from the category. |
| `Audio_GetMusicVolume` | `float Audio_GetMusicVolume(void)` | Get music volume (0.0–1.0). |
| `Audio_SetMusicVolume` | `void Audio_SetMusicVolume(float vol)` | Set music volume. |
| `Audio_GetSFXVolume` | `float Audio_GetSFXVolume(void)` | Get SFX volume (0.0–1.0). |
| `Audio_SetSFXVolume` | `void Audio_SetSFXVolume(float vol)` | Set SFX volume. |

---

## Shared Types

### Game Types (`game/src/game_types.h`)

#### Constants

```c
#define MAP_WIDTH                  100
#define MAP_HEIGHT                 100
#define PROJECTILE_ANIM_DURATION   0.25f
#define MOVE_ANIM_DURATION         0.15f
```

#### Enums

**AttackType** — How an entity attacks:
```c
typedef enum { ATTACK_MELEE = 0, ATTACK_RANGED, ATTACK_MAGIC, ATTACK_THROW } AttackType;
```

**Direction** — Cardinal movement direction:
```c
typedef enum { DIR_NONE = 0, DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT } Direction;
```

**MonsterType** — 11 monster types:
```c
typedef enum {
    MONSTER_FLOATING_EYE, MONSTER_FUNGAL_MYCONID, MONSTER_OGRE,
    MONSTER_SHADOW, MONSTER_BAT, MONSTER_DEMON_EYE, MONSTER_DRAGON,
    MONSTER_GOBLIN, MONSTER_SKELETON, MONSTER_WARP_SKULL,
    MONSTER_RANGER_GOBLIN, MONSTER_TYPE_COUNT
} MonsterType;
```

**ItemType** — Potion types:
```c
typedef enum { ITEM_NONE = 0, ITEM_SMALL_HP_POTION, ITEM_MEDIUM_HP_POTION, ITEM_LARGE_HP_POTION, ITEM_COUNT } ItemType;
```

**EquipSlot** — 5 equipment slots:
```c
typedef enum { EQUIP_SLOT_HEAD = 0, EQUIP_SLOT_CHEST, EQUIP_SLOT_WEAPON, EQUIP_SLOT_OFF_HAND, EQUIP_SLOT_ACCESSORY } EquipSlot;
```

**EquipCategory** — Equipment categories:
```c
typedef enum { EQUIP_CAT_ARMOR = 0, EQUIP_CAT_WEAPON, EQUIP_CAT_ACCESSORY, EQUIP_CAT_COUNT } EquipCategory;
```

**EquipType** — 30 equipment types (see `inventory.c` for full list):
```c
typedef enum {
    EQUIP_NONE = 0,
    // Head: EQUIP_LEATHER_CAP, EQUIP_IRON_HELM, EQUIP_STEEL_HELM
    // Chest: EQUIP_LEATHER_VEST, EQUIP_CHAIN_MAIL, EQUIP_PLATE_MAIL
    // Weapons: EQUIP_SURVIVAL_KNIFE through EQUIP_WAR_HAMMER
    // Off-hand: EQUIP_WOODEN_SHIELD, EQUIP_IRON_SHIELD, EQUIP_STEEL_SHIELD
    // Accessories: EQUIP_RING_OF_STRENGTH through EQUIP_BERSERKER_BAND
    // Ranged: EQUIP_SIMPLE_BOW through EQUIP_CROSSBOW
    // Legendary: EQUIP_BAND_OF_GROWTH
    EQUIP_COUNT
} EquipType;
```

**GameState** — Turn-based state machine:
```c
typedef enum {
    STATE_PLAYER_TURN, STATE_ENEMY_TURN, STATE_GAME_OVER,
    STATE_WIN, STATE_INVENTORY, STATE_MAP, STATE_SHOP
} GameState;
```

#### Structs

**Projectile** — Active projectile state:
```c
typedef struct {
    bool active;
    float sx, sy;           // Start position (pixel)
    float ex, ey;           // End position (pixel)
    Color color;
    int tileSX, tileSY;     // Start tile (for magic wave)
    int tileEX, tileEY;     // End tile
    AttackType attackType;
    int startFrame, startRow, animFrameCount;  // Magic animation
    Texture2D* throwTex;    // Thrown weapon texture
    float throwRotation;
    int projectileVisual;   // ProjectileVisual enum
} Projectile;
```

---

## ECS Components

### Component Bitmask Defines (`game/src/components.h`)

```c
#define COMP_POSITION       (1u << 0)
#define COMP_STATS          (1u << 1)
#define COMP_SPRITE_ANIM    (1u << 2)
#define COMP_FALLBACK_COLOR (1u << 3)
#define COMP_AI             (1u << 4)
#define COMP_PICKUP         (1u << 5)
#define COMP_NAME           (1u << 6)
#define COMP_PLAYER_TAG     (1u << 7)
#define COMP_HIT_FLASH      (1u << 8)
#define COMP_ABILITIES      (1u << 9)
```

### Component Structs

**CPosition** — Grid position and facing:
```c
typedef struct {
    int x, y;           // Current tile position
    int prevX, prevY;   // Previous tile (for animation interpolation)
    Direction facingDir;
} CPosition;
```

**CStats** — Combat and progression stats:
```c
typedef struct {
    int hp, maxHp;
    int attack, defense;
    int level, expValue;
    int str, dex, intel, con, lck;
    int statPoints;
    bool alive;
    int exp, expToNext;     // Player-only (ignored on monsters)
} CStats;
```

**CSpriteAnim** — Sprite sheet animation:
```c
typedef struct {
    Texture2D* tex;     // Pointer into ResourceManager cache (NOT owned)
    int row;            // Spritesheet row (for direction)
    int frame;          // Current animation frame
    int frameCount;     // Total horizontal frames (0 = static)
    float animTimer;
    float animSpeed;    // Seconds per frame
} CSpriteAnim;
```

**CFallbackColor** — Solid color when no sprite:
```c
typedef struct { Color color; } CFallbackColor;
```

**CAI** — Monster AI state:
```c
typedef struct {
    MonsterType type;
    AttackType  attackType;
    int detectionRange;
    int attackRange;
    int lastSeenX, lastSeenY;
    int huntTurns;
    int shadowTurnCounter;
    EquipType equippedWeapon;
    EquipType equippedArmor;
    int attackCooldown;
} CAI;
```

**CPickup** — Ground item data:
```c
typedef struct {
    bool isEquip;
    ItemType  itemType;     // Valid when !isEquip
    EquipType equipType;    // Valid when isEquip
    int quantity;
} CPickup;
```

**CName** — Display name:
```c
typedef struct { char name[32]; } CName;
```

**CHitFlash** — White flash on damage:
```c
typedef struct { float timer; } CHitFlash;  // Seconds remaining
```

**CAbilities** — Ability slots and MP:
```c
#define MAX_ABILITIES 8
typedef struct {
    AbilityType abilities[MAX_ABILITIES];
    int         cooldowns[MAX_ABILITIES];
    int         count;
    int         mp;
    int         maxMp;
} CAbilities;
```

---

## Game Balance

### Constants (`game/include/game_balance.h`)

| Category | Constant | Value | Description |
|----------|----------|-------|-------------|
| **Map** | `PROCEDURAL_MAP_WIDTH` | 80 | Procedural dungeon width |
| | `PROCEDURAL_MAP_HEIGHT` | 50 | Procedural dungeon height |
| **Floors** | `DEFAULT_MAX_FLOORS` | 10 | Floors before escape |
| | `DEFAULT_START_FLOOR` | 1 | Starting floor |
| **Player** | `PLAYER_BASE_STR/DEX/INT/CON` | 3 | Base stats |
| | `PLAYER_BASE_LCK` | 2 | Base luck |
| | `PLAYER_BASE_ATTACK` | 5 | Base attack |
| | `PLAYER_BASE_DEFENSE` | 1 | Base defense |
| **Stats** | `STAT_CAP_STR/DEX/INT/CON/LCK` | 99 | Per-stat caps |
| | `STAT_CAP_DEFAULT` | 20 | Default cap |
| | `STAT_CAP_UNLIMITED` | 999 | Band of Growth cap |
| | `STAT_POINTS_PER_LEVEL` | 2 | Points per level-up |
| **HP** | `HP_BASE_CONSTANT` | 30 | Base HP |
| | `HP_PER_CON` | 5 | HP per CON point |
| **XP** | `XP_BASE` | 50 | Base XP |
| | `XP_PER_LEVEL` | 40 | XP per level multiplier |
| **Melee** | `STR_MELEE_MULT` | 2 | STR multiplier for melee |
| **Ranged** | `DEX_RANGED_MULT` | 1.5f | DEX multiplier for ranged |
| **Throw** | `DEX_THROW_MULT` | 2 | DEX multiplier for throw |
| | `THROW_RANGE_BASE` | 3 | Base throw range |
| | `THROW_RANGE_PER_DEX_DIV` | 3 | Extra range = dex / 3 |
| **Magic** | `MAGIC_RESIST_MULT` | 3 | INT multiplier for resist |
| **Crit** | `CRIT_MULT` | 2 | Crit damage multiplier |
| | `MEGA_CRIT_THRESHOLD` | 100 | Min crit damage for mega-crit |
| | `MEGA_CRIT_CHANCE` | 50 | % chance to double mega-crit |
| **Dual** | `DUAL_WIELD_OFFHAND_MULT` | 0.5f | Off-hand damage multiplier |
| **Damage** | `MIN_DAMAGE` | 1 | Minimum damage after defense |
| **Dodge** | `DODGE_PER_DEX` | 2 | Dodge % per DEX |
| | `DODGE_CAP_PCT` | 60 | Maximum dodge % |
| **Wait** | `WAIT_HEAL_BASE` | 1 | Base wait heal |
| | `WAIT_HEAL_INTEL_DIV` | 2 | Wait heal = base + int/2 |
| | `WAIT_COOLDOWN` | 0.08f | Cooldown after wait (seconds) |
| **Shadow** | `SHADOW_SPAWN_WAITS` | 15 | Wait turns before shadow |
| | `SHADOW_DOUBLE_MOVE_MIN` | 25 | Shadow double-move threshold |
| | `SHADOW_DOUBLE_MOVE_FAST` | 35 | Shadow fast double-move |
| **Potion** | `POTION_INT_SCALE` | 0.02f | INT scaling for potions |
| | `POTION_HEAL_DENOM` | 10000 | Potion heal denominator |
| **Anim** | `HIT_FLASH_DURATION` | 0.15f | Hit flash seconds |
| | `SPRINT_ANIM_DURATION` | 0.30f | Sprint anim seconds |
| | `DEFAULT_MELEE_COOLDOWN` | 0.15f | Melee cooldown seconds |
| **AI** | `AI_HUNT_TURNS` | 4 | Turns to hunt after losing sight |
| | `AI_WANDER_RANGE_EXTRA` | 8 | Extra wander range |
| | `AI_NEIGHBOR_COUNT` | 4 | Neighbor tiles for AI |
| **Camera** | `DEFAULT_CAMERA_ZOOM` | 4.0f | Default zoom level |
| **UI** | `UI_BASE_WIDTH` | 1024.0f | Base UI width |
| | `UI_BASE_HEIGHT` | 768.0f | Base UI height |
| | `UI_MIN_AUTO_SCALE` | 0.75f | Minimum auto-scale |
| | `UI_GUI_SCALE_MIN` | 1.0f | Minimum GUI scale |
| | `UI_GUI_SCALE_MAX` | 4.0f | Maximum GUI scale |
| **Gold** | `GOLD_RARITY_BONUS_COMMON` | 0 | Common gold bonus |
| | `GOLD_RARITY_BONUS_UNCOMMON` | 50 | Uncommon gold bonus |
| | `GOLD_RARITY_BONUS_RARE` | 150 | Rare gold bonus |
| | `GOLD_RARITY_BONUS_LEGENDARY` | 500 | Legendary gold bonus |
| | `GOLD_SELL_RATIO` | 0.5f | Sell price ratio |
| **MP** | `MP_BASE` | 10 | Base MP |
| | `MP_PER_INT` | 2 | MP per INT |
| | `MP_REGEN_PER_TURN` | 1 | MP regen per turn |
| | `MP_BONUS_ON_KILL` | 3 | MP bonus on kill |
| **CR** | `CON_HP_SCALE` | 5 | CON HP scaling for CR |
| | `STR_ATTACK_SCALE` | 2 | STR attack scaling for CR |
| | `FLOOR_DR_BASE` | 8.0f | Base difficulty rating |
| | `FLOOR_DR_PER_FLOOR` | 4.0f | DR increase per floor |

### Inline Formula Functions

| Function | Signature | Formula |
|----------|-----------|---------|
| `calc_xp_for_level` | `int calc_xp_for_level(int level)` | `XP_BASE + level * XP_PER_LEVEL` |
| `calc_max_hp` | `int calc_max_hp(int con)` | `HP_BASE_CONSTANT + con * HP_PER_CON` |
| `calc_melee_damage` | `int calc_melee_damage(int attack, int str)` | `attack + str * STR_MELEE_MULT` |
| `calc_ranged_damage` | `int calc_ranged_damage(int dex)` | `(int)(dex * DEX_RANGED_MULT)` |
| `calc_throw_damage` | `int calc_throw_damage(int attack, int dex)` | `attack + dex * DEX_THROW_MULT` |
| `calc_magic_damage` | `int calc_magic_damage(int attack, int intel)` | `attack + intel` |
| `calc_magic_resist` | `int calc_magic_resist(int intel)` | `intel * MAGIC_RESIST_MULT` |
| `calc_damage_after_defense` | `int calc_damage_after_defense(int damage, int defense)` | `max(damage - defense, MIN_DAMAGE)` |
| `calc_dodge_chance` | `int calc_dodge_chance(int dex)` | `min(dex * DODGE_PER_DEX, DODGE_CAP_PCT)` |
| `calc_throw_range` | `int calc_throw_range(int dex)` | `max(THROW_RANGE_BASE + dex / 3, THROW_RANGE_BASE)` |
| `calc_wait_heal` | `int calc_wait_heal(int intel)` | `WAIT_HEAL_BASE + intel / WAIT_HEAL_INTEL_DIV` |
| `calc_potion_heal` | `int calc_potion_heal(int maxHp, int healPercent, int intel)` | `(maxHp * healPercent * (int)((1.0 + intel * 0.02) * 100)) / 10000`, min 1 |

---

## World & Game State

### GameWorld (`game/src/world.h`)

The central game state struct. See [Architecture](ARCHITECTURE.md) for field-by-field breakdown. Key fields:

| Field | Type | Description |
|-------|------|-------------|
| `ecs` | `World` | ECS entity pool |
| `playerEntity` | `EntityId` | Player entity ID |
| `monsterGrid[100][100]` | `EntityId` | Spatial hash grid |
| `aliveMonsterCount` | `int` | Cached alive monster count |
| `gold` | `int` | Player gold |
| `state` | `GameState` | Current turn state |
| `currentFloor` | `int` | Current dungeon floor |
| `map` | `MapData*` | Current map |
| `visibility[100][100]` | `unsigned char` | FOW state (0/1/2) |
| `blocking[100][100]` | `unsigned char` | Walkability map |
| `camera` | `Camera2D` | Viewport camera |
| `inventory[16]` | `InventorySlot` | Potion inventory |
| `equipped[5]` | `EquipType` | Equipment slots |
| `equipInventory[16]` | `EquipType` | Equipment bag |
| `damageNumbers` | `DamageNumberPool` | Floating damage pool |
| `floatMsgs` | `FloatMsgPool` | Floating message pool |

### Pool Types

```c
typedef struct {
    bool active;
    char text[16];
    Vector2 pos;
    float timer;
    Color color;
} DamageNumber;

typedef struct { DamageNumber entries[32]; } DamageNumberPool;

typedef struct {
    bool active;
    char text[64];
    float worldX, worldY;
    float velY;
    float alpha;
    float lifetime;
    Color color;
} FloatMsg;

typedef struct { FloatMsg entries[16]; } FloatMsgPool;
```

### Functions

| Function | Signature | Description |
|----------|-----------|-------------|
| `GameWorld_Init` | `void GameWorld_Init(GameWorld* gw)` | Zero entire struct, init ECS, set player/selected to ENTITY_NONE. |
| `DamageNumber_Spawn` | `void DamageNumber_Spawn(DamageNumberPool* pool, int value, int tileX, int tileY, int tw, int th, Color color)` | Spawn floating damage number at tile position. Finds first inactive slot. |
| `DamageNumber_UpdateAll` | `void DamageNumber_UpdateAll(DamageNumberPool* pool, float dt)` | Update all active damage numbers: decrement timer, float upward, deactivate when expired. |
| `FloatMsg_Spawn` | `void FloatMsg_Spawn(GameWorld* gw, int tileX, int tileY, Color color, const char* fmt, ...)` | Spawn floating status message with printf-style formatting. |
| `FloatMsg_UpdateAll` | `void FloatMsg_UpdateAll(GameWorld* gw, float dt)` | Update all active float messages: decrement lifetime, float upward, fade alpha. |

---

## Game Bridge

### Game (`game/src/game.h`)

| Function | Signature | Description |
|----------|-----------|-------------|
| `InitGame` | `bool InitGame(GameWorld* game, const char* tmxFile)` | Initialize a new game. Loads TMX map (or generates procedural), spawns player, gives starting equipment (survival knife + small potion), calls `Floor_InitNewFloor()`. Returns false on failure. |
| `CleanupGame` | `void CleanupGame(GameWorld* game)` | Unload all textures, free fallback tileset, unload map, zero struct. |
| `UpdateGame` | `void UpdateGame(GameWorld* game)` | Per-frame update: decrement timers, update damage numbers/float messages, update animations, process enemy turn (calls `AISystem_ProcessAll()`), check floor clear/escape conditions. |
| `DescendFloor` | `void DescendFloor(GameWorld* game)` | Transition to next floor: save player state, regenerate map, restore player, re-apply equipment, call `Floor_InitNewFloor()`. |
| `SetGuiScale` | `void SetGuiScale(float scale)` | Set GUI scale (clamped to [1.0, 4.0]). |
| `GetGuiScale` | `float GetGuiScale(void)` | Get current GUI scale setting. |
| `GetUIScale` | `float GetUIScale(void)` | Get computed UI scale (auto-scale × GUI setting). |

---

## Equipment Bonus

### Equipment Bonus (`game/include/equipment_bonus.h`)

| Function | Signature | Description |
|----------|-----------|-------------|
| `EquipmentBonus_Apply` | `void EquipmentBonus_Apply(World* w, EntityId unit, EquipType type)` | Add stat bonuses from equipment to entity's CStats. Safe no-op for NULL world, ENTITY_NONE, EQUIP_NONE, or entities without COMP_STATS. Calls `Recalculate()` after applying. |
| `EquipmentBonus_Remove` | `void EquipmentBonus_Remove(World* w, EntityId unit, EquipType type)` | Subtract stat bonuses. Same safety guarantees as Apply. |
| `EquipmentBonus_Recalculate` | `void EquipmentBonus_Recalculate(World* w, EntityId unit)` | Recalculate maxHp from CON. If maxHp increased, adds the difference to current HP. Clamps HP to [1, maxHp]. |

---

## Floor Initialization

### Floor Init (`game/include/floor_init.h`)

| Function | Signature | Description |
|----------|-----------|-------------|
| `Floor_InitNewFloor` | `void Floor_InitNewFloor(GameWorld* game)` | Per-floor setup: build blocking map, init monster templates, select biome, spawn TMX entities, spawn monsters (CR-budget), spawn pickups, optional shop room (3% chance), set stair position, reset turn state, clear FOW, reveal FOW from player position, position camera. |

---

## Validation

### Validation (`game/include/validation.h`)

All functions are **pure predicates** — no side effects, no logging.

| Function | Signature | Description |
|----------|-----------|-------------|
| `Validate_InventorySlot` | `bool Validate_InventorySlot(int slot, int slotCount)` | True if `0 <= slot < slotCount`. |
| `Validate_EquipType` | `bool Validate_EquipType(EquipType type)` | True if `EQUIP_NONE < type < EQUIP_COUNT`. |
| `Validate_ItemType` | `bool Validate_ItemType(ItemType type)` | True if `ITEM_NONE < type < ITEM_COUNT`. |
| `Validate_MonsterType` | `bool Validate_MonsterType(MonsterType type)` | True if `0 <= type < MONSTER_TYPE_COUNT`. |
| `Validate_StatIndex` | `bool Validate_StatIndex(int statIdx)` | True if `0 <= statIdx <= 4`. |
| `Validate_Floor` | `bool Validate_Floor(int floor)` | True if `floor > 0`. |
| `Clamp_Int` | `int Clamp_Int(int value, int min, int max)` | Returns value clamped to [min, max]. |

---

## Inventory & Equipment

### Inventory (`game/src/inventory.h`)

#### Types

```c
#define MAX_POTIONS          32
#define MAX_EQUIP_ON_MAP     64
#define MAX_INVENTORY_SLOTS  16
#define EQUIP_SLOT_COUNT      5
#define MAX_EQUIPMENT_TYPES  32

typedef struct EquipData {
    EquipType type;
    char name[32];
    const char* description;
    const char* spritePath;
    EquipCategory category;
    EquipSlot slot;
    int bonusAttack, bonusDefense, bonusMaxHp;
    int bonusStr, bonusDex, bonusInt, bonusCon, bonusLck;
    bool twoHanded;
    bool isRanged;
    ItemRarity rarity;
} EquipData;

typedef struct { ItemType type; int quantity; } InventorySlot;

typedef enum { INV_BROWSE, INV_ACTION_MENU } InventorySubState;
typedef enum { INV_TAB_INVENTORY = 0, INV_TAB_EQUIPMENT, INV_TAB_STATS, INV_TAB_COUNT } InventoryTab;
```

#### Functions

| Function | Signature | Description |
|----------|-----------|-------------|
| `GetEquipData` | `const EquipData* GetEquipData(EquipType type)` | Look up equipment data by type. Returns NULL for invalid types. |
| `GetEquipRangeBonus` | `int GetEquipRangeBonus(EquipType type)` | Returns ranged attack range bonus for bows (4–8), 0 for non-ranged. |
| `GetItemName` | `const char* GetItemName(ItemType type)` | Returns potion display name. Empty string for invalid types. |
| `GetItemHealAmount` | `int GetItemHealAmount(ItemType type)` | Returns heal percentage (25/50/75). 0 for invalid types. |
| `GetItemDescription` | `const char* GetItemDescription(ItemType type)` | Returns potion description text. |
| `InventoryAdd` | `bool InventoryAdd(GameWorld* game, ItemType type)` | Add potion to inventory. Stacks if already present. Returns false if full. |
| `InventoryUse` | `bool InventoryUse(GameWorld* game, int slot)` | Use potion at slot. Heals player, spawns damage number, decrements quantity. Returns false for invalid slot. |
| `EquipItem` | `bool EquipItem(GameWorld* game, EquipType type)` | Equip item. Handles dual-wield redirect, two-handed conflicts, auto-unequip. Applies bonuses. Updates weapon ability. |
| `EquipItemSilent` | `void EquipItemSilent(GameWorld* game, EquipType type)` | Equip without UI feedback. Removes old item first. |
| `UnequipSlot` | `void UnequipSlot(GameWorld* game, EquipSlot slot)` | Unequip item from slot. Removes bonuses. Adds to equipment bag or drops on floor if bag full. |
| `IsEquipSlotOccupied` | `bool IsEquipSlotOccupied(const GameWorld* game, EquipSlot slot)` | True if slot has an item. |
| `IsTwoHandedEquipped` | `bool IsTwoHandedEquipped(const GameWorld* game)` | True if weapon slot has a two-handed weapon. |
| `IsWeaponDualWieldable` | `bool IsWeaponDualWieldable(EquipType type)` | True if weapon is single-handed, not ranged, and is a weapon. |
| `IsDualWielding` | `bool IsDualWielding(const GameWorld* game)` | True if both weapon and off-hand have dual-wieldable weapons. |
| `AddEquipToInventory` | `bool AddEquipToInventory(GameWorld* game, EquipType type)` | Add equipment to bag. Returns false if full. |
| `RemoveEquipFromInventory` | `bool RemoveEquipFromInventory(GameWorld* game, int slot)` | Remove equipment from bag by index. Shifts remaining items. |
| `LoadPotionTextures` | `void LoadPotionTextures(GameWorld* game)` | Pre-load potion and UI textures into resource cache. |
| `Inventory_LoadEquipTexture` | `Texture2D* Inventory_LoadEquipTexture(EquipType type)` | Load equipment icon texture. Returns NULL for invalid types. |
| `Inventory_LoadPotionTexture` | `Texture2D* Inventory_LoadPotionTexture(ItemType type)` | Load potion texture. Returns NULL for invalid types. |
| `Inventory_GetWeaponAbility` | `AbilityType Inventory_GetWeaponAbility(EquipType weapon)` | Maps weapon type to ability type (PUNCH, LIGHT_MELEE, HEAVY_MELEE, RANGED). |

---

## Systems

### AI System (`game/src/systems/ai_system.h`)

| Function | Signature | Description |
|----------|-----------|-------------|
| `AI_GetActiveAbility` | `AbilityType AI_GetActiveAbility(GameWorld* gw, EntityId entity)` | Determine monster's active ability from equipped weapon or attack type. |
| `AISystem_ProcessAll` | `bool AISystem_ProcessAll(GameWorld* gw, int timeWaited)` | Process all monster AI turns. Returns false if player dies. Handles hunt, wander, flank, kite, ranged, shadow double-move. |

### Combat System (`game/src/systems/combat_system.h`)

| Function | Signature | Description |
|----------|-----------|-------------|
| `CombatSystem_PlayerMeleeAttack` | `bool CombatSystem_PlayerMeleeAttack(GameWorld* game, EntityId attackerId, int targetX, int targetY)` | Execute melee attack on tile. Finds monster via spatial hash, calculates damage (melee formula + crit + mega-crit + dual-wield), checks dodge, applies damage, handles death (XP, loot, gold, spatial hash removal). Returns true if a monster was targeted. |
| `CombatSystem_PlayerRangedAttack` | `bool CombatSystem_PlayerRangedAttack(GameWorld* game, EntityId attackerId)` | Fire ranged attack in facing direction. Traces projectile path, finds first monster hit, calculates damage (ranged formula). Returns true if turn consumed. |
| `CombatSystem_PlayerThrowWeapon` | `bool CombatSystem_PlayerThrowWeapon(GameWorld* game, EntityId attackerId)` | Throw equipped melee weapon. Unequips weapon, traces path, calculates damage (throw formula). Returns true if weapon was thrown. |

### Input System (`game/src/systems/input_system.h`)

| Function | Signature | Description |
|----------|-----------|-------------|
| `InputSystem_Inventory` | `void InputSystem_Inventory(GameWorld* game, InventoryUIState* ui)` | Handle keyboard input during inventory state. Tab switching (Q/E), item selection (W/S), action menu (Enter), equip/use/drop, stat allocation. |
| `InputSystem_PlayerTurn` | `void InputSystem_PlayerTurn(GameWorld* game, InventoryUIState* ui)` | Handle keyboard input during player turn. WASD/arrows for movement, F for attack, T for throw, Space for wait, I for inventory, M for map, E for interact. |

### Movement System (`game/src/systems/movement_system.h`)

| Function | Signature | Description |
|----------|-----------|-------------|
| `MovementSystem_IsWalkable` | `bool MovementSystem_IsWalkable(GameWorld* gw, int x, int y)` | True if tile is in bounds, not blocked, and no monster present. |
| `MovementSystem_PlayerMove` | `void MovementSystem_PlayerMove(GameWorld* gw, Direction dir)` | Execute single-step player movement. Updates position, triggers FOW reveal, collects pickups, checks stairs/escape, transitions to enemy turn. |
| `MovementSystem_UpdateAnimations` | `void MovementSystem_UpdateAnimations(GameWorld* gw, float dt)` | Update animation interpolation timers for all entities. |

### Render System (`game/src/systems/render_system.h`)

| Function | Signature | Description |
|----------|-----------|-------------|
| `RenderSystem_DrawMap` | `void RenderSystem_DrawMap(const GameWorld* gw)` | Draw all visible tile layers with FOW overlay. visibility=1 is full brightness, visibility=2 is darkened. |
| `RenderSystem_DrawMonsters` | `void RenderSystem_DrawMonsters(GameWorld* gw, float monsterT)` | Draw all visible alive monsters with movement interpolation and sprite animation. |
| `RenderSystem_World` | `void RenderSystem_World(GameWorld* gw)` | Combined world rendering (map + entities + projectile). |
| `RenderSystem_HUD` | `void RenderSystem_HUD(GameWorld* gw)` | HUD rendering (health bar, floor label). |

### Renderer (`game/src/systems/renderer.h`)

| Function | Signature | Description |
|----------|-----------|-------------|
| `GetUIScale` | `float GetUIScale(void)` | Compute UI scale from window size and GUI setting. |
| `Draw9Slice` | `void Draw9Slice(Texture2D tex, Rectangle dest, int l, int t, int r, int b)` | Draw a 9-slice textured rectangle for UI panels. |
| `RenderGame` | `void RenderGame(GameWorld* game, const InventoryUIState* ui)` | Master render function. Draws everything: world (BeginMode2D), HUD, overlays (screen-space). |

### Spawner System (`game/src/systems/spawner_system.h`)

| Function | Signature | Description |
|----------|-----------|-------------|
| `SpawnerSystem_SpawnMonster` | `EntityId SpawnerSystem_SpawnMonster(GameWorld* gw, MonsterType type, int x, int y, int floor)` | Create monster ECS entity from template with floor-scaled stats. Adds to spatial hash. Increments alive count. |
| `SpawnerSystem_SpawnByTypeName` | `EntityId SpawnerSystem_SpawnByTypeName(GameWorld* gw, const char* tmxTypeName, int x, int y, int floor)` | Spawn monster by TMX type name string. |
| `SpawnerSystem_ConfigureShadow` | `void SpawnerSystem_ConfigureShadow(GameWorld* gw, EntityId shadow, int playerLevel)` | Configure shadow monster stats based on player level. |
| `SpawnerSystem_SpawnMonsters` | `void SpawnerSystem_SpawnMonsters(GameWorld* gw, const ProceduralRoom* rooms, int roomCount, int playerX, int playerY)` | Populate rooms with monsters from TMX data. |
| `SpawnMonstersForFloor` | `void SpawnMonstersForFloor(GameWorld* game)` | CR-budget spawner: selects monsters based on biome, floor budget, and challenge rating. |
| `SpawnShopRoom` | `void SpawnShopRoom(GameWorld* game)` | Spawn shop NPC in a random room. |
| `SpawnerSystem_SpawnPickups` | `void SpawnerSystem_SpawnPickups(GameWorld* gw)` | Spawn loot pickups from TMX objects. |
| `SpawnerSystem_FindPickupAt` | `EntityId SpawnerSystem_FindPickupAt(GameWorld* gw, int x, int y)` | Find pickup entity at tile. Returns ENTITY_NONE if none. |
| `SpawnerSystem_CollectPickupsAt` | `int SpawnerSystem_CollectPickupsAt(GameWorld* gw, int x, int y, ItemType* outTypes, int* outQtys, int maxSlots)` | Collect all potion pickups at tile. Returns count collected. |
| `SpawnerSystem_CollectEquipAt` | `int SpawnerSystem_CollectEquipAt(GameWorld* gw, int x, int y, EquipType* outTypes, int* outQtys, int maxSlots)` | List equipment pickups at tile without removing. |
| `SpawnerSystem_ReduceEquipAt` | `void SpawnerSystem_ReduceEquipAt(GameWorld* gw, int x, int y, EquipType type, int amount)` | Reduce stacked equipment quantity after partial pickup. |
| `SpawnerSystem_AddPotionAt` | `void SpawnerSystem_AddPotionAt(GameWorld* gw, int x, int y, ItemType type, int qty)` | Drop or stack a potion pickup on the map. |
| `SpawnerSystem_AddEquipAt` | `void SpawnerSystem_AddEquipAt(GameWorld* gw, int x, int y, EquipType type, int qty)` | Drop or stack an equipment pickup on the map. |
| `SpawnerSystem_SpawnHealingPotionAt` | `void SpawnerSystem_SpawnHealingPotionAt(GameWorld* gw, int x, int y)` | Spawn random rarity healing potion at tile. |
| `SpawnerSystem_ListPotionsAt` | `int SpawnerSystem_ListPotionsAt(GameWorld* gw, int x, int y, ItemType* outTypes, int* outQtys, int maxSlots)` | List potions at tile without collecting. |
| `SpawnerSystem_ListEquipAt` | `int SpawnerSystem_ListEquipAt(GameWorld* gw, int x, int y, EquipType* outTypes, int* outQtys, int maxSlots)` | List equipment at tile without collecting. |

### Player System (`game/src/systems/player_system.h`)

| Function | Signature | Description |
|----------|-----------|-------------|
| `PlayerSystem_Spawn` | `void PlayerSystem_Spawn(GameWorld* gw)` | Create player entity with all components: position (1,1), base stats, sprite, fallback color, name, player tag, hit flash, abilities (PUNCH). |

### Player (`game/src/systems/player.h`)

| Function | Signature | Description |
|----------|-----------|-------------|
| `ExpForLevel` | `int ExpForLevel(int level)` | XP required for given level. Wraps `calc_xp_for_level()`. |
| `GainExperience` | `void GainExperience(GameWorld* game, int amount)` | Add XP to player. Triggers level-ups (possibly multiple). |
| `AllocateStatPoint` | `bool AllocateStatPoint(GameWorld* game, CStats* s, int statIdx)` | Spend one stat point (0=STR, 1=DEX, 2=INT, 3=CON, 4=LCK). Respects stat caps. Recalculates maxHp. |

### Ability System (`game/src/systems/ability_system.h`)

| Function | Signature | Description |
|----------|-----------|-------------|
| `AbilitySystem_Use` | `bool AbilitySystem_Use(GameWorld* gw, EntityId caster, EntityId target, AbilityType type)` | Execute ability. **STUB — returns false.** |
| `AbilitySystem_TickCooldowns` | `void AbilitySystem_TickCooldowns(GameWorld* gw, EntityId entity)` | Decrement ability cooldowns. **STUB — no-op.** |

### Spatial Hash (`game/src/systems/spatial_hash.h`)

| Function | Signature | Description |
|----------|-----------|-------------|
| `SpatialHash_Clear` | `void SpatialHash_Clear(GameWorld* gw)` | Zero entire monsterGrid. |
| `SpatialHash_Add` | `void SpatialHash_Add(GameWorld* gw, EntityId e, int x, int y)` | Set `monsterGrid[y][x] = e`. Bounds-checked. |
| `SpatialHash_Remove` | `void SpatialHash_Remove(GameWorld* gw, EntityId e, int x, int y)` | Clear tile if it contains `e`. |
| `SpatialHash_Move` | `void SpatialHash_Move(GameWorld* gw, EntityId e, int oldX, int oldY, int newX, int newY)` | Remove from old tile, add to new tile. |

### World Monster (`game/src/systems/world_monster.h`)

| Function | Signature | Description |
|----------|-----------|-------------|
| `World_FindMonsterAt` | `EntityId World_FindMonsterAt(GameWorld* gw, int x, int y, EntityId exclude)` | O(1) lookup via spatial hash. Validates alive, has AI, not player. `exclude` skips a specific entity. |
| `World_CountAliveMonsters` | `int World_CountAliveMonsters(GameWorld* gw)` | Returns cached `aliveMonsterCount`. |
| `World_AreAllMonstersDead` | `bool World_AreAllMonstersDead(GameWorld* gw)` | True if `aliveMonsterCount <= 0`. |
| `World_UpdateMonsterAnimations` | `void World_UpdateMonsterAnimations(GameWorld* gw, float dt)` | Tick sprite frames and hit flash timers for all monsters. |

---

## Data Modules

### Monster Data (`game/src/data/monster_data.h`)

```c
typedef enum { PROJ_ARROW, PROJ_SPORE, PROJ_FIREBALL, PROJ_ROCK, PROJ_SHADOW } ProjectileVisual;

typedef struct {
    MonsterType type;
    const char* tmxTypeName;
    char name[32];
    int hp, attack, defense, expValue, level;
    int str, dex, intel, con, lck;
    Color color;
    const char* spritePath;
    int frameCount;
    float animSpeed;
    int detectionRange;
    int minFloor, maxFloor, maxLevel;
    int spawnWeight;
    AttackType attackType;
    int attackRange;
    EquipType weaponPool[4]; int weaponPoolCount;
    EquipType armorPool[4];  int armorPoolCount;
    int equipDropChance;
    ProjectileVisual projectileVisual;
} MonsterTemplate;
```

| Function | Signature | Description |
|----------|-----------|-------------|
| `Monster_InitTemplates` | `void Monster_InitTemplates(void)` | Initialize all 11 monster templates with stats. |
| `Monster_GetTemplate` | `const MonsterTemplate* Monster_GetTemplate(MonsterType type)` | Look up template by type. |
| `Monster_FindTypeByTmxName` | `MonsterType Monster_FindTypeByTmxName(const char* tmxTypeName)` | Map TMX type string to MonsterType. Returns MONSTER_TYPE_COUNT if not found. |
| `Monster_CalcCR` | `float Monster_CalcCR(const MonsterTemplate* def, int currentFloor)` | Calculate challenge rating for a monster on a given floor. |
| `Monster_SnapCRToTier` | `float Monster_SnapCRToTier(float rawCR)` | Snap CR to nearest tier boundary. |
| `Monster_GetFloorBudget` | `float Monster_GetFloorBudget(int floorNumber)` | Calculate total CR budget for a floor. |

### Loot Data (`game/src/data/loot_data.h`)

```c
typedef enum { LOOT_TYPE_POTION, LOOT_TYPE_EQUIP } LootCategory;
typedef struct { LootCategory cat; int typeId; int tier; int baseWeight; } LootEntry;
extern const LootEntry LOOT_TABLE[];
extern const int LOOT_TABLE_COUNT;
```

### Equipment Data (`game/src/data/equip_data.h`)

```c
typedef enum { RARITY_COMMON, RARITY_UNCOMMON, RARITY_RARE, RARITY_LEGENDARY } ItemRarity;
```

| Function | Signature | Description |
|----------|-----------|-------------|
| `Equip_GetPowerScore` | `int Equip_GetPowerScore(const EquipData* d)` | Calculate equipment power score from bonuses. |
| `Equip_GetBasePrice` | `int Equip_GetBasePrice(const EquipData* d)` | Calculate buy price from power score and rarity. |
| `Equip_GetSellPrice` | `int Equip_GetSellPrice(const EquipData* d)` | Calculate sell price (base × GOLD_SELL_RATIO). |

### Biome Data (`game/src/data/biome_data.h`)

```c
typedef enum { BIOME_NONE = 0, BIOME_GOBLIN_DEN, BIOME_COUNT } BiomeType;
typedef struct {
    BiomeType id; const char* name;
    int minFloor, maxFloor, spawnWeight;
    MonsterType monsterPool[16]; int monsterCount;
    const char* tilesetPath;
} BiomeDef;
```

| Function | Signature | Description |
|----------|-----------|-------------|
| `GetBiomeData` | `const BiomeDef* GetBiomeData(BiomeType type)` | Look up biome definition. |
| `Biome_SelectForFloor` | `BiomeType Biome_SelectForFloor(int floorNumber)` | Select biome for a floor number (weighted random). |

### Ability Data (`game/src/data/ability_data.h`)

```c
typedef enum { ABILITY_NONE, ABILITY_PUNCH, ABILITY_LIGHT_MELEE, ABILITY_HEAVY_MELEE, ABILITY_RANGED, ABILITY_COUNT } AbilityType;
typedef struct { AbilityType id; const char* name; const char* description; int mpCost; int cooldownMax; } AbilityData;
```

| Function | Signature | Description |
|----------|-----------|-------------|
| `GetAbilityData` | `const AbilityData* GetAbilityData(AbilityType type)` | Look up ability metadata. |

---

## Map Modules

### TMX Loader (`game/src/map/tmx/tmx.h`)

```c
#define MAX_TILESETS 8
#define MAX_LAYERS   8
#define MAX_OBJECTS  64

typedef struct { int firstGID; char name[64]; int tileWidth, tileHeight, tileCount, columns; char imageSource[256]; int imageWidth, imageHeight; } TilesetDef;
typedef struct { char name[64]; int width, height; int* data; bool visible; } TileLayer;
typedef struct { char name[64]; char type[32]; float x, y; int gid; bool hasGID; } MapObject;
typedef struct {
    int width, height, tileWidth, tileHeight;
    char orientation[16];
    int tilesetCount; TilesetDef tilesets[MAX_TILESETS];
    int layerCount;   TileLayer layers[MAX_LAYERS];
    int objectCount;  MapObject objects[MAX_OBJECTS];
} MapData;
```

| Function | Signature | Description |
|----------|-----------|-------------|
| `LoadTMX` | `MapData* LoadTMX(const char* filename)` | Parse TMX XML file. Returns heap-allocated MapData or NULL. |
| `UnloadTMX` | `void UnloadTMX(MapData* map)` | Free MapData and all layer data arrays. |
| `GetTileGID` | `int GetTileGID(const MapData* map, int layerIndex, int x, int y)` | Get tile GID at position. Returns 0 for empty/OOB. |
| `GetTileSourceRect` | `Rectangle GetTileSourceRect(const MapData* map, int gid)` | Compute sprite source rectangle for a tile GID. |
| `FindTilesetForGID` | `int FindTilesetForGID(const MapData* map, int gid)` | Find which tileset contains the GID. Returns -1 if not found. |

### Procedural Generator (`game/src/map/procedural.h`)

```c
#define MAX_GENERATED_ROOMS 8
typedef struct { int x, y, w, h, cx, cy; } ProceduralRoom;
```

| Function | Signature | Description |
|----------|-----------|-------------|
| `ProceduralMap_SetBiome` | `void ProceduralMap_SetBiome(BiomeType biome)` | Set biome for next generation. Must be called before `GenerateProceduralMap()`. |
| `GenerateProceduralMap` | `MapData* GenerateProceduralMap(int width, int height, int generateStairs)` | Generate dungeon. Returns heap-allocated MapData. If `generateStairs` is non-zero, places stair tile. |
| `GetGeneratedRooms` | `int GetGeneratedRooms(ProceduralRoom* outRooms, int maxRooms)` | Copy room metadata from last generation. Returns room count. |
| `IsFloorGID` | `int IsFloorGID(int gid)` | Returns 1 if GID is any floor variant. |
| `GetStairX` / `GetStairY` | `int GetStairX(void)` / `int GetStairY(void)` | Get stair tile position from last generation. |

### Map Helpers (`game/src/map/map_helpers.h`)

| Function | Signature | Description |
|----------|-----------|-------------|
| `IsInRoom` | `bool IsInRoom(int x, int y)` | True if tile is inside any generated room. |
| `RevealFOW` | `void RevealFOW(GameWorld* game)` | Cast Bresenham rays from player (FOG_RADIUS=7). Sets visibility to 1 (seen) or 2 (explored). |
| `SpawnEscapeTile` | `void SpawnEscapeTile(GameWorld* game)` | Place escape tile on final floor. |
| `SpawnShadow` | `void SpawnShadow(GameWorld* game)` | Create shadow pursuer entity. |
| `BuildBlockingMap` | `void BuildBlockingMap(GameWorld* game)` | Mark walls and obstacles in blocking array. |
| `SpawnEntitiesFromObjects` | `void SpawnEntitiesFromObjects(GameWorld* game)` | Convert TMX object layer to ECS entities (monsters, pickups, healing objects). |
| `TileToScreen` | `Vector2 TileToScreen(int x, int y, int tw, int th)` | Convert tile coordinates to pixel coordinates. |

---

## UI Modules

### Menu (`game/src/ui/menu.h`)

```c
typedef enum { MENU_NONE, MENU_PLAY, MENU_SETTINGS, MENU_CONTROLS, MENU_STORY, MENU_CREDITS, MENU_EXIT } MenuAction;
```

| Function | Signature | Description |
|----------|-----------|-------------|
| `Menu_Update` | `MenuAction Menu_Update(void)` | Process main menu input. Returns selected action. |
| `Menu_Render` | `void Menu_Render(void)` | Draw main menu. |
| `Menu_CreditsUpdate` | `MenuAction Menu_CreditsUpdate(void)` | Process credits screen input. |
| `Menu_CreditsRender` | `void Menu_CreditsRender(void)` | Draw credits screen. |
| `Menu_SettingsUpdate` | `MenuAction Menu_SettingsUpdate(void)` | Process settings screen input. |
| `Menu_SettingsRender` | `void Menu_SettingsRender(void)` | Draw settings screen. |
| `Menu_SettingsUpdateGame` | `void Menu_SettingsUpdateGame(void)` | Process in-game settings overlay input. |
| `Menu_SettingsRenderGame` | `void Menu_SettingsRenderGame(void)` | Draw in-game settings overlay. |
| `Menu_StoryUpdate` | `MenuAction Menu_StoryUpdate(void)` | Process story screen input. |
| `Menu_StoryRender` | `void Menu_StoryRender(void)` | Draw story screen. |
| `Menu_ControlsUpdate` | `MenuAction Menu_ControlsUpdate(void)` | Process controls screen input. |
| `Menu_ControlsRender` | `void Menu_ControlsRender(void)` | Draw controls screen. |
| `GameMenu_Update` | `MenuAction GameMenu_Update(void)` | Process in-game pause menu input. |
| `GameMenu_Render` | `void GameMenu_Render(void)` | Draw in-game pause menu overlay. |
| `Menu_Reset` | `void Menu_Reset(void)` | Reset menu state. |
| `Menu_ResetStory` | `void Menu_ResetStory(void)` | Reset story screen state. |
| `Menu_ResetSettings` | `void Menu_ResetSettings(void)` | Reset settings selection. |

### Inventory UI (`game/src/ui/inventory_ui.h`)

```c
typedef struct InventoryUIState {
    int selection;
    int scrollOffset;
    int statsScrollCol1, statsScrollCol2;
    int statsActiveCol;
    int statsSelection;
    InventorySubState subState;
    InventoryTab tab;
    int actionSelection;
} InventoryUIState;
```

| Function | Signature | Description |
|----------|-----------|-------------|
| `InventoryUI_Init` | `void InventoryUI_Init(InventoryUIState* ui)` | Reset UI state to defaults. |
| `InventoryUI_Render` | `void InventoryUI_Render(GameWorld* game, const InventoryUIState* ui)` | Draw inventory overlay with 3 tabs. |

### Inspector (`game/src/ui/inspector.h`)

```c
typedef enum { INSPECTOR_MONSTER, INSPECTOR_ITEM } InspectorType;
```

| Function | Signature | Description |
|----------|-----------|-------------|
| `Inspector_Render` | `void Inspector_Render(const GameWorld* game, InspectorType type, int x, int y, int w, int h, int selectedSlot)` | Draw monster or item info panel. |

### Shop UI (`game/src/ui/shop_ui.h`)

| Function | Signature | Description |
|----------|-----------|-------------|
| `ShopUI_Render` | `void ShopUI_Render(GameWorld* game, float scale)` | Draw shop interface. |
| `ShopUI_HandleInput` | `void ShopUI_HandleInput(GameWorld* game)` | Process shop buy/sell input. |

### Map UI (`game/src/ui/map_ui.h`)

| Function | Signature | Description |
|----------|-----------|-------------|
| `MapUI_Update` | `void MapUI_Update(GameWorld* gw)` | Handle minimap toggle (M/Z keys). |
| `MapUI_Render` | `void MapUI_Render(const GameWorld* gw)` | Draw minimap overlay. |

### Text Data (`game/src/ui/text_data.h`)

| Function | Signature | Description |
|----------|-----------|-------------|
| `RenderTextScreen` | `void RenderTextScreen(const unsigned char* data, int len, const char* backHint, int scrollOffset)` | Decrypt and render XOR-encoded text with scrolling. |

---

## Audio

### Game Audio (`game/src/game_audio.h`)

| Function | Signature | Description |
|----------|-----------|-------------|
| `GameAudio_Init` | `void GameAudio_Init(void)` | Register 2 music contexts (menu, game) and 5 sound categories (hit, pickup, ranged, magic, levelup). Set initial context to menu. |
| `GameAudio_SetContext` | `void GameAudio_SetContext(bool inGame)` | Switch music context: true = game music, false = menu music. |
| `GameAudio_PlayHitSound` | `void GameAudio_PlayHitSound(void)` | Play random hit sound. |
| `GameAudio_PlayPickupSound` | `void GameAudio_PlayPickupSound(void)` | Play random pickup sound. |
| `GameAudio_PlayRangedAttackSound` | `void GameAudio_PlayRangedAttackSound(void)` | Play random ranged attack sound. |
| `GameAudio_PlayMagicAttackSound` | `void GameAudio_PlayMagicAttackSound(void)` | Play random magic attack sound. |
| `GameAudio_PlayLevelUpSound` | `void GameAudio_PlayLevelUpSound(void)` | Play random level-up sound. |
