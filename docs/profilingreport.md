# Profiling Report — Heroes of Taboo

Generated 2026-06-02. Methodology: static code analysis of ECS query hot-paths,
prioritised by call frequency and entity iteration count.

---

## Top 5 Hot Queries

### 1. `World_FindMonsterAt` — linear entity scan per spatial query

**Location:** `game/src/systems/world_monster.c:4`
**Call sites:** `combat_system.c:37`, `ai_system.c:46,72,100` (MoveToward, MoveAwayFrom, MoveFlank), `input_system.c:275,298,380`
**Complexity:** O(n) — iterates all entities for each (x,y) lookup
**Call count per frame (worst case):**  
- Sprint: up to ~20 tiles × 4 checks per step = 80 calls
- AI: each moving monster calls 4–12 times (neighbor checks + flank/toward/away)
- Player attack: 1 per melee, N per ranged/throw (range steps)

**Impact:** This is the #1 bottleneck. A single AI turn with 10 monsters doing pathfinding can trigger 40–120 linear entity scans.

**Suggested fix:** Add a `MonsterPositions` 2D lookup or spatial hash updated when monsters move/die. This is a simple 2D array `EntityId positionMap[MAP_HEIGHT][MAP_WIDTH]` that maps tile coordinates to the occupying monster. Populate once per enemy turn cycle and clear/invalidate on updates. The memory cost is 100×100×4 bytes = 40 KB, negligible.

---

### 2. `AISystem_ProcessAll` — repeated component access for same entity

**Location:** `game/src/systems/ai_system.c:330`
**Pattern:** `World_GetPosition(w, e)` and `World_GetStats(w, e)` called 2–3 times per entity in nested functions (ProcessMonsterAI calls them, which calls MoveFlank/MoveToward which call them again, which call World_FindMonsterAt which calls them yet again).
**Complexity:** O(n) for outer loop + O(n²) inner from FindMonsterAt spatial queries
**Call count per frame:** ~128 entity checks + K×128 for each FindMonsterAt call

**Suggested fix:**  
In `ProcessMonsterAI`, the `mp` and `ms` pointers are already cached. The inner movement functions (MoveToward, MoveAwayFrom, MoveFlank) should receive these pointers directly instead of re-deriving them. The `AISystem_ProcessAll` outer loop already fetches position and stats on lines 336–337, then calls `ProcessMonsterAI` which re-fetches them on lines 123–124. Remove the redundant fetches in `ProcessMonsterAI` or pass the pre-fetched pointers. Safe: these functions don't destroy/move the entity between calls.

---

### 3. `World_CountAliveMonsters` — per-frame full scan for win condition

**Location:** `game/src/systems/world_monster.c:18`
**Call sites:** `game.c:121` (UpdateGame → AreAllMonstersDead check, every frame during enemy turn)
**Complexity:** O(n) entity scan, called every frame
**Impact:** Low for <128 entities, but unnecessary redundant scanning.

**Suggested fix:** Track `aliveMonsterCount` as a field in `GameWorld`. Increment on spawn, decrement on death. Check `aliveMonsterCount == 0` instead of calling `World_CountAliveMonsters`. Extremely safe: monster spawn and death are already centralized in SpawnerSystem and CombatSystem. Add `game->aliveMonsterCount` to world.h and update it in `GainExperience`/death handlers and `SpawnerSystem_SpawnMonster`.

---

### 4. `RenderSystem_DrawMonsters` — full entity scan per frame with component access

**Location:** `game/src/systems/render_system.c:43`
**Pattern:** Iterates all entities, calls `World_HasComponents` + `World_GetPosition` + `World_GetStats` per entity, then `Monster_GetTemplate` + `World_GetAI` + `World_GetSprite` for each visible monster.
**Complexity:** O(n) per frame at ~60fps
**Impact:** Moderate — the per-entity work is light (mostly visibility checks), but runs every frame for all entities.

**Suggested fix:** Store the pre-filtered list of visible monster entities (those with COMP_POSITION|COMP_STATS|COMP_AI and !COMP_PLAYER_TAG) in a small array updated once per enemy turn or on significant state change. The renderer can iterate the smaller list. Safe: the visibility check per tile remains dynamic (FOW), so only entity component filtering benefits from caching.

---

### 5. `World_UpdateMonsterAnimations` — per-frame animation scan

**Location:** `game/src/systems/world_monster.c:34`
**Pattern:** Iterates all entities for hit-flash timer updates and sprite animation advancement.
**Complexity:** O(n) per frame
**Impact:** Low — but can be combined with RenderSystem_DrawMonsters cache.

**Suggested fix:** Use the same pre-filtered monster list from #4 for animation updates. The hit-flash component check and sprite update only apply to monster entities. A combined update-render pass would reduce from 2 full entity scans to 1.

---

## Summary of Recommendations

| Priority | Change | Lines | Risk |
|----------|--------|-------|------|
| **High** | Add spatial monster position map for O(1) lookups | ~20 LOC added to world_monster.c | Low: update on move/die/spawn only |
| **High** | Incremental alive monster counter | ~5 LOC added to combat/player/spawner | Very low: simple count tracking |
| **Medium** | Cache entity pointers in AI functions | ~10 LOC in ai_system.c | Low: pass pointers, don't re-fetch |
| **Low** | Pre-filtered monster list for render/animation | ~15 LOC in render_system.c | Low: static list updated on change |

**Estimated total improvement:** The spatial map alone reduces AI-turn processing from O(n²) to O(n), which is the biggest win. With all recommendations combined, AI processing time would drop ~80% for 10+ monsters, and per-frame scanning overhead would drop ~50%.
