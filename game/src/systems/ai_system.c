#include "ai_system.h"
#include "debug_log.h"
#include "validation.h"
#include "event_bus.h"
#include "game_audio.h"
#include "data/monster_data.h"
#include "world_monster.h"
#include "spatial_hash.h"
#include "inventory.h"
#include <math.h>

AbilityType AI_GetActiveAbility(GameWorld* gw, EntityId entity)
{
    if (World_HasComponents(&gw->ecs, entity, COMP_ABILITIES)) {
        CAbilities* ab = World_GetAbilities(&gw->ecs, entity);
        if (ab->count > 0) return ab->abilities[0];
    }
    if (World_HasComponents(&gw->ecs, entity, COMP_AI)) {
        CAI* ai = World_GetAI(&gw->ecs, entity);
        if (ai->equippedWeapon != EQUIP_NONE) {
            AbilityType wa = Inventory_GetWeaponAbility(ai->equippedWeapon);
            if (wa != ABILITY_NONE) return wa;
        }
        if (ai->attackType == ATTACK_RANGED || ai->attackType == ATTACK_MAGIC)
            return ABILITY_RANGED;
    }
    return ABILITY_PUNCH;
}

static bool MonsterLineOfSight(int x0, int y0, int x1, int y1,
                                const unsigned char blocking[][MAP_WIDTH], int maxDist) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int steps = dx > dy ? dx : dy;
    if (steps > maxDist) return false;

    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    int x = x0, y = y0;
    for (int i = 0; i < steps; i++) {
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x += sx; }
        if (e2 <  dx) { err += dx; y += sy; }
        if ((x != x1 || y != y1) && blocking[y][x]) return false;
    }
    return true;
}

static Direction MoveToward(GameWorld* gw, EntityId mover, CPosition* mp, int targetX, int targetY) {
    int mapWidth = gw->map->width;
    int mapHeight = gw->map->height;

    Direction bestDir = DIR_NONE;
    int bestDist = abs(targetX - mp->x) + abs(targetY - mp->y);
    Direction dirs[] = { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT };
    for (int d = 0; d < 4; d++) {
        int tx = mp->x, ty = mp->y;
        switch (dirs[d]) {
            case DIR_UP:    ty--; break;
            case DIR_DOWN:  ty++; break;
            case DIR_LEFT:  tx--; break;
            case DIR_RIGHT: tx++; break;
            default: break;
        }
        if (tx >= 0 && tx < mapWidth && ty >= 0 && ty < mapHeight &&
            !gw->blocking[ty][tx] && World_FindMonsterAt(gw, tx, ty, mover) == ENTITY_NONE) {
            int nd = abs(targetX - tx) + abs(targetY - ty);
            if (nd < bestDist) { bestDist = nd; bestDir = dirs[d]; }
        }
    }
    return bestDir;
}

static Direction MoveAwayFrom(GameWorld* gw, EntityId mover, CPosition* mp, int targetX, int targetY) {
    int mapWidth  = gw->map->width;
    int mapHeight = gw->map->height;

    Direction bestDir = DIR_NONE;
    int bestDist = abs(targetX - mp->x) + abs(targetY - mp->y);
    Direction dirs[] = { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT };
    for (int d = 0; d < 4; d++) {
        int tx = mp->x, ty = mp->y;
        switch (dirs[d]) {
            case DIR_UP:    ty--; break;
            case DIR_DOWN:  ty++; break;
            case DIR_LEFT:  tx--; break;
            case DIR_RIGHT: tx++; break;
            default: break;
        }
        if (tx >= 0 && tx < mapWidth && ty >= 0 && ty < mapHeight &&
            !gw->blocking[ty][tx] &&
            World_FindMonsterAt(gw, tx, ty, mover) == ENTITY_NONE) {
            int nd = abs(targetX - tx) + abs(targetY - ty);
            if (nd > bestDist) { bestDist = nd; bestDir = dirs[d]; }
        }
    }
    return bestDir;
}

