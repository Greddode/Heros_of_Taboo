#include "monster.h"
#include "core/game.h"
#include "core/audio.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// ============================================================================
//  INTERNAL STATE
// ============================================================================

static MonsterTemplate s_templates[MONSTER_TYPE_COUNT];
static bool s_templatesReady = false;

static Monster s_monsters[MAX_MONSTERS];
static int s_monsterCount = 0;

static Texture2D s_monsterTextures[MONSTER_TYPE_COUNT];

// ============================================================================
//  TEMPLATES  –  add a new block here for each new monster type
// ============================================================================

void Monster_InitTemplates(void) {
    if (s_templatesReady) return;
    s_templatesReady = true;

    s_templates[MONSTER_SHADOW] = (MonsterTemplate){
        .type            = MONSTER_SHADOW,
        .tmxTypeName     = "shadow",
        .name            = "Shadow",
        .hp              = 10,
        .attack          = 4,
        .defense         = 1,
        .expValue        = 10,
        .level           = 1,
        .str             = 4,
        .dex             = 6,
        .intel           = 2,
        .con             = 3,
        .lck             = 5,
        .color           = { 40, 0, 80, 255 },
        .spritePath      = "resources/sprites/monsters/Shadow.png",
        .frameCount      = 4,
        .animSpeed       = 0.5f,
        .detectionRange  = 20,
        .minFloor        = 1,
        .spawnWeight     = 0,
        .attackType      = ATTACK_MELEE,
        .attackRange     = 1,
    };

    // --- Group 1: Early (floors 1+) -----------------------------------------
    s_templates[MONSTER_BAT] = (MonsterTemplate){
        .type            = MONSTER_BAT,
        .tmxTypeName     = "bat",
        .name            = "Bat",
        .hp              = 5,
        .attack          = 2,
        .defense         = 0,
        .expValue        = 10,
        .level           = 1,
        .str             = 1,
        .dex             = 3,
        .intel           = 0,
        .con             = 1,
        .lck             = 1,
        .color           = { 100, 60, 140, 255 },
        .spritePath      = "resources/sprites/monsters/Bat.png",
        .frameCount      = 4,
        .animSpeed       = 0.5f,
        .detectionRange  = 8,
        .minFloor        = 1,
        .spawnWeight     = 14,
        .attackType      = ATTACK_MELEE,
        .attackRange     = 1,
    };

    s_templates[MONSTER_GOBLIN] = (MonsterTemplate){
        .type            = MONSTER_GOBLIN,
        .tmxTypeName     = "goblin",
        .name            = "Goblin",
        .hp              = 7,
        .attack          = 3,
        .defense         = 1,
        .expValue        = 12,
        .level           = 1,
        .str             = 2,
        .dex             = 2,
        .intel           = 1,
        .con             = 2,
        .lck             = 2,
        .color           = { 80, 160, 80, 255 },
        .spritePath      = "resources/sprites/monsters/Goblin.png",
        .frameCount      = 4,
        .animSpeed       = 0.5f,
        .detectionRange  = 8,
        .minFloor        = 1,
        .spawnWeight     = 14,
        .attackType      = ATTACK_MELEE,
        .attackRange     = 1,
    };

    s_templates[MONSTER_SKELETON] = (MonsterTemplate){
        .type            = MONSTER_SKELETON,
        .tmxTypeName     = "skeleton",
        .name            = "Skeleton",
        .hp              = 8,
        .attack          = 3,
        .defense         = 1,
        .expValue        = 14,
        .level           = 1,
        .str             = 2,
        .dex             = 1,
        .intel           = 0,
        .con             = 3,
        .lck             = 1,
        .color           = { 200, 200, 180, 255 },
        .spritePath      = "resources/sprites/monsters/Skeleton.png",
        .frameCount      = 4,
        .animSpeed       = 0.5f,
        .detectionRange  = 8,
        .minFloor        = 1,
        .spawnWeight     = 14,
        .attackType      = ATTACK_MELEE,
        .attackRange     = 1,
    };

    // --- Group 2: Mid (floors 3+) -------------------------------------------
    s_templates[MONSTER_FLOATING_EYE] = (MonsterTemplate){
        .type            = MONSTER_FLOATING_EYE,
        .tmxTypeName     = "floating_eye",
        .name            = "Floating Eye",
        .hp              = 10,
        .attack          = 4,
        .defense         = 1,
        .expValue        = 22,
        .level           = 1,
        .str             = 3,
        .dex             = 2,
        .intel           = 2,
        .con             = 3,
        .lck             = 2,
        .color           = { 50, 220, 80, 255 },
        .spritePath      = "resources/sprites/monsters/Floating_Eye.png",
        .frameCount      = 4,
        .animSpeed       = 0.5f,
        .detectionRange  = 8,
        .minFloor        = 3,
        .spawnWeight     = 10,
        .attackType      = ATTACK_MELEE,
        .attackRange     = 1,
    };

    s_templates[MONSTER_FUNGAL_MYCONID] = (MonsterTemplate){
        .type            = MONSTER_FUNGAL_MYCONID,
        .tmxTypeName     = "fungal_myconid",
        .name            = "Fungal Myconid",
        .hp              = 12,
        .attack          = 5,
        .defense         = 2,
        .expValue        = 26,
        .level           = 1,
        .str             = 2,
        .dex             = 1,
        .intel           = 0,
        .con             = 5,
        .lck             = 1,
        .color           = { 220, 220, 200, 255 },
        .spritePath      = "resources/sprites/monsters/Fungal_Myconid.png",
        .frameCount      = 4,
        .animSpeed       = 0.5f,
        .detectionRange  = 8,
        .minFloor        = 3,
        .spawnWeight     = 10,
        .attackType      = ATTACK_MELEE,
        .attackRange     = 1,
    };

    s_templates[MONSTER_WARP_SKULL] = (MonsterTemplate){
        .type            = MONSTER_WARP_SKULL,
        .tmxTypeName     = "warp_skull",
        .name            = "Warp Skull",
        .hp              = 11,
        .attack          = 5,
        .defense         = 1,
        .expValue        = 30,
        .level           = 1,
        .str             = 1,
        .dex             = 3,
        .intel           = 5,
        .con             = 2,
        .lck             = 2,
        .color           = { 160, 40, 200, 255 },
        .spritePath      = "resources/sprites/monsters/Warp_Skull.png",
        .frameCount      = 4,
        .animSpeed       = 0.5f,
        .detectionRange  = 10,
        .minFloor        = 3,
        .spawnWeight     = 10,
        .attackType      = ATTACK_MAGIC,
        .attackRange     = 3,
    };

    // --- Group 3: Late (floors 6+) ------------------------------------------
    s_templates[MONSTER_DEMON_EYE] = (MonsterTemplate){
        .type            = MONSTER_DEMON_EYE,
        .tmxTypeName     = "demon_eye",
        .name            = "Demon Eye",
        .hp              = 18,
        .attack          = 6,
        .defense         = 2,
        .expValue        = 45,
        .level           = 1,
        .str             = 5,
        .dex             = 2,
        .intel           = 3,
        .con             = 4,
        .lck             = 2,
        .color           = { 200, 50, 50, 255 },
        .spritePath      = "resources/sprites/monsters/Demon_Eye.png",
        .frameCount      = 4,
        .animSpeed       = 0.5f,
        .detectionRange  = 10,
        .minFloor        = 6,
        .spawnWeight     = 7,
        .attackType      = ATTACK_MELEE,
        .attackRange     = 1,
    };

    s_templates[MONSTER_OGRE] = (MonsterTemplate){
        .type            = MONSTER_OGRE,
        .tmxTypeName     = "ogre",
        .name            = "Ogre",
        .hp              = 22,
        .attack          = 6,
        .defense         = 3,
        .expValue        = 55,
        .level           = 1,
        .str             = 7,
        .dex             = 1,
        .intel           = 0,
        .con             = 7,
        .lck             = 1,
        .color           = { 120, 60, 180, 255 },
        .spritePath      = "resources/sprites/monsters/Ogre.png",
        .frameCount      = 4,
        .animSpeed       = 0.5f,
        .detectionRange  = 8,
        .minFloor        = 6,
        .spawnWeight     = 7,
        .attackType      = ATTACK_MELEE,
        .attackRange     = 1,
    };

    s_templates[MONSTER_DRAGON] = (MonsterTemplate){
        .type            = MONSTER_DRAGON,
        .tmxTypeName     = "dragon",
        .name            = "Dragon",
        .hp              = 28,
        .attack          = 7,
        .defense         = 3,
        .expValue        = 70,
        .level           = 1,
        .str             = 9,
        .dex             = 3,
        .intel           = 4,
        .con             = 8,
        .lck             = 3,
        .color           = { 180, 80, 20, 255 },
        .spritePath      = "resources/sprites/monsters/Dragon.png",
        .frameCount      = 4,
        .animSpeed       = 0.5f,
        .detectionRange  = 10,
        .minFloor        = 6,
        .spawnWeight     = 7,
        .attackType      = ATTACK_MELEE,
        .attackRange     = 1,
    };

    // --- Ranger: early floor 2+ (shorter hp, ranged attack) -------------------
    s_templates[MONSTER_RANGER_GOBLIN] = (MonsterTemplate){
        .type            = MONSTER_RANGER_GOBLIN,
        .tmxTypeName     = "goblin_archer",
        .name            = "Goblin Archer",
        .hp              = 5,
        .attack          = 2,
        .defense         = 0,
        .expValue        = 14,
        .level           = 1,
        .str             = 1,
        .dex             = 5,
        .intel           = 1,
        .con             = 1,
        .lck             = 3,
        .color           = { 100, 150, 60, 255 },
        .spritePath      = "resources/sprites/monsters/Goblin_Archer.png",
        .frameCount      = 4,
        .animSpeed       = 0.5f,
        .detectionRange  = 9,
        .minFloor        = 2,
        .spawnWeight     = 8,
        .attackType      = ATTACK_RANGED,
        .attackRange     = 5,
    };
}

