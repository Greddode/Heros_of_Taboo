# Adding New Features — Starting Points

**Heroes of Taboo** — v0.0.10 | June 5, 2026

---

## Table of Contents

1. [How to Add a New ECS System](#how-to-add-a-new-ecs-system)
2. [How to Add a New Component](#how-to-add-a-new-component)
3. [How to Add a New Monster](#how-to-add-a-new-monster)
4. [How to Add a New Equipment Item](#how-to-add-a-new-equipment-item)
5. [How to Add a New Potion/Item](#how-to-add-a-new-potionitem)
6. [How to Add a New Ability](#how-to-add-a-new-ability)
7. [How to Add a New Biome](#how-to-add-a-new-biome)
8. [How to Add a New UI Screen](#how-to-add-a-new-ui-screen)
9. [How to Add a New Game State](#how-to-add-a-new-game-state)
10. [How to Add a New Sound Effect](#how-to-add-a-new-sound-effect)
11. [How to Modify Combat Formulas](#how-to-modify-combat-formulas)
12. [How to Add a New Status Effect](#how-to-add-a-new-status-effect)
13. [How to Add a New Map Feature](#how-to-add-a-new-map-feature)

---

## How to Add a New ECS System

**Example**: A weather system that affects visibility and combat.

### Step 1: Create the header

`game/src/systems/weather_system.h`:
```c
#ifndef WEATHER_SYSTEM_H
#define WEATHER_SYSTEM_H

#include "world.h"

typedef enum { WEATHER_CLEAR, WEATHER_RAIN, WEATHER_FOG, WEATHER_STORM } WeatherType;

void WeatherSystem_Update(GameWorld* gw, float dt);
void WeatherSystem_Render(const GameWorld* gw);
WeatherType WeatherSystem_GetCurrent(const GameWorld* gw);

#endif
```

### Step 2: Add state to GameWorld

In `game/src/world.h`, add to the `GameWorld` struct:
```c
// Weather
WeatherType currentWeather;
float weatherTimer;
```

### Step 3: Implement the system

`game/src/systems/weather_system.c`:
```c
#include "weather_system.h"

void WeatherSystem_Update(GameWorld* gw, float dt) {
    if (!gw) return;
    gw->weatherTimer -= dt;
    if (gw->weatherTimer <= 0.0f) {
        // Change weather
        gw->currentWeather = (WeatherType)GetRandomValue(0, 3);
        gw->weatherTimer = GetRandomValue(30, 120);
    }
}

void WeatherSystem_Render(const GameWorld* gw) {
    // Draw rain particles, fog overlay, etc.
}

WeatherType WeatherSystem_GetCurrent(const GameWorld* gw) {
    return gw ? gw->currentWeather : WEATHER_CLEAR;
}
```

### Step 4: Integrate into game loop

In `game/src/game.c`, `UpdateGame()`:
```c
#include "systems/weather_system.h"

void UpdateGame(GameWorld* game) {
    // ... existing code ...
    WeatherSystem_Update(game, GetFrameTime());
}
```

In `game/src/systems/renderer.c`, `RenderGame()`:
```c
// After EndMode2D(), before HUD:
WeatherSystem_Render(game);
```

### Step 5: Initialize in GameWorld_Init

In `game/src/world.c`:
```c
void GameWorld_Init(GameWorld* gw) {
    // ... existing code ...
    gw->currentWeather = WEATHER_CLEAR;
    gw->weatherTimer = 60.0f;
}
```

### Step 6: Write tests

Add to `tests/test_runner.c`:
```c
static bool test_weather_system_updates(void) {
    GameWorld gw;
    GameWorld_Init(&gw);
    gw.weatherTimer = 0.01f;
    WeatherSystem_Update(&gw, 0.02f);
    // Timer should have reset (weather changed)
    CHECK(gw.weatherTimer > 0.0f, "weather timer reset after expiry");
    TEST_PASS();
    return true;
}
```

### Key Rules

- **No static variables** in the system `.c` file — all state in `GameWorld`
- **Include `world.h`** — it provides access to all game state
- **Check for NULL** — every function should handle `gw == NULL` gracefully
- **Update `premake5.lua`** if the build doesn't auto-detect new files

---

## How to Add a New Component

**Example**: A `CPoison` component for poison damage over time.

### Step 1: Define the component in `components.h`

```c
#define COMP_POISON (1u << 10)

typedef struct {
    int damagePerTurn;
    int remainingTurns;
} CPoison;
```

### Step 2: Add to World struct in `ecs.h`

```c
typedef struct World {
    // ... existing arrays ...
    CPoison poisons[MAX_ENTITIES];
} World;
```

### Step 3: Add accessor in `ecs.h`

```c
static inline CPoison* World_GetPoison(World* w, EntityId e) {
    ECS_ASSERT_COMPONENT(w, e, POISON);
    return &w->poisons[e];
}
```

### Step 4: Zero in `ecs.c`

In `ZeroEntityComponents()`:
```c
memset(&w->poisons[e], 0, sizeof(CPoison));
```

### Step 5: Use in systems

```c
// In combat_system.c, when applying poison:
World_AddComponent(&gw->ecs, target, COMP_POISON);
CPoison* p = World_GetPoison(&gw->ecs, target);
p->damagePerTurn = 5;
p->remainingTurns = 3;

// In ai_system.c or game.c, at start of turn:
if (World_HasComponents(&gw->ecs, e, COMP_POISON)) {
    CPoison* p = World_GetPoison(&gw->ecs, e);
    CStats* s = World_GetStats(&gw->ecs, e);
    s->hp -= p->damagePerTurn;
    p->remainingTurns--;
    if (p->remainingTurns <= 0) {
        World_RemoveComponent(&gw->ecs, e, COMP_POISON);
    }
}
```

---

## How to Add a New Monster

### Files to modify:

1. **`game_types.h`** — Add to `MonsterType` enum (before `MONSTER_TYPE_COUNT`)
2. **`monster_data.c`** — Add `MonsterTemplate` entry in `Monster_InitTemplates()`:
   ```c
   templates[MONSTER_MY_NEW_MONSTER] = (MonsterTemplate){
       .type = MONSTER_MY_NEW_MONSTER,
       .tmxTypeName = "my_monster",
       .name = "My New Monster",
       .hp = 30, .attack = 8, .defense = 3,
       .expValue = 25, .level = 2,
       .str = 4, .dex = 3, .intel = 2, .con = 4, .lck = 1,
       .color = (Color){ 128, 0, 128, 255 },
       .spritePath = "resources/sprites/monsters/my_monster.png",
       .frameCount = 4, .animSpeed = 0.3f,
       .detectionRange = 6,
       .minFloor = 2, .maxFloor = -1, .maxLevel = -1,
       .spawnWeight = 10,
       .attackType = ATTACK_MELEE, .attackRange = 1,
       .weaponPool = { EQUIP_IRON_SWORD }, .weaponPoolCount = 1,
       .armorPool = {0}, .armorPoolCount = 0,
       .equipDropChance = 15,
       .projectileVisual = PROJ_ARROW,
   };
   ```
3. **`biome_data.c`** — Add to biome monster pool
4. **`spawner_system.c`** — Add meme name variants (optional)
5. **`resources/sprites/monsters/`** — Add sprite sheet PNG

---

## How to Add a New Equipment Item

### Files to modify:

1. **`game_types.h`** — Add to `EquipType` enum (before `EQUIP_COUNT`)
2. **`inventory.c`** — Add entry to `EQUIP_TABLE[]`:
   ```c
   { EQUIP_MY_SWORD, "My Sword", "A legendary blade.\n+15 ATK, +3 STR",
     "resources/sprites/items/equipment/weapons/my_sword.png",
     EQUIP_CAT_WEAPON, EQUIP_SLOT_WEAPON,
     15, 0, 0, 3, 0, 0, 0, 0,
     false, false, RARITY_LEGENDARY },
   ```
3. **`loot_data.c`** — Add to `LOOT_TABLE[]` if droppable
4. **`monster_data.c`** — Add to weapon/armor pools if monsters can equip it
5. **`resources/sprites/items/equipment/`** — Add sprite PNG
6. **`inventory.c`** — Update `Inventory_GetWeaponAbility()` if it's a weapon

---

## How to Add a New Potion/Item

### Files to modify:

1. **`game_types.h`** — Add to `ItemType` enum (before `ITEM_COUNT`)
2. **`inventory.c`** — Add to all 4 static arrays:
   - `ITEM_NAMES[]` — display name
   - `ITEM_HEALS[]` — heal percentage (or 0 for non-healing)
   - `ITEM_DESCS[]` — description text
   - `ITEM_SPRITES[]` — sprite path
3. **`inventory.c`** — Add handling in `InventoryUse()` for special effects
4. **`loot_data.c`** — Add to loot table
5. **`resources/sprites/items/`** — Add sprite PNG

---

## How to Add a New Ability

### Files to modify:

1. **`ability_data.h`** — Add to `AbilityType` enum (before `ABILITY_COUNT`)
2. **`ability_data.c`** — Add `AbilityData` entry:
   ```c
   [ABILITY_FIREBALL] = {
       ABILITY_FIREBALL, "Fireball", "Deals magic fire damage in an area",
       15, 3  // mpCost, cooldownMax
   },
   ```
3. **`ability_system.c`** — Implement in `AbilitySystem_Use()`:
   ```c
   case ABILITY_FIREBALL:
       // Calculate damage, apply to target area
       break;
   ```
4. **`ability_system.c`** — Implement cooldown ticking in `AbilitySystem_TickCooldowns()`
5. **`inventory.c`** — Map weapon to ability in `Inventory_GetWeaponAbility()` if applicable

---

## How to Add a New Biome

### Files to modify:

1. **`biome_data.h`** — Add to `BiomeType` enum (before `BIOME_COUNT`)
2. **`biome_data.c`** — Add `BiomeDef` entry:
   ```c
   [BIOME_UNDEAD_CRYPT] = {
       BIOME_UNDEAD_CRYPT, "Undead Crypt",
       4, 8,   // minFloor, maxFloor
       20,     // spawnWeight
       { MONSTER_SKELETON, MONSTER_WARP_SKULL, MONSTER_SHADOW },
       3,      // monsterCount
       "resources/tilesets/undead_crypt.png"  // tilesetPath
   },
   ```
3. **`procedural.c`** — Add biome-specific tile selection logic if needed
4. **Create tileset** — Add biome tileset PNG to `resources/tilesets/`

---

## How to Add a New UI Screen

### Files to create:

1. **`game/src/ui/my_screen.h`**:
   ```c
   #ifndef MY_SCREEN_H
   #define MY_SCREEN_H
   #include "world.h"
   void MyScreen_Update(GameWorld* gw);
   void MyScreen_Render(const GameWorld* gw);
   #endif
   ```

2. **`game/src/ui/my_screen.c`**:
   ```c
   #include "my_screen.h"
   void MyScreen_Update(GameWorld* gw) { /* handle input */ }
   void MyScreen_Render(const GameWorld* gw) { /* draw UI */ }
   ```

3. **Integrate in `main.c`** or `renderer.c`:
   - For overlay screens: call `MyScreen_Render()` in `RenderGame()` after `EndMode2D()`
   - For full screens: add a new `Scene` enum value in `main.c`

4. **Use `GetUIScale()`** for responsive sizing:
   ```c
   float scale = GetUIScale();
   int panelW = (int)(300 * scale);
   ```

---

## How to Add a New Game State

### Files to modify:

1. **`game_types.h`** — Add to `GameState` enum:
   ```c
   typedef enum {
       STATE_PLAYER_TURN, STATE_ENEMY_TURN, STATE_GAME_OVER,
       STATE_WIN, STATE_INVENTORY, STATE_MAP, STATE_SHOP,
       STATE_CRAFTING    // ← NEW
   } GameState;
   ```

2. **`input_system.c`** — Add input handling:
   ```c
   if (game->state == STATE_CRAFTING) {
       // Handle crafting UI input
       return;
   }
   ```

3. **`game.c`** — Add to `UpdateGame()`:
   ```c
   if (game->state == STATE_CRAFTING) return; // No enemy turns during crafting
   ```

4. **`renderer.c`** — Add rendering:
   ```c
   if (game->state == STATE_CRAFTING)
       CraftingUI_Render(game, scale);
   ```

5. **Trigger the state** — Add a key binding or interaction that sets `game->state = STATE_CRAFTING`

---

## How to Add a New Sound Effect

### Steps:

1. **Add audio files** to `resources/audio/sounds/my_sound/` (multiple files for randomization)
2. **Register in `game_audio.c`**:
   ```c
   static AudioSoundCategory s_sfxMySound = AUDIO_INVALID;

   void GameAudio_Init(void) {
       // ... existing registrations ...
       s_sfxMySound = Audio_RegisterSoundCategory("mysound", "resources/audio/sounds/my_sound");
   }

   void GameAudio_PlayMySound(void) { Audio_PlaySound(s_sfxMySound); }
   ```
3. **Declare in `game_audio.h`**:
   ```c
   void GameAudio_PlayMySound(void);
   ```
4. **Call from game code**:
   ```c
   GameAudio_PlayMySound();
   ```

---

## How to Modify Combat Formulas

### All formulas live in `game/include/game_balance.h`

1. **Edit the constant**:
   ```c
   #define STR_MELEE_MULT  3   // was 2
   ```

2. **Or edit the inline function**:
   ```c
   static inline int calc_melee_damage(int attack, int str) {
       return attack + str * STR_MELEE_MULT + 5;  // added flat bonus
   }
   ```

3. **Update tests** in `test_runner.c`:
   ```c
   CHECK_EQ(calc_melee_damage(5, 3), 14, "melee: atk=5, str=3");  // was 11
   ```

4. **Build and run tests** — all 28 must pass

5. **Play-test** to verify the change feels right

---

## How to Add a New Status Effect

### Pattern: Component + System

1. **Add component** in `components.h` (see [How to Add a New Component](#how-to-add-a-new-component))
2. **Apply the effect** in `combat_system.c` or `ai_system.c`:
   ```c
   World_AddComponent(&gw->ecs, target, COMP_STATUS_EFFECT);
   CStatusEffect* se = World_GetStatusEffect(&gw->ecs, target);
   se->type = STATUS_BURN;
   se->damagePerTurn = 3;
   se->remainingTurns = 5;
   ```
3. **Process the effect** at the start of each turn in `game.c` or a dedicated system:
   ```c
   for (EntityId e = 1; e < gw->ecs.count; e++) {
       if (!World_HasComponents(&gw->ecs, e, COMP_STATUS_EFFECT)) continue;
       // Apply damage, decrement turns, remove when expired
   }
   ```
4. **Render the effect** in `renderer.c` (icon overlay, particle effect, etc.)
5. **Show in inspector** — add to `inspector.c` for monster info panel

---

## How to Add a New Map Feature

### Example: Traps

1. **Add tile GID constant** in `procedural.h`:
   ```c
   #define TILE_TRAP  200
   ```

2. **Place traps** in `procedural.c` during map generation:
   ```c
   // After carving rooms, randomly place trap tiles
   ```

3. **Add to blocking map** (or not — traps should be walkable):
   - Traps are walkable but trigger on step

4. **Handle trap trigger** in `movement_system.c`:
   ```c
   // After player moves to new tile:
   int gid = GetTileGID(gw->map, 0, ppos->x, ppos->y);
   if (gid == TILE_TRAP) {
       int damage = GetRandomValue(5, 15);
       CStats* ps = World_GetStats(w, pe);
       ps->hp -= damage;
       DamageNumber_Spawn(&gw->damageNumbers, damage, ppos->x, ppos->y, tw, th, ORANGE);
       FloatMsg_Spawn(gw, ppos->x, ppos->y, RED, "Trap!");
   }
   ```

5. **Render trap** — add to tileset or draw overlay in `render_system.c`