static Direction MoveFlank(GameWorld* gw, EntityId mover, CPosition* mp, int targetX, int targetY) {
    int mapWidth  = gw->map->width;
    int mapHeight = gw->map->height;

    Direction bestDir = DIR_NONE;
    int bestScore = 9999;
    Direction dirs[] = { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT };
    for (int d = 0; d < 4; d++) {
        int tx = mp->x, ty = mp->y;
        switch (dirs[d]) {
            case DIR_UP:    ty--; break;
            case DIR_DOWN:  ty++; break;
            case DIR_LEFT:  tx--; break;
            case DIR_RIGHT: tx++; break;
            default: break;
        }
        if (tx < 0 || tx >= mapWidth || ty < 0 || ty >= mapHeight) continue;
        if (gw->blocking[ty][tx]) continue;
        EntityId occupant = World_FindMonsterAt(gw, tx, ty, mover);
        if (occupant != ENTITY_NONE && !World_HasComponents(&gw->ecs, occupant, COMP_PLAYER_TAG)) continue;

        int distToTarget = abs(targetX - tx) + abs(targetY - ty);
        int score = distToTarget;
        if (tx == mp->x || ty == mp->y) score += 1;
        if (score < bestScore) { bestScore = score; bestDir = dirs[d]; }
    }
    return bestDir;
}

static void ApplyMove(GameWorld* gw, EntityId e, CPosition* p, Direction dir) {
    int oldX = p->x, oldY = p->y;
    switch (dir) {
        case DIR_UP:    p->y--; break;
        case DIR_DOWN:  p->y++; break;
        case DIR_LEFT:  p->x--; break;
        case DIR_RIGHT: p->x++; break;
        default: return;
    }
    SpatialHash_Move(gw, e, oldX, oldY, p->x, p->y);
}