const MonsterTemplate* Monster_GetTemplate(MonsterType type) {
    if (!s_templatesReady) Monster_InitTemplates();
    if (type < 0 || type >= MONSTER_TYPE_COUNT) return NULL;
    return &s_templates[type];
}

// ============================================================================
//  SPRITE SHEET MANAGEMENT
// ============================================================================

void Monster_LoadSprites(void) {
    for (int i = 0; i < MONSTER_TYPE_COUNT; i++) {
        s_monsterTextures[i] = (Texture2D){ 0 };
        const MonsterTemplate* tpl = &s_templates[i];
        if (tpl->spritePath && tpl->frameCount > 0) {
            s_monsterTextures[i] = LoadTexture(tpl->spritePath);
            if (s_monsterTextures[i].id == 0) {
                TraceLog(LOG_WARNING, "Monster: Could not load sprite %s", tpl->spritePath);
            } else {
                TraceLog(LOG_INFO, "Monster: Loaded %s (%dx%d)",
                         tpl->name, s_monsterTextures[i].width, s_monsterTextures[i].height);
            }
        }
    }
}

void Monster_UnloadSprites(void) {
    for (int i = 0; i < MONSTER_TYPE_COUNT; i++) {
        if (s_monsterTextures[i].id > 0) {
            UnloadTexture(s_monsterTextures[i]);
            s_monsterTextures[i] = (Texture2D){ 0 };
        }
    }
}

