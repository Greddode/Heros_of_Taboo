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
        .color           = { 40, 0, 80, 255 },
        .spritePath      = "resources/sprite_animations/idle/Shadow.png",
        .frameCount      = 4,
        .animSpeed       = 0.5f,
        .detectionRange  = 20,
        .minFloor        = 1,
        .spawnWeight     = 0,
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
        .color           = { 100, 60, 140, 255 },
        .spritePath      = "resources/sprite_animations/idle/Bat.png",
        .frameCount      = 4,
        .animSpeed       = 0.5f,
        .detectionRange  = 8,
        .minFloor        = 1,
        .spawnWeight     = 14,
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
        .color           = { 80, 160, 80, 255 },
        .spritePath      = "resources/sprite_animations/idle/Goblin.png",
        .frameCount      = 4,
        .animSpeed       = 0.5f,
        .detectionRange  = 8,
        .minFloor        = 1,
        .spawnWeight     = 14,
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
        .color           = { 200, 200, 180, 255 },
        .spritePath      = "resources/sprite_animations/idle/Skeleton.png",
        .frameCount      = 4,
        .animSpeed       = 0.5f,
        .detectionRange  = 8,
        .minFloor        = 1,
        .spawnWeight     = 14,
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
        .color           = { 50, 220, 80, 255 },
        .spritePath      = "resources/sprite_animations/idle/Floating_Eye.png",
        .frameCount      = 4,
        .animSpeed       = 0.5f,
        .detectionRange  = 8,
        .minFloor        = 3,
        .spawnWeight     = 10,
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
        .color           = { 220, 220, 200, 255 },
        .spritePath      = "resources/sprite_animations/idle/Fungal_Myconid.png",
        .frameCount      = 4,
        .animSpeed       = 0.5f,
        .detectionRange  = 8,
        .minFloor        = 3,
        .spawnWeight     = 10,
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
        .color           = { 160, 40, 200, 255 },
        .spritePath      = "resources/sprite_animations/idle/Warp_Skull.png",
        .frameCount      = 4,
        .animSpeed       = 0.5f,
        .detectionRange  = 10,
        .minFloor        = 3,
        .spawnWeight     = 10,
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
        .color           = { 200, 50, 50, 255 },
        .spritePath      = "resources/sprite_animations/idle/Demon_Eye.png",
        .frameCount      = 4,
        .animSpeed       = 0.5f,
        .detectionRange  = 10,
        .minFloor        = 6,
        .spawnWeight     = 7,
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
        .color           = { 120, 60, 180, 255 },
        .spritePath      = "resources/sprite_animations/idle/Ogre.png",
        .frameCount      = 4,
        .animSpeed       = 0.5f,
        .detectionRange  = 8,
        .minFloor        = 6,
        .spawnWeight     = 7,
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
        .color           = { 180, 80, 20, 255 },
        .spritePath      = "resources/sprite_animations/idle/Dragon.png",
        .frameCount      = 4,
        .animSpeed       = 0.5f,
        .detectionRange  = 10,
        .minFloor        = 6,
        .spawnWeight     = 7,
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
    m->hp       = tpl->hp;
    m->maxHp    = tpl->hp;
    m->attack   = tpl->attack;
    m->defense  = tpl->defense;
    m->level    = tpl->level;
    m->expValue = tpl->expValue;
    m->alive    = true;
    m->active   = true;
    m->facingRight = true;
    strncpy(m->name, tpl->name, MONSTER_NAME_LEN - 1);
    m->name[MONSTER_NAME_LEN - 1] = '\0';

    if (floor > 1) {
        int extra = (floor - 1) * GetRandomValue(1, 3);
        m->level    = tpl->level + extra;
        m->maxHp    = tpl->hp + extra * 2;
        m->hp       = m->maxHp;
        m->attack   = tpl->attack + extra;
        m->defense  = tpl->defense + extra / 2;
        m->expValue = tpl->expValue + extra * 5;
    }

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
    int dist = dx > dy ? dx : dy;
    if (dist > maxDist) return false;

    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    int x = x0, y = y0;
    while (x != x1 || y != y1) {
        if ((x != x0 || y != y0) && blocking[y][x]) return false;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x += sx; }
        if (e2 <  dx) { err += dx; y += sy; }
    }
    return true;
}