static void ProcessMonsterAI(GameWorld* gw, EntityId monster,
                              CPosition* mp, CStats* ms, CAI* ai) {
    const MonsterTemplate* tpl = Monster_GetTemplate(ai->type);
    if (!tpl) return;

    AbilityType ability = AI_GetActiveAbility(gw, monster);

    CPosition* playerPos = World_GetPosition(&gw->ecs, gw->playerEntity);
    CStats* playerStats = World_GetStats(&gw->ecs, gw->playerEntity);
    int playerX = playerPos->x;
    int playerY = playerPos->y;
    int dx = playerX - mp->x;
    int dy = playerY - mp->y;
    int dist = abs(dx) + abs(dy);
    int mapWidth = gw->map->width;
    int mapHeight = gw->map->height;
    bool hasLOS = MonsterLineOfSight(mp->x, mp->y, playerX, playerY,
                                      gw->blocking, tpl->detectionRange);

    CName* mname = World_GetName(&gw->ecs, monster);

    if (dist <= tpl->detectionRange && hasLOS) {
        DebugLog(DEBUG_AI, "%s: state=attack dist=%d LOS=yes pos=(%d,%d) ability=%d",
                 mname->name, dist, mp->x, mp->y, (int)ability);
        ai->lastSeenX = playerX;
        ai->lastSeenY = playerY;
        ai->huntTurns = 4;

        if (ability == ABILITY_RANGED && dist <= ai->attackRange &&
            (dx == 0 || dy == 0) &&
            MonsterLineOfSight(mp->x, mp->y, playerX, playerY, gw->blocking, ai->attackRange)) {
            int tw = gw->map->tileWidth;
            int th = gw->map->tileHeight;
            gw->projectile.active = true;
            gw->projectile.sx = mp->x * tw + tw / 2.0f;
            gw->projectile.sy = mp->y * th + th / 2.0f;
            gw->projectile.ex = playerX * tw + tw / 2.0f;
            gw->projectile.ey = playerY * th + th / 2.0f;
            gw->projectile.tileSX = mp->x;
            gw->projectile.tileSY = mp->y;
            gw->projectile.tileEX = playerX;
            gw->projectile.tileEY = playerY;
            gw->projectile.attackType = ai->attackType;
            if (ai->attackType == ATTACK_MAGIC) {
                int row = 1;
                Color magicColor;
                switch (ai->type) {
                    case MONSTER_WARP_SKULL:   row = 3; magicColor = (Color){ 180,  60, 220, 255 }; break;
                    case MONSTER_FLOATING_EYE: row = 2; magicColor = (Color){  60, 200, 220, 255 }; break;
                    case MONSTER_DEMON_EYE:    row = 7; magicColor = (Color){ 220,  40,  40, 255 }; break;
                    default:                   row = 1; magicColor = (Color){  80,  80, 255, 255 }; break;
                }
                gw->projectile.startRow    = row;
                gw->projectile.startFrame  = row * 10;
                gw->projectile.animFrameCount = 9;
                gw->projectile.color       = magicColor;
            } else {
                gw->projectile.startRow    = 0;
                gw->projectile.startFrame  = 0;
                gw->projectile.animFrameCount = 0;
                gw->projectile.color       = (Color){ 139, 69, 19, 255 };
            }
            gw->projectile.projectileVisual = tpl->projectileVisual;
            gw->projectileTimer = PROJECTILE_ANIM_DURATION;
            gw->projectileDuration = PROJECTILE_ANIM_DURATION;

            int dodgePct = playerStats->dex * 2;
            if (dodgePct > 60) dodgePct = 60;
            if (dodgePct > 0 && GetRandomValue(1, 100) <= dodgePct) {
                if (gw->playerEntity != ENTITY_NONE &&
                    World_HasComponents(&gw->ecs, gw->playerEntity, COMP_POSITION)) {
                    CPosition* pp = World_GetPosition(&gw->ecs, gw->playerEntity);
                    FloatMsg_Spawn(gw, pp->x, pp->y, SKYBLUE, "Dodge!");
                }
                CHitFlash* hf = World_GetHitFlash(&gw->ecs, monster);
                hf->timer = 0.15f;
                return;
            }

            int dmg;
            if (ai->attackType == ATTACK_RANGED) {
                dmg = ms->attack + (int)(ms->dex * 1.5f) - playerStats->defense;
            } else {
                int magicDef = playerStats->defense / 2 + playerStats->intel * 2;
                dmg = ms->attack + ms->intel - magicDef;
            }
            if (dmg < 1) dmg = 1;

            if (GetRandomValue(1, 100) <= ms->lck) {
                dmg = dmg * 2;
                if (dmg < 1) dmg = 1;
                FloatMsg_Spawn(gw, playerX, playerY, ORANGE, "Critical!");
            }

            playerStats->hp -= dmg;
            if (playerStats->hp < 0) playerStats->hp = 0;
            {
                PlayerDamagedEvent evt = { dmg, playerStats->hp };
                EventBus_Publish(EVT_PLAYER_DAMAGED, &evt);
            }
            {
                CHitFlash* hf = World_GetHitFlash(&gw->ecs, monster);
                hf->timer = 0.15f;
            }
            CHitFlash* phf = World_GetHitFlash(&gw->ecs, gw->playerEntity);
            phf->timer = 0.15f;

            DamageNumber_Spawn(&gw->damageNumbers, dmg, playerX, playerY, tw, th, RED);

            GameAudio_PlayHitSound();
            const char* verb = (ai->attackType == ATTACK_MAGIC) ? "blasts" : "shoots";
            if (ai->attackType == ATTACK_MAGIC) GameAudio_PlayMagicAttackSound();
            else GameAudio_PlayRangedAttackSound();
            TraceLog(LOG_INFO, "%s %s you for %d damage (HP: %d)!",
                     World_GetName(&gw->ecs, monster)->name, verb, dmg, playerStats->hp);
            return;
        }

        int neighborX[] = { mp->x, mp->x, mp->x - 1, mp->x + 1 };
        int neighborY[] = { mp->y - 1, mp->y + 1, mp->y, mp->y };
        bool canAttack = !(ability == ABILITY_HEAVY_MELEE && ai->attackCooldown > 0);
        for (int i = 0; i < 4; i++) {
            if (neighborX[i] == playerX && neighborY[i] == playerY) {
                if (!canAttack) {
                    ai->attackCooldown--;
                    break;
                }
                int dodgePct = playerStats->dex * 2;
                if (dodgePct > 60) dodgePct = 60;
                if (dodgePct > 0 && GetRandomValue(1, 100) <= dodgePct) {
                    if (gw->playerEntity != ENTITY_NONE &&
                        World_HasComponents(&gw->ecs, gw->playerEntity, COMP_POSITION)) {
                        CPosition* pp = World_GetPosition(&gw->ecs, gw->playerEntity);
                        FloatMsg_Spawn(gw, pp->x, pp->y, SKYBLUE, "Dodge!");
                    }
                    CHitFlash* hf = World_GetHitFlash(&gw->ecs, monster);
                    hf->timer = 0.15f;
                    return;
                }

                int dmg = ms->attack + ms->str * 2 - playerStats->defense;
                if (dmg < 1) dmg = 1;
                if (ability == ABILITY_LIGHT_MELEE) dmg = (int)((float)dmg * 0.85f);
                else if (ability == ABILITY_HEAVY_MELEE) dmg = (int)((float)dmg * 1.40f);
                if (GetRandomValue(1, 100) <= ms->lck) {
                    dmg = dmg * 2;
                    if (dmg < 1) dmg = 1;
                    FloatMsg_Spawn(gw, playerX, playerY, ORANGE, "Critical!");
                }
                playerStats->hp -= dmg;
                if (playerStats->hp < 0) playerStats->hp = 0;
                {
                    PlayerDamagedEvent evt = { dmg, playerStats->hp };
                    EventBus_Publish(EVT_PLAYER_DAMAGED, &evt);
                }
                {
                    int tw = gw->map->tileWidth;
                    int th = gw->map->tileHeight;
                    DamageNumber_Spawn(&gw->damageNumbers, dmg, playerX, playerY, tw, th, RED);
                }
                GameAudio_PlayHitSound();
                CHitFlash* hf2 = World_GetHitFlash(&gw->ecs, monster);
                hf2->timer = 0.15f;
                CHitFlash* phf2 = World_GetHitFlash(&gw->ecs, gw->playerEntity);
                phf2->timer = 0.15f;
                TraceLog(LOG_INFO, "%s attacks you for %d damage (HP: %d)!",
                         World_GetName(&gw->ecs, monster)->name, dmg, playerStats->hp);
                if (ability == ABILITY_HEAVY_MELEE) ai->attackCooldown = 1;
                return;
            }
        }

        if (ability == ABILITY_RANGED) {
            if (dist <= 1) {
                Direction awayDir = MoveAwayFrom(gw, monster, mp, playerX, playerY);
                if (awayDir != DIR_NONE) {
                    ApplyMove(gw, monster, mp, awayDir);
                    DebugLog(DEBUG_AI, "%s: move=away dir=%d pos=(%d,%d)", mname->name, (int)awayDir, mp->x, mp->y);
                }
            } else if (dist <= ai->attackRange && dx != 0 && dy != 0) {
                Direction flankDir = MoveFlank(gw, monster, mp, playerX, playerY);
                if (flankDir != DIR_NONE) {
                    ApplyMove(gw, monster, mp, flankDir);
                    DebugLog(DEBUG_AI, "%s: move=flank dir=%d pos=(%d,%d)", mname->name, (int)flankDir, mp->x, mp->y);
                }
            } else if (dist > ai->attackRange) {
                Direction towardDir = MoveToward(gw, monster, mp, playerX, playerY);
                if (towardDir != DIR_NONE) {
                    ApplyMove(gw, monster, mp, towardDir);
                    DebugLog(DEBUG_AI, "%s: move=toward dir=%d pos=(%d,%d)", mname->name, (int)towardDir, mp->x, mp->y);
                }
            }
        } else {
            if (ability == ABILITY_HEAVY_MELEE) {
                if (ai->attackCooldown > 0) {
                    ai->attackCooldown--;
                }
            }
            Direction flankDir = MoveFlank(gw, monster, mp, playerX, playerY);
            if (flankDir == DIR_NONE) flankDir = MoveToward(gw, monster, mp, playerX, playerY);
            if (flankDir != DIR_NONE) {
                ApplyMove(gw, monster, mp, flankDir);
                DebugLog(DEBUG_AI, "%s: move=approach dir=%d pos=(%d,%d)", mname->name, (int)flankDir, mp->x, mp->y);
            }
        }
        if (ability != ABILITY_RANGED) return;
    }
    else if (ai->huntTurns > 0 && ai->lastSeenX >= 0) {
        ai->huntTurns--;

        int neighborX[] = { mp->x, mp->x, mp->x - 1, mp->x + 1 };
        int neighborY[] = { mp->y - 1, mp->y + 1, mp->y, mp->y };
        for (int i = 0; i < 4; i++) {
            if (neighborX[i] == playerX && neighborY[i] == playerY) {
                int dodgePct = playerStats->dex * 2;
                if (dodgePct > 60) dodgePct = 60;
                if (dodgePct > 0 && GetRandomValue(1, 100) <= dodgePct) {
                    if (gw->playerEntity != ENTITY_NONE &&
                        World_HasComponents(&gw->ecs, gw->playerEntity, COMP_POSITION)) {
                        CPosition* pp = World_GetPosition(&gw->ecs, gw->playerEntity);
                        FloatMsg_Spawn(gw, pp->x, pp->y, SKYBLUE, "Dodge!");
                    }
                    CHitFlash* hf = World_GetHitFlash(&gw->ecs, monster);
                    hf->timer = 0.15f;
                    return;
                }

                int dmg = ms->attack + ms->str * 2 - playerStats->defense;
                if (dmg < 1) dmg = 1;
                if (ability == ABILITY_LIGHT_MELEE) dmg = (int)((float)dmg * 0.85f);
                else if (ability == ABILITY_HEAVY_MELEE) dmg = (int)((float)dmg * 1.40f);
                if (GetRandomValue(1, 100) <= ms->lck) {
                    dmg = dmg * 2;
                    if (dmg < 1) dmg = 1;
                    FloatMsg_Spawn(gw, playerX, playerY, ORANGE, "Critical!");
                }
                playerStats->hp -= dmg;
                if (playerStats->hp < 0) playerStats->hp = 0;
                {
                    PlayerDamagedEvent evt = { dmg, playerStats->hp };
                    EventBus_Publish(EVT_PLAYER_DAMAGED, &evt);
                }
                {
                    int tw = gw->map->tileWidth;
                    int th = gw->map->tileHeight;
                    DamageNumber_Spawn(&gw->damageNumbers, dmg, playerX, playerY, tw, th, RED);
                }
                GameAudio_PlayHitSound();
                CHitFlash* hf = World_GetHitFlash(&gw->ecs, monster);
                hf->timer = 0.15f;
                CHitFlash* phf = World_GetHitFlash(&gw->ecs, gw->playerEntity);
                phf->timer = 0.15f;
                TraceLog(LOG_INFO, "%s attacks you for %d damage (HP: %d)!",
                         World_GetName(&gw->ecs, monster)->name, dmg, playerStats->hp);
                if (ability == ABILITY_HEAVY_MELEE) ai->attackCooldown = 1;
                return;
            }
        }

        Direction huntDir = MoveFlank(gw, monster, mp, ai->lastSeenX, ai->lastSeenY);
        if (huntDir == DIR_NONE) huntDir = MoveToward(gw, monster, mp, ai->lastSeenX, ai->lastSeenY);
        if (huntDir != DIR_NONE) {
            ApplyMove(gw, monster, mp, huntDir);
            DebugLog(DEBUG_AI, "%s: state=hunt lastSeen=(%d,%d) moved=%d->(%d,%d)",
                     mname->name, ai->lastSeenX, ai->lastSeenY, (int)huntDir, mp->x, mp->y);
        } else {
            DebugLog(DEBUG_AI, "%s: state=hunt lastSeen=(%d,%d) blocked", mname->name, ai->lastSeenX, ai->lastSeenY);
        }
    }
    else if (dist <= tpl->detectionRange + 8) {
        bool wandMoved = false;
        for (int attempt = 0; attempt < 4; attempt++) {
            Direction dir = (Direction)(GetRandomValue(DIR_UP, DIR_RIGHT));
            int tx = mp->x, ty = mp->y;
            switch (dir) {
                case DIR_UP:    ty--; break;
                case DIR_DOWN:  ty++; break;
                case DIR_LEFT:  tx--; break;
                case DIR_RIGHT: tx++; break;
                default: break;
            }
            if (tx >= 0 && tx < mapWidth && ty >= 0 && ty < mapHeight &&
                !gw->blocking[ty][tx] && World_FindMonsterAt(gw, tx, ty, monster) == ENTITY_NONE) {
                SpatialHash_Move(gw, monster, mp->x, mp->y, tx, ty);
                mp->x = tx;
                mp->y = ty;
                wandMoved = true;
                break;
            }
        }
        DebugLog(DEBUG_AI, "%s: state=wander dist=%d LOS=no moved=%s pos=(%d,%d)",
                 mname->name, dist, wandMoved ? "yes" : "no", mp->x, mp->y);
    } else {
        DebugLog(DEBUG_AI, "%s: state=idle dist=%d LOS=no", mname->name, dist);
    }
}

