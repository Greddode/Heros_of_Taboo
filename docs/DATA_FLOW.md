# Data Flow & Control Flow

**Heroes of Taboo** — v0.0.10 | June 5, 2026

---

## Table of Contents

1. [Application Lifecycle](#application-lifecycle)
2. [Scene Control Flow](#scene-control-flow)
3. [Game Loop (Per Frame)](#game-loop-per-frame)
4. [Turn-Based Control Flow](#turn-based-control-flow)
5. [Player Turn Data Flow](#player-turn-data-flow)
6. [Enemy Turn Data Flow](#enemy-turn-data-flow)
7. [Floor Transition Data Flow](#floor-transition-data-flow)
8. [Combat Data Flow](#combat-data-flow)
9. [Inventory/Equipment Data Flow](#inventoryequipment-data-flow)
10. [Rendering Data Flow](#rendering-data-flow)
11. [Spatial Hash Data Flow](#spatial-hash-data-flow)
12. [FOW Data Flow](#fow-data-flow)

---

## Application Lifecycle

```
main()
  │
  ├─ InitWindow(1024, 768, "Heroes of Taboo")
  ├─ SetWindowState(FLAG_WINDOW_RESIZABLE)
  ├─ SetTargetFPS(60)
  ├─ SetExitKey(KEY_NULL)
  ├─ SetRandomSeed(time(NULL))
  ├─ InitAudioSystem()           ← engine/audio.c
  ├─ GameAudio_Init()            ← game_audio.c (register contexts + categories)
  ├─ InventoryUI_Init(&invUI)    ← initialize inventory UI state
  │
  ├─ while (!WindowShouldClose() && scene != SCENE_EXIT)
  │    │
  │    ├─ GameAudio_SetContext(scene == SCENE_GAME)
  │    ├─ UpdateMusicSystem()
  │    │
  │    ├─ switch (scene)
  │    │    ├─ SCENE_MENU      → Menu_Update() + Menu_Render()
  │    │    ├─ SCENE_SETTINGS  → Menu_SettingsRender() + Menu_SettingsUpdate()
  │    │    ├─ SCENE_CONTROLS  → Menu_ControlsRender() + Menu_ControlsUpdate()
  │    │    ├─ SCENE_STORY     → Menu_StoryRender() + Menu_StoryUpdate()
  │    │    ├─ SCENE_CREDITS   → Menu_CreditsRender() + Menu_CreditsUpdate()
  │    │    └─ SCENE_GAME      → [full game loop, see below]
  │    │
  │    └─ (loop)
  │
  ├─ CleanupGame(&game)
  ├─ ShutdownAudioSystem()
  └─ CloseWindow()
```

---

## Scene Control Flow

```
                    ┌──────────┐
                    │SCENE_MENU│◄──────────────────────────────┐
                    └────┬─────┘                               │
                         │                                     │
              ┌──────────┼──────────┬──────────┐               │
              ▼          ▼          ▼          ▼               │
        ┌──────────┐ ┌────────┐ ┌───────┐ ┌────────┐          │
        │ SETTINGS │ │CONTROLS│ │ STORY │ │CREDITS │          │
        └────┬─────┘ └───┬────┘ └──┬────┘ └───┬────┘          │
             │           │         │          │               │
             └───────────┴─────────┴──────────┘               │
                         │ (back)                              │
                    ┌────▼─────┐                               │
                    │SCENE_MENU│                               │
                    └────┬─────┘                               │
                   PLAY  │                                     │
                    ┌────▼─────┐                               │
                    │SCENE_GAME│──── GAME_OVER/ESC ────────────┘
                    └────┬─────┘
                   EXIT  │
                    ┌────▼─────┐
                    │SCENE_EXIT│
                    └──────────┘
```

---

## Game Loop (Per Frame)

When `scene == SCENE_GAME`, each frame executes:

```
1. ESC handling
   ├─ If map open → MapUI_Update()
   ├─ If ESC pressed → toggle game menu / settings / close inventory
   └─ If Shift+R → restart game

2. Input processing (if no menu open)
   ├─ If STATE_SHOP → ShopUI_HandleInput()
   ├─ Else:
   │    ├─ InputSystem_Inventory(game, &invUI)
   │    ├─ InputSystem_PlayerTurn(game, &invUI)
   │    └─ UpdateGame(game)
   └─ (skip if restart requested)

3. Camera interpolation
   ├─ Calculate interpolation factor t from animTimer/animDuration
   ├─ Lerp camera.target between prevPos and currentPos
   └─ Set camera.offset to screen center

4. Rendering
   ├─ BeginDrawing() + ClearBackground(BLACK)
   ├─ RenderGame(game, &invUI)     ← master render
   ├─ If STATE_MAP → MapUI_Render()
   ├─ If settings open → Menu_SettingsRenderGame()
   ├─ If game menu open → GameMenu_Render()
   └─ EndDrawing()

5. Restart handling (if Shift+R was pressed)
   ├─ CleanupGame()
   ├─ InitGame() with map file
   └─ Reset menu flags
```

---

## Turn-Based Control Flow

```
┌─────────────────┐
│ STATE_PLAYER_TURN│◄──────────────────────────────────────┐
└────────┬────────┘                                       │
         │ Player acts (move, attack, wait, use item)      │
         │                                                 │
         ├─ Movement → STATE_ENEMY_TURN ──────────────────┐│
         ├─ Attack   → STATE_ENEMY_TURN ──────────────────┤│
         ├─ Wait     → STATE_ENEMY_TURN ──────────────────┤│
         ├─ Use Item → (stays PLAYER_TURN, no enemy turn) ││
         ├─ Open Inv → STATE_INVENTORY                    ││
         ├─ Open Map → STATE_MAP                          ││
         └─ Interact → STATE_SHOP (if shopkeeper)         ││
                                                              ││
┌─────────────────┐                                       ││
│STATE_ENEMY_TURN │                                       ││
└────────┬────────┘                                       ││
         │                                                ││
         ├─ enemyTurnCooldown > 0 → wait (return)         ││
         ├─ animatingEnemyTurn → wait for animation       ││
         │                                                ││
         ├─ AISystem_ProcessAll()                         ││
         │    ├─ Player dies → STATE_GAME_OVER            ││
         │    └─ Player alive → continue                  ││
         │                                                ││
         ├─ All monsters dead?                            ││
         │    ├─ Floor >= maxFloors → spawn escape tile   ││
         │    └─ Floor < maxFloors → "Floor Clear!" msg   ││
         │                                                ││
         ├─ Set monsterAnimTimer = MOVE_ANIM_DURATION     ││
         ├─ animatingEnemyTurn = true                     ││
         │                                                ││
         └─ Animation finishes → STATE_PLAYER_TURN ───────┘│
                                                            │
┌──────────────────┐                                       │
│STATE_INVENTORY   │──── I or ESC ─────────────────────────┘
└──────────────────┘
┌──────────────────┐
│STATE_MAP         │──── M or Z ──→ STATE_PLAYER_TURN
└──────────────────┘
┌──────────────────┐
│STATE_SHOP        │──── ESC ──→ STATE_PLAYER_TURN
└──────────────────┘
┌──────────────────┐
│STATE_GAME_OVER   │──── Shift+R ──→ restart
└──────────────────┘
┌──────────────────┐
│STATE_WIN         │──── Shift+R ──→ restart
└──────────────────┘
```

---

## Player Turn Data Flow

When the player presses a movement key:

```
InputSystem_PlayerTurn()
  │
  ├─ Read WASD/arrow keys → Direction dir
  │
  ├─ MovementSystem_PlayerMove(gw, dir)
  │    │
  │    ├─ Calculate new tile (nx, ny) from direction
  │    ├─ Check walkability:
  │    │    ├─ In bounds? (0 <= nx < map.width, 0 <= ny < map.height)
  │    │    ├─ Not blocked? (blocking[ny][nx] == 0)
  │    │    └─ No monster? (World_FindMonsterAt(nx, ny) == ENTITY_NONE)
  │    │
  │    ├─ If walkable:
  │    │    ├─ Save prevX/prevY, update x/y
  │    │    ├─ Set facingDir = dir
  │    │    ├─ Increment turnCount
  │    │    ├─ Set enemyTurnCooldown = 0.15f
  │    │    ├─ Set animTimer = MOVE_ANIM_DURATION
  │    │    ├─ RevealFOW(gw)
  │    │    ├─ state = STATE_ENEMY_TURN
  │    │    │
  │    │    ├─ Collect potions at new tile:
  │    │    │    ├─ SpawnerSystem_ListPotionsAt() → check if room in inventory
  │    │    │    ├─ If room: SpawnerSystem_CollectPickupsAt() → InventoryAdd()
  │    │    │    └─ Play pickup sound, spawn FloatMsg
  │    │    │
  │    │    ├─ Collect equipment at new tile:
  │    │    │    ├─ SpawnerSystem_CollectEquipAt() → AddEquipToInventory()
  │    │    │    └─ If bag full: FloatMsg "Inventory full!"
  │    │    │
  │    │    ├─ Check stairs tile:
  │    │    │    └─ If (x,y) == (stairX,stairY) → DescendFloor()
  │    │    │
  │    │    └─ Check escape tile:
  │    │         └─ If (x,y) == (escapeX,escapeY) → state = STATE_WIN
  │    │
  │    └─ If not walkable but monster at (nx,ny):
  │         └─ CombatSystem_PlayerMeleeAttack(attackerId, nx, ny)
  │              → state = STATE_ENEMY_TURN
  │
  ├─ F key → CombatSystem_PlayerMeleeAttack(facing tile)
  ├─ T key → CombatSystem_PlayerThrowWeapon()
  ├─ Space → wait turn (heal, shadow check)
  ├─ I key → state = STATE_INVENTORY
  ├─ M key → state = STATE_MAP
  └─ E key → interact (inspect tile, enter shop)
```

---

## Enemy Turn Data Flow

```
UpdateGame() → state == STATE_ENEMY_TURN
  │
  ├─ Wait for enemyTurnCooldown (0.15s)
  │
  ├─ If animatingEnemyTurn:
  │    ├─ Wait for projectile animation to finish
  │    ├─ Wait for monster animation to finish
  │    └─ Both done → animatingEnemyTurn = false, state = STATE_PLAYER_TURN
  │
  ├─ AISystem_ProcessAll(gw, timeWaited)
  │    │
  │    ├─ Pre-fetch all monster pointers (batch optimization)
  │    │
  │    ├─ For each alive monster with COMP_AI:
  │    │    │
  │    │    ├─ Calculate distance to player (Manhattan)
  │    │    ├─ Check line of sight (Bresenham ray through blocking map)
  │    │    │
  │    │    ├─ If in attack range + LOS:
  │    │    │    ├─ Melee: move adjacent + attack
  │    │    │    ├─ Ranged: fire projectile at player
  │    │    │    └─ Magic: fire magic projectile
  │    │    │
  │    │    ├─ If in detection range + LOS:
  │    │    │    ├─ Record lastSeenX/Y
  │    │    │    ├─ Move toward player (greedy pathfinding)
  │    │    │    └─ Update spatial hash (SpatialHash_Move)
  │    │    │
  │    │    ├─ If hunt turns > 0:
  │    │    │    ├─ Move toward lastSeenX/Y
  │    │    │    └─ Decrement huntTurns
  │    │    │
  │    │    ├─ Else: wander randomly
  │    │    │
  │    │    └─ Shadow special: double-move after threshold
  │    │
  │    └─ Returns false if player HP <= 0
  │
  ├─ If !playerAlive:
  │    ├─ Set player stats.alive = false, hp = 0
  │    └─ state = STATE_GAME_OVER
  │
  ├─ Check floor clear:
  │    ├─ If monstersEverSpawned && AreAllMonstersDead():
  │    │    ├─ Floor >= maxFloors → SpawnEscapeTile(), "Escape!" msg
  │    │    └─ Floor < maxFloors → "Floor Clear!" msg
  │
  ├─ Set monsterAnimTimer = MOVE_ANIM_DURATION
  ├─ animatingEnemyTurn = true
  │
  └─ If no projectile active:
       └─ animatingEnemyTurn = false, state = STATE_PLAYER_TURN
```

---

## Floor Transition Data Flow

```
DescendFloor(game)
  │
  ├─ 1. Show "LOADING..." screen
  │
  ├─ 2. Save player state:
  │    ├─ SavePlayerEcs() → CPosition, CStats, CSpriteAnim, CHitFlash
  │    ├─ memcpy inventory[16], equipped[5], equipInventory[16]
  │    └─ Save inspected tile state
  │
  ├─ 3. Save GameWorld copy, unload current map (UnloadTMX)
  │
  ├─ 4. memset(game, 0, sizeof(GameWorld))  ← ZERO EVERYTHING
  │
  ├─ 5. Restore inspected tile state
  │
  ├─ 6. GameWorld_Init(game)
  │    ├─ memset(gw, 0, sizeof(GameWorld))
  │    ├─ World_Init(&gw->ecs)  ← count=1, clear masks/alive
  │    ├─ playerEntity = ENTITY_NONE
  │    └─ selectedMonsterEntity = ENTITY_NONE
  │    (monsterGrid is already zeroed → all ENTITY_NONE)
  │    (aliveMonsterCount is already 0)
  │
  ├─ 7. Set currentFloor = savedFloor + 1, maxFloors = savedMaxFloors
  │
  ├─ 8. Reload player sprite texture from resource cache
  │
  ├─ 9. LoadPotionTextures()
  │
  ├─ 10. PlayerSystem_Spawn(game) → create new player entity
  │
  ├─ 11. RestorePlayerEcs() → copy saved position/stats/sprite
  │
  ├─ 12. Restore inventory, equipment
  │
  ├─ 13. Re-apply equipment bonuses:
  │    └─ For each equipped slot: EquipmentBonus_Apply()
  │    └─ Recalculate maxHp, preserve HP ratio
  │
  ├─ 14. Generate new map:
  │    └─ GenerateProceduralMap(80, 50, hasStairs)
  │
  ├─ 15. LoadTilesets()
  │
  ├─ 16. Floor_InitNewFloor(game)
  │    ├─ BuildBlockingMap()
  │    ├─ Monster_InitTemplates()
  │    ├─ Biome_SelectForFloor() + ProceduralMap_SetBiome()
  │    ├─ SpawnEntitiesFromObjects()
  │    ├─ SpawnMonstersForFloor()  ← CR-budget spawning
  │    ├─ SpawnerSystem_SpawnPickups()
  │    ├─ 3% chance: SpawnShopRoom()
  │    ├─ Set stairX/Y from GetStairX/Y()
  │    ├─ Reset turn state, animation timers
  │    ├─ Clear visibility array
  │    ├─ RevealFOW()
  │    └─ Position camera on player
  │
  └─ 17. Spawn FloatMsg "Floor N"
```

---

## Combat Data Flow

### Melee Attack

```
CombatSystem_PlayerMeleeAttack(game, attackerId, targetX, targetY)
  │
  ├─ World_FindMonsterAt(game, targetX, targetY, ENTITY_NONE)
  │    └─ O(1) lookup via monsterGrid[y][x]
  │
  ├─ Get attacker CStats (as) and monster CStats (ms)
  │
  ├─ Calculate raw damage:
  │    └─ raw = calc_melee_damage(as->attack, as->str)
  │
  ├─ Check dodge:
  │    ├─ dodgePct = calc_dodge_chance(ms->dex)
  │    ├─ Roll: GetRandomValue(1, 100) <= dodgePct
  │    └─ If dodged: spawn "DODGE" FloatMsg, return true
  │
  ├─ Apply defense:
  │    └─ damage = calc_damage_after_defense(raw, ms->defense)
  │
  ├─ Check critical hit:
  │    ├─ critChance = as->lck (percentage)
  │    ├─ Roll: GetRandomValue(1, 100) <= critChance
  │    ├─ If crit: damage *= CRIT_MULT (2×)
  │    │
  │    └─ Check mega-crit:
  │         ├─ If damage >= MEGA_CRIT_THRESHOLD (100)
  │         ├─ Roll: GetRandomValue(1, 100) <= MEGA_CRIT_CHANCE (50%)
  │         └─ If mega-crit: damage *= 2
  │
  ├─ Apply damage: ms->hp -= damage
  │
  ├─ Spawn DamageNumber (red for normal, yellow for crit)
  ├─ Spawn hit flash (CHitFlash.timer = HIT_FLASH_DURATION)
  ├─ Play hit sound
  │
  ├─ Dual-wield off-hand follow-up:
  │    ├─ If IsDualWielding(game):
  │    │    ├─ Calculate off-hand damage (× DUAL_WIELD_OFFHAND_MULT)
  │    │    ├─ Apply to monster
  │    │    └─ Spawn additional DamageNumber
  │
  ├─ If monster HP <= 0:
  │    ├─ ms->alive = false
  │    ├─ SpatialHash_Remove() from grid
  │    ├─ aliveMonsterCount--
  │    ├─ GainExperience(game, ms->expValue)
  │    ├─ Add gold (base + rarity bonus)
  │    ├─ MP bonus on kill
  │    ├─ DropMonsterEquipment() (random chance from template)
  │    └─ Spawn death FloatMsg
  │
  └─ Return true (monster was targeted)
```

### Ranged Attack

```
CombatSystem_PlayerRangedAttack(game, attackerId)
  │
  ├─ Get equipped weapon range bonus (GetEquipRangeBonus)
  ├─ Trace projectile path in facing direction
  │    └─ Bresenham line through blocking map
  │    ├─ Stop at wall or max range
  │    └─ Stop at first monster hit
  │
  ├─ If monster hit:
  │    ├─ damage = calc_ranged_damage(as->dex)
  │    ├─ Apply defense, check dodge, check crit
  │    └─ Apply damage, handle death
  │
  ├─ Set projectile state (active, start/end positions, visual)
  ├─ projectileTimer = PROJECTILE_ANIM_DURATION
  ├─ Play ranged attack sound
  │
  └─ Return true (turn consumed)
```

### Throw Weapon

```
CombatSystem_PlayerThrowWeapon(game, attackerId)
  │
  ├─ Get equipped weapon type
  ├─ Unequip weapon (remove from equipped slot)
  ├─ Trace path (same as ranged)
  │
  ├─ If monster hit:
  │    ├─ damage = calc_throw_damage(as->attack, as->dex)
  │    ├─ Apply defense, check dodge, check crit
  │    └─ Apply damage, handle death
  │
  ├─ Set projectile state (throwTex = weapon texture, rotating)
  ├─ projectileTimer = PROJECTILE_ANIM_DURATION
  │
  └─ Return true (turn consumed, weapon lost)
```

---

## Inventory/Equipment Data Flow

### Picking Up Items

```
MovementSystem_PlayerMove() → player steps on tile with pickups
  │
  ├─ SpawnerSystem_ListPotionsAt(x, y, ...)
  │    └─ Scan ECS for entities with COMP_POSITION | COMP_PICKUP at (x,y)
  │    └─ Return list of ItemType + quantity
  │
  ├─ For each potion:
  │    ├─ Check InventoryAdd() would succeed (room in bag)
  │    ├─ If room: SpawnerSystem_CollectPickupsAt() → InventoryAdd()
  │    │    ├─ CollectPickupsAt: destroy pickup entities, decrement quantities
  │    │    └─ InventoryAdd: stack or add new slot
  │    ├─ Play pickup sound
  │    └─ Spawn FloatMsg "+Item Name"
  │
  └─ For equipment:
       ├─ SpawnerSystem_CollectEquipAt(x, y, ...)
       ├─ AddEquipToInventory() for each
       └─ If bag full: FloatMsg "Inventory full!"
```

### Equipping Items

```
EquipItem(game, type)
  │
  ├─ Get EquipData for type
  ├─ Determine target slot (data->slot)
  │
  ├─ Dual-wield redirect:
  │    └─ If weapon + dual-wieldable + main occupied + off-hand free
  │         → redirect to EQUIP_SLOT_OFF_HAND
  │
  ├─ Two-handed conflict:
  │    └─ If off-hand occupied + new item is two-handed
  │         → UnequipSlot(OFF_HAND) first
  │
  ├─ If slot occupied:
  │    └─ UnequipSlot(slot) first
  │
  ├─ EquipmentBonus_Apply(ecs, player, type)
  │    ├─ Add bonusAttack, bonusDefense, bonusStr, etc. to CStats
  │    └─ EquipmentBonus_Recalculate() → update maxHp from CON
  │
  ├─ Set equipped[slotIdx] = type
  │
  ├─ Special: Band of Growth → statCapsRemoved = true
  │
  └─ Update weapon ability in CAbilities
```

### Unequipping Items

```
UnequipSlot(game, slot)
  │
  ├─ Get current EquipType from equipped[slot]
  ├─ EquipmentBonus_Remove(ecs, player, type)
  │    ├─ Subtract bonuses from CStats
  │    └─ EquipmentBonus_Recalculate()
  │
  ├─ Set equipped[slot] = EQUIP_NONE
  │
  ├─ Special: Band of Growth → statCapsRemoved = false
  │
  ├─ If equipInventory has room:
  │    └─ AddEquipToInventory(type)
  │
  └─ Else (bag full):
       ├─ Find adjacent walkable tile (no monster)
       ├─ SpawnerSystem_AddEquipAt(dropX, dropY, type, 1)
       └─ FloatMsg "No room - item dropped!"
```

---

## Rendering Data Flow

```
RenderGame(game, &invUI)
  │
  ├─ BeginMode2D(camera)  ← world-space rendering
  │
  ├─ RenderSystem_DrawMap(game)
  │    └─ For each layer, for each visible tile:
  │         ├─ Get GID from layer data
  │         ├─ Find tileset for GID
  │         ├─ Compute source rectangle
  │         ├─ Draw texture at tile position
  │         └─ If visibility == 2: overlay dark rectangle (explored but not seen)
  │
  ├─ Draw pickups (ECS iteration)
  │    └─ For each entity with COMP_POSITION | COMP_PICKUP:
  │         ├─ If potion: draw potion texture + quantity label
  │         └─ If equipment: draw equip texture or loot pile (if stacked)
  │
  ├─ RenderSystem_DrawMonsters(game, monsterT)
  │    └─ For each alive monster with COMP_POSITION | COMP_STATS:
  │         ├─ Skip if not visible (visibility != 1)
  │         ├─ Interpolate position: prevPos × (1-t) + curPos × t
  │         ├─ If has sprite: draw animated frame
  │         ├─ Else: draw fallback color rectangle
  │         ├─ If hit flash active: white tint overlay
  │         └─ Draw health bar above monster
  │
  ├─ Draw player
  │    ├─ Interpolate position
  │    ├─ Draw sprite (4-frame animation) or fallback color
  │    ├─ If hit flash: white tint
  │    ├─ Draw facing direction indicator (red overlay on adjacent tile)
  │    └─ (only during STATE_PLAYER_TURN)
  │
  ├─ Draw shopkeeper (gold rectangle + "Shop" label)
  │
  ├─ Draw projectile
  │    ├─ Magic: animated wave along Bresenham path
  │    ├─ Throw: rotating weapon texture
  │    └─ Arrow/spore/rock: line + arrowhead or colored circle
  │
  ├─ Draw damage numbers (world-space floating text)
  │    └─ For each active DamageNumber: draw text with alpha fade
  │
  ├─ Draw float messages (world-space floating text)
  │    └─ For each active FloatMsg: draw text with alpha fade
  │
  ├─ EndMode2D()
  │
  └─ Screen-space HUD:
       ├─ HP bar (red) + EXP bar (blue) + level + floor info
       ├─ Gold display (top-right)
       ├─ Inspector panel (monster info, top-right)
       ├─ Tile info panel (potion/equipment at inspected tile)
       ├─ State text ("Your turn" / "Enemy turn..." / "GAME OVER")
       ├─ Combat hints ("[F] Attack" / "[T] Throw Weapon")
       ├─ Level-up overlay (animated "LEVEL UP!" + "+2 Stat Points!")
       ├─ InventoryUI_Render() (if STATE_INVENTORY)
       └─ ShopUI_Render() (if STATE_SHOP)
```

---

## Spatial Hash Data Flow

```
Spatial hash grid: GameWorld.monsterGrid[MAP_HEIGHT][MAP_WIDTH]
Each cell holds an EntityId or ENTITY_NONE.

SPAWN:
  SpawnerSystem_SpawnMonster()
    ├─ World_CreateEntity() → get EntityId
    ├─ Set CPosition (x, y)
    ├─ SpatialHash_Add(gw, e, x, y)
    │    └─ monsterGrid[y][x] = e
    └─ gw->aliveMonsterCount++

MOVE (AI):
  AISystem_ProcessAll() → monster moves from (oldX,oldY) to (newX,newY)
    ├─ Update CPosition (prevX=oldX, prevY=oldY, x=newX, y=newY)
    └─ SpatialHash_Move(gw, e, oldX, oldY, newX, newY)
         ├─ monsterGrid[oldY][oldX] = ENTITY_NONE (if was e)
         └─ monsterGrid[newY][newX] = e

DEATH:
  CombatSystem → monster HP <= 0
    ├─ CStats.alive = false
    ├─ SpatialHash_Remove(gw, e, x, y)
    │    └─ if monsterGrid[y][x] == e → monsterGrid[y][x] = ENTITY_NONE
    └─ gw->aliveMonsterCount--

LOOKUP:
  World_FindMonsterAt(gw, x, y, exclude)
    ├─ e = monsterGrid[y][x]
    ├─ If e == ENTITY_NONE or e == exclude → return ENTITY_NONE
    ├─ Validate: alive, has POSITION+STATS+AI, not PLAYER_TAG, stats.alive
    └─ Return e

FLOOR TRANSITION:
  GameWorld_Init()
    └─ memset(gw, 0, sizeof(GameWorld))
         └─ monsterGrid zeroed → all cells = ENTITY_NONE (0)
         └─ aliveMonsterCount = 0
```

---

## FOW Data Flow

```
RevealFOW(game)
  │
  ├─ Step 1: Demote all "seen" tiles to "explored"
  │    └─ For each tile: if visibility[y][x] == 1 → set to 2
  │
  ├─ Step 2: Cast Bresenham rays from player position
  │    └─ For each tile within FOG_RADIUS (7):
  │         ├─ Skip if outside circle (dx² + dy² > r²)
  │         ├─ Trace ray from player to tile
  │         │    └─ Bresenham line: step through intermediate tiles
  │         │    └─ If any intermediate tile is blocking → ray blocked
  │         ├─ If ray reaches tile: visibility[y][x] = 1 (seen)
  │         └─ If blocked: skip (remains 2 or 0)
  │
  └─ Step 3: Player tile always visible
       └─ visibility[playerY][playerX] = 1

VISIBILITY STATES:
  0 = Hidden (never seen) → not rendered
  1 = Seen (currently visible) → full brightness
  2 = Explored (seen before, not currently) → darkened overlay

BuildBlockingMap(game)
  │
  └─ For each tile:
       ├─ Get GID from layer 0
       ├─ If GID is wall tile → blocking[y][x] = 1
       └─ If GID is floor tile → blocking[y][x] = 0
```