// ============================================================================
//  SPAWN / RESET
// ============================================================================

Monster* Monster_Spawn(MonsterType type, int x, int y, int floor) {
    const MonsterTemplate* tpl = Monster_GetTemplate(type);
    if (!tpl) return NULL;
    if (s_monsterCount >= MAX_MONSTERS) return NULL;

    Monster* m = &s_monsters[s_monsterCount++];
    memset(m, 0, sizeof(Monster));

    m->type     = type;
    m->x        = x;
    m->y        = y;
    m->prevX    = x;
    m->prevY    = y;

    // Determine floor-based scaling factor
    float scale = powf(1.12f, (float)floor);

    // Base stats from template, scaled by floor
    m->str       = (int)(tpl->str * scale);
    m->dex       = (int)(tpl->dex * scale);
    m->intel     = (int)(tpl->intel * scale);
    m->con       = (int)(tpl->con * scale);
    m->lck       = (int)(tpl->lck * scale);

    // Derived stats
    int baseHp = tpl->hp;
    m->maxHp    = (int)(baseHp * scale) + m->con * 5;
    m->hp       = m->maxHp;
    m->attack   = (int)(tpl->attack * scale);
    m->defense  = (int)(tpl->defense * scale) + m->con / 2;
    m->level       = tpl->level + floor * GetRandomValue(1, 3);
    if (m->level < 1) m->level = 1;
    m->expValue    = (int)(tpl->expValue * scale) + m->lck * 3;
    m->attackType  = tpl->attackType;
    m->attackRange = tpl->attackRange;
    m->alive       = true;
    m->active      = true;
    m->facingRight = true;
    m->lastSeenX   = -1;
    m->lastSeenY   = -1;
    m->huntTurns   = 0;
    strncpy(m->name, tpl->name, MONSTER_NAME_LEN - 1);
    m->name[MONSTER_NAME_LEN - 1] = '\0';

    return m;
}