// ============================================================================
//  AI  –  processes all living monsters in one batch
// ============================================================================

// Process a single monster's AI (chase/attack/wander).
// Does NOT snapshot prevX/prevY — that is done by the caller.
static void ProcessMonsterAI(Monster* m,
                              int playerX, int playerY, int* playerHp, int playerDefense,
                              float* playerHitFlash,
                              const unsigned char blocking[][MAP_WIDTH],
                              int mapWidth, int mapHeight,
                              CombatLog* combatLog) {
    const MonsterTemplate* tpl = Monster_GetTemplate(m->type);
    if (!tpl) return;

    int dx = playerX - m->x;
    int dy = playerY - m->y;
    int dist = abs(dx) + abs(dy);

    // --- chase / attack ----------------------------------------------------
    if (dist <= tpl->detectionRange &&
        MonsterLineOfSight(m->x, m->y, playerX, playerY, blocking, tpl->detectionRange)) {

        Direction bestDir = DIR_NONE;
        int bestDist = dist;
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

            // can attack the player?
            if (tx == playerX && ty == playerY) {
                int dmg = m->attack - playerDefense;
                if (dmg < 1) dmg = 1;
                *playerHp -= dmg;
                if (*playerHp < 0) *playerHp = 0;
                PlayHitSound();
                m->hitFlashTimer = 0.15f;
                if (playerHitFlash) *playerHitFlash = 0.15f;
                TraceLog(LOG_INFO, "%s attacks you for %d damage (HP: %d)!",
                         m->name, dmg, *playerHp);
                CombatLog_Add(combatLog, LIGHTGRAY, "%s hits you for %d!", m->name, dmg);
                return;  // skip movement — we attacked
            }

            if (tx >= 0 && tx < mapWidth && ty >= 0 && ty < mapHeight &&
                !blocking[ty][tx] && !Monster_GetAt(tx, ty, m)) {
                int newDist = abs(playerX - tx) + abs(playerY - ty);
                if (newDist < bestDist) {
                    bestDist = newDist;
                    bestDir = dirs[d];
                }
            }
        }

        if (bestDir != DIR_NONE) {
            switch (bestDir) {
                case DIR_UP:    m->y--; break;
                case DIR_DOWN:  m->y++; break;
                case DIR_LEFT:  m->x--; m->facingRight = false; break;
                case DIR_RIGHT: m->x++; m->facingRight = true;  break;
                default: break;
            }
        }
    }
    // --- wander ------------------------------------------------------------
    else if (dist <= tpl->detectionRange + 4) {
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
        }
    }
}

bool Monster_ProcessAllAI(int playerX, int playerY, int* playerHp, int playerDefense,
                           float* playerHitFlash,
                           const unsigned char blocking[][MAP_WIDTH],
                           int mapWidth, int mapHeight,
                           CombatLog* combatLog,
                           int timeWaited) {
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

        ProcessMonsterAI(m, playerX, playerY, playerHp, playerDefense,
                         playerHitFlash, blocking, mapWidth, mapHeight, combatLog);
        if (*playerHp <= 0) return false;

        // --- Frenzy: second action for shadow ----------------------------------
        if (m->type == MONSTER_SHADOW && timeWaited >= 35) {
            ProcessMonsterAI(m, playerX, playerY, playerHp, playerDefense,
                             playerHitFlash, blocking, mapWidth, mapHeight, combatLog);
            if (*playerHp <= 0) return false;
        }
    }
    return (*playerHp > 0);
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