bool AISystem_ProcessAll(GameWorld* gw, int timeWaited) {
    if (gw->playerEntity == ENTITY_NONE || !World_HasComponents(&gw->ecs, gw->playerEntity, COMP_POSITION)) {
        TraceLog(LOG_ERROR, "AISystem_ProcessAll: player entity invalid or missing COMP_POSITION [e=%d]", (int)gw->playerEntity);
        return false;
    }
    for (EntityId e = 1; e < (EntityId)gw->ecs.count; e++) {
        if (!gw->ecs.alive[e]) continue;
        if (!World_HasComponents(&gw->ecs, e, COMP_POSITION | COMP_STATS | COMP_AI)) continue;
        if (World_HasComponents(&gw->ecs, e, COMP_PLAYER_TAG)) continue;

        CPosition* p = World_GetPosition(&gw->ecs, e);
        CStats* s = World_GetStats(&gw->ecs, e);
        CAI* ai = World_GetAI(&gw->ecs, e);

        if (!s->alive) {
            TraceLog(LOG_ERROR, "AISystem_ProcessAll: desync — ecs.alive=true but CStats.alive=false [e=%d type=%d]", (int)e, (int)ai->type);
            continue;
        }

        if (Validate_TilePos(gw, p->x, p->y) && gw->monsterGrid[p->y][p->x] != e) {
            TraceLog(LOG_WARNING, "AISystem_ProcessAll: spatial hash desync — CPosition=(%d,%d) but monsterGrid[%d][%d]=%d [e=%d]", p->x, p->y, p->y, p->x, (int)gw->monsterGrid[p->y][p->x], (int)e);
        }

        if (e == gw->shopkeeperEntity) continue;

        p->prevX = p->x;
        p->prevY = p->y;

        if (ai->type == MONSTER_SHADOW && timeWaited < 25) {
            ai->shadowTurnCounter++;
            if (ai->shadowTurnCounter < 2) continue;
            ai->shadowTurnCounter = 0;
        }

        ProcessMonsterAI(gw, e, p, s, ai);
        CStats* ps = World_GetStats(&gw->ecs, gw->playerEntity);
        if (ps->hp <= 0) return false;

        if (ai->type == MONSTER_SHADOW && timeWaited >= 35) {
            ProcessMonsterAI(gw, e, p, s, ai);
            ps = World_GetStats(&gw->ecs, gw->playerEntity);
            if (ps->hp <= 0) return false;
        }
    }
    CStats* ps = World_GetStats(&gw->ecs, gw->playerEntity);
    return (ps->hp > 0);
}