Monster* Monster_SpawnByTypeName(const char* tmxTypeName, int x, int y, int floor) {
    if (!tmxTypeName) return NULL;
    for (int i = 0; i < MONSTER_TYPE_COUNT; i++) {
        const MonsterTemplate* tpl = &s_templates[i];
        if (tpl->tmxTypeName && strcmp(tmxTypeName, tpl->tmxTypeName) == 0)
            return Monster_Spawn((MonsterType)i, x, y, floor);
    }
    return NULL;
}

void Monster_ResetAll(void) {
    s_monsterCount = 0;
}

// ============================================================================
//  QUERIES
// ============================================================================

int Monster_GetAliveCount(void) {
    int count = 0;
    for (int i = 0; i < s_monsterCount; i++) {
        if (s_monsters[i].alive && s_monsters[i].active) count++;
    }
    return count;
}

bool Monster_AreAllDead(void) {
    return Monster_GetAliveCount() == 0;
}

Monster* Monster_GetAt(int x, int y, const Monster* exclude) {
    for (int i = 0; i < s_monsterCount; i++) {
        Monster* m = &s_monsters[i];
        if (m != exclude && m->alive && m->active && m->x == x && m->y == y) {
            return m;
        }
    }
    return NULL;
}

Monster* Monster_GetArray(void) {
    return s_monsters;
}

int Monster_GetCount(void) {
    return s_monsterCount;
}

// ============================================================================
//  ANIMATIONS
// ============================================================================

void Monster_UpdateAnimations(float dt) {
    for (int i = 0; i < s_monsterCount; i++) {
        Monster* m = &s_monsters[i];
        if (!m->alive || !m->active) continue;

        // Flash timer countdown
        if (m->hitFlashTimer > 0.0f) {
            m->hitFlashTimer -= dt;
            if (m->hitFlashTimer < 0.0f) m->hitFlashTimer = 0.0f;
        }

        const MonsterTemplate* tpl = Monster_GetTemplate(m->type);
        if (!tpl || tpl->frameCount <= 1) continue;

        m->animTimer += dt;
        if (m->animTimer >= tpl->animSpeed) {
            m->animTimer -= tpl->animSpeed;
            m->animFrame = (m->animFrame + 1) % tpl->frameCount;
        }
    }
}

// ============================================================================
//  LINE-OF-SIGHT  (simple Bresenham, local helper)
// ============================================================================

static bool MonsterLineOfSight(int x0, int y0, int x1, int y1,
                            const unsigned char blocking[][MAP_WIDTH],
                            int maxDist) {
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

// ============================================================================
//  AI  –  processes all living monsters in one batch
// ============================================================================

// Evaluate movement toward a target tile, returns best direction or DIR_NONE.
static Direction MoveToward(Monster* m, int targetX, int targetY,
                            const unsigned char blocking[][MAP_WIDTH],
                            int mapWidth, int mapHeight) {
    Direction bestDir = DIR_NONE;
    int bestDist = abs(targetX - m->x) + abs(targetY - m->y);
    Direction dirs[] = { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT };
    for (int d = 0; d < 4; d++) {
        int tx = m->x, ty = m->y;
        switch (dirs[d]) {
            case DIR_UP:    ty--; break;
            case DIR_DOWN:  ty++; break;
            case DIR_LEFT:  tx--; break;
            case DIR_RIGHT: tx++; break;
            default: break;
        }
        if (tx >= 0 && tx < mapWidth && ty >= 0 && ty < mapHeight &&
            !blocking[ty][tx] && !Monster_GetAt(tx, ty, m)) {
            int nd = abs(targetX - tx) + abs(targetY - ty);
            if (nd < bestDist) { bestDist = nd; bestDir = dirs[d]; }
        }
    }
    return bestDir;
}

// Apply a directional move to the monster, updating facing.
static void ApplyMove(Monster* m, Direction dir) {
    switch (dir) {
        case DIR_UP:    m->y--; break;
        case DIR_DOWN:  m->y++; break;
        case DIR_LEFT:  m->x--; m->facingRight = false; break;
        case DIR_RIGHT: m->x++; m->facingRight = true;  break;
        default: break;
    }
}

// Process a single monster's AI (chase/attack/wander).
// Does NOT snapshot prevX/prevY — that is done by the caller.
static void ProcessMonsterAI(Monster* m, Game* game) {
    const MonsterTemplate* tpl = Monster_GetTemplate(m->type);
    if (!tpl) return;

    int playerX = game->player.ent.x;
    int playerY = game->player.ent.y;
    int dx = playerX - m->x;
    int dy = playerY - m->y;
    int dist = abs(dx) + abs(dy);
    int mapWidth = game->map->width;
    int mapHeight = game->map->height;
    const unsigned char(*blocking)[MAP_WIDTH] = game->blocking;
    bool hasLOS = MonsterLineOfSight(m->x, m->y, playerX, playerY,
                                     blocking, tpl->detectionRange);

    // --- chase / attack (player in LOS) -----------------------------------
    if (dist <= tpl->detectionRange && hasLOS) {
        // Remember where we last saw the player
        m->lastSeenX = playerX;
        m->lastSeenY = playerY;
        m->huntTurns = 4;

        // Ranged / magic attack check
        if (m->attackType != ATTACK_MELEE && dist <= m->attackRange &&
            (dx == 0 || dy == 0) &&
            MonsterLineOfSight(m->x, m->y, playerX, playerY, blocking, m->attackRange)) {
            // Player dodge check (DEX-based)
            int dodgePct = game->player.ent.dex * 2;
            if (dodgePct > 60) dodgePct = 60;
            if (dodgePct > 0 && GetRandomValue(1, 100) <= dodgePct) {
                m->hitFlashTimer = 0.15f;
                CombatLog_Add(&game->combatLog, BLACK, "You dodge the %s's attack!", m->name);
                TraceLog(LOG_INFO, "Player dodges %s's attack!", m->name);
                return;
            }

            int dmg;
            if (m->attackType == ATTACK_RANGED) {
                dmg = m->attack + (int)(m->dex * 1.5f) - game->player.ent.defense;
            } else { // ATTACK_MAGIC
                dmg = m->attack + m->intel - game->player.ent.defense;
                // Apply magic resistance (MGC × 3) for magic attacks
                int magicRes = game->player.ent.intel * 3;
                dmg -= magicRes;
            }
            if (dmg < 1) dmg = 1;

            // Monster crit check
            if (GetRandomValue(1, 100) <= m->lck) {
                dmg = dmg * 2;
                if (dmg < 1) dmg = 1;
                CombatLog_Add(&game->combatLog, BLACK, "Critical hit!");
            }

            game->player.ent.hp -= dmg;
            if (game->player.ent.hp < 0) game->player.ent.hp = 0;
            m->hitFlashTimer = 0.15f;
            game->player.ent.hitFlashTimer = 0.15f;

            int tw = game->map->tileWidth;
            int th = game->map->tileHeight;
            game->projectile.active = true;
            game->projectile.sx = m->x * tw + tw / 2.0f;
            game->projectile.sy = m->y * th + th / 2.0f;
            game->projectile.ex = playerX * tw + tw / 2.0f;
            game->projectile.ey = playerY * th + th / 2.0f;
            game->projectile.tileSX = m->x;
            game->projectile.tileSY = m->y;
            game->projectile.tileEX = playerX;
            game->projectile.tileEY = playerY;
            game->projectile.attackType = m->attackType;
            game->projectile.color = (m->attackType == ATTACK_MAGIC) ? (Color){ 80, 80, 255, 255 } : (Color){ 139, 69, 19, 255 };
            if (m->attackType == ATTACK_MAGIC) {
                game->projectile.startFrame = 30;
                game->projectile.animFrameCount = 9;
            } else {
                game->projectile.startFrame = 0;
                game->projectile.animFrameCount = 0;
            }
            game->projectileTimer = PROJECTILE_ANIM_DURATION;
            game->projectileDuration = PROJECTILE_ANIM_DURATION;

            PlayHitSound();
            const char* verb = (m->attackType == ATTACK_MAGIC) ? "blasts" : "shoots";
            if (m->attackType == ATTACK_MAGIC) {
                PlayMagicAttackSound();
            } else {
                PlayRangedAttackSound();
            }
            TraceLog(LOG_INFO, "%s %s you for %d damage (HP: %d)!",
                     m->name, verb, dmg, game->player.ent.hp);
            CombatLog_Add(&game->combatLog, BLACK, "%s %s you for %d!", m->name, verb, dmg);
            return;
        }

        // Melee attack check (check if any adjacent tile has the player)
        int neighborX[] = { m->x, m->x, m->x - 1, m->x + 1 };
        int neighborY[] = { m->y - 1, m->y + 1, m->y, m->y };
        for (int i = 0; i < 4; i++) {
            if (neighborX[i] == playerX && neighborY[i] == playerY) {
                // Player dodge
                int dodgePct = game->player.ent.dex * 2;
                if (dodgePct > 60) dodgePct = 60;
                if (dodgePct > 0 && GetRandomValue(1, 100) <= dodgePct) {
                    m->hitFlashTimer = 0.15f;
                    CombatLog_Add(&game->combatLog, BLACK, "You dodge the %s's attack!", m->name);
                    TraceLog(LOG_INFO, "Player dodges %s's attack!", m->name);
                    return;
                }

                int dmg = m->attack + m->str * 2 - game->player.ent.defense;
                if (dmg < 1) dmg = 1;

                // Monster crit
                if (GetRandomValue(1, 100) <= m->lck) {
                    dmg = dmg * 2;
                    if (dmg < 1) dmg = 1;
                    CombatLog_Add(&game->combatLog, BLACK, "Critical hit!");
                }

                game->player.ent.hp -= dmg;
                if (game->player.ent.hp < 0) game->player.ent.hp = 0;
                PlayHitSound();
                m->hitFlashTimer = 0.15f;
                game->player.ent.hitFlashTimer = 0.15f;
                TraceLog(LOG_INFO, "%s attacks you for %d damage (HP: %d)!",
                         m->name, dmg, game->player.ent.hp);
                CombatLog_Add(&game->combatLog, BLACK, "%s hits you for %d!", m->name, dmg);
                return;
            }
        }

        // Move toward player
        Direction bestDir = MoveToward(m, playerX, playerY, blocking, mapWidth, mapHeight);
        if (bestDir != DIR_NONE) ApplyMove(m, bestDir);
    }
    // --- hunt: chase last known position when LOS is lost ------------------
    else if (m->huntTurns > 0 && m->lastSeenX >= 0) {
        m->huntTurns--;

        // If we stumble back onto the player, melee
        int neighborX[] = { m->x, m->x, m->x - 1, m->x + 1 };
        int neighborY[] = { m->y - 1, m->y + 1, m->y, m->y };
        for (int i = 0; i < 4; i++) {
            if (neighborX[i] == playerX && neighborY[i] == playerY) {
                int dodgePct = game->player.ent.dex * 2;
                if (dodgePct > 60) dodgePct = 60;
                if (dodgePct > 0 && GetRandomValue(1, 100) <= dodgePct) {
                    m->hitFlashTimer = 0.15f;
                    CombatLog_Add(&game->combatLog, BLACK, "You dodge the %s's attack!", m->name);
                    return;
                }
                int dmg = m->attack + m->str * 2 - game->player.ent.defense;
                if (dmg < 1) dmg = 1;
                if (GetRandomValue(1, 100) <= m->lck) {
                    dmg = dmg * 2;
                    if (dmg < 1) dmg = 1;
                    CombatLog_Add(&game->combatLog, BLACK, "Critical hit!");
                }
                game->player.ent.hp -= dmg;
                if (game->player.ent.hp < 0) game->player.ent.hp = 0;
                PlayHitSound();
                m->hitFlashTimer = 0.15f;
                game->player.ent.hitFlashTimer = 0.15f;
                TraceLog(LOG_INFO, "%s attacks you for %d damage (HP: %d)!",
                         m->name, dmg, game->player.ent.hp);
                CombatLog_Add(&game->combatLog, BLACK, "%s hits you for %d!", m->name, dmg);
                return;
            }
        }

        Direction bestDir = MoveToward(m, m->lastSeenX, m->lastSeenY,
                                       blocking, mapWidth, mapHeight);
        if (bestDir != DIR_NONE) ApplyMove(m, bestDir);
    }
    // --- wander ------------------------------------------------------------
    else if (dist <= tpl->detectionRange + 8) {
        for (int attempt = 0; attempt < 4; attempt++) {
            Direction dir = (Direction)(GetRandomValue(DIR_UP, DIR_RIGHT));
            int tx = m->x, ty = m->y;
            switch (dir) {
                case DIR_UP:    ty--; break;
                case DIR_DOWN:  ty++; break;
                case DIR_LEFT:  tx--; break;
                case DIR_RIGHT: tx++; break;
                default: break;
            }
            if (tx >= 0 && tx < mapWidth && ty >= 0 && ty < mapHeight &&
                !blocking[ty][tx] && !Monster_GetAt(tx, ty, m)) {
                m->x = tx;
                m->y = ty;
                if (dir == DIR_LEFT)  m->facingRight = false;
                if (dir == DIR_RIGHT) m->facingRight = true;
                break;
            }
        }
    }
}

bool Monster_ProcessAllAI(Game* game, int timeWaited) {
    for (int i = 0; i < s_monsterCount; i++) {
        Monster* m = &s_monsters[i];
        if (!m->alive || !m->active) continue;

        // --- snapshot old position for interpolation ---------------------------
        m->prevX = m->x;
        m->prevY = m->y;

        // --- Shadow state handling ---------------------------------------------
        if (m->type == MONSTER_SHADOW && timeWaited < 25) {
            m->shadowTurnCounter++;
            if (m->shadowTurnCounter < 2) continue;
            m->shadowTurnCounter = 0;
        }

        ProcessMonsterAI(m, game);
        if (game->player.ent.hp <= 0) return false;

        // --- Frenzy: second action for shadow ----------------------------------
        if (m->type == MONSTER_SHADOW && timeWaited >= 35) {
            ProcessMonsterAI(m, game);
            if (game->player.ent.hp <= 0) return false;
        }
    }
    return (game->player.ent.hp > 0);
}

// ============================================================================
//  RENDER
// ============================================================================

void Monster_RenderAll(const unsigned char visibility[][MAP_WIDTH],
                        int mapWidth, int mapHeight,
                        int tileWidth, int tileHeight,
                        float t) {
    for (int i = 0; i < s_monsterCount; i++) {
        Monster* m = &s_monsters[i];
        if (!m->alive || !m->active) continue;

        // ---- visibility -------------------------------------------------------
        if (m->y < 0 || m->y >= mapHeight || m->x < 0 || m->x >= mapWidth) continue;
        if (visibility[m->y][m->x] != 1) continue;

        const MonsterTemplate* tpl = Monster_GetTemplate(m->type);
        if (!tpl) continue;

        // ---- interpolated pixel position --------------------------------------
        int pixelX, pixelY;
        if (t >= 0.0f && t <= 1.0f) {
            pixelX = (int)(m->prevX * tileWidth  + (m->x - m->prevX) * tileWidth  * t);
            pixelY = (int)(m->prevY * tileHeight + (m->y - m->prevY) * tileHeight * t);
        } else {
            pixelX = m->x * tileWidth;
            pixelY = m->y * tileHeight;
        }

        // ---- draw from sprite sheet or coloured rectangle ----------------------
        Texture2D tex = s_monsterTextures[m->type];
        if (tex.id > 0 && tpl->frameCount > 0) {
            float frameW = (float)tex.width / (float)tpl->frameCount;
            float frameH = (float)tex.height;

            Rectangle src = {
                (float)m->animFrame * frameW, 0,
                frameW, frameH
            };
            Rectangle dest = {
                (float)pixelX, (float)pixelY,
                frameW, frameH
            };
            DrawTexturePro(tex, src, dest, (Vector2){ 0, 0 }, 0, WHITE);
            // White flash overlay for sprites
            if (m->hitFlashTimer > 0.0f) {
                unsigned char alpha = (unsigned char)(m->hitFlashTimer * 4.0f * 255.0f);
                if (alpha > 200) alpha = 200;
                DrawRectangle(pixelX, pixelY, (int)frameW, (int)frameH,
                              (Color){ 255, 255, 255, alpha });
            }
        } else {
            // coloured rectangle with eyes
            int pad = tileWidth / 6;
            Color drawColor = (m->hitFlashTimer > 0.0f) ? WHITE : tpl->color;
            DrawRectangle(pixelX + pad, pixelY + pad,
                          tileWidth - 2 * pad, tileHeight - 2 * pad, drawColor);
            int cx = pixelX + tileWidth / 2;
            int cy = pixelY + tileHeight / 2;
            DrawCircle(cx - 3, cy - 2, 2, RED);
            DrawCircle(cx + 3, cy - 2, 2, RED);
            if (m->facingRight) {
                DrawCircle(cx + 6, cy + 4, 1, RED);
            } else {
                DrawCircle(cx - 6, cy + 4, 1, RED);
            }
        }
    }
}
