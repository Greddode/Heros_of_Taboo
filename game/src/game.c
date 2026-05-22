#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ---------------------------------------------------------------------------
// Color palette for different entity types
// ---------------------------------------------------------------------------
static Color GetEntityColor(EntityType type) {
    switch (type) {
        case ENTITY_PLAYER:  return (Color){ 50, 200, 255, 255 }; // Cyan
        case ENTITY_GOBLIN:  return (Color){ 50, 220, 80, 255 };  // Green
        case ENTITY_SKELETON: return (Color){ 220, 220, 200, 255 }; // Bone white
        case ENTITY_ORC:     return (Color){ 200, 100, 50, 255 };  // Brown/red
        case ENTITY_OGRE:    return (Color){ 120, 60, 180, 255 };  // Purple
        default: return WHITE;
    }
}

// ---------------------------------------------------------------------------
// Tile <-> Screen coordinate conversion
// ---------------------------------------------------------------------------
Vector2 TileToScreen(int x, int y, int tileWidth, int tileHeight) {
    return (Vector2){ (float)(x * tileWidth), (float)(y * tileHeight) };
}

Vector2 ScreenToTile(float screenX, float screenY, int tileWidth, int tileHeight) {
    return (Vector2){ floorf(screenX / tileWidth), floorf(screenY / tileHeight) };
}

// ---------------------------------------------------------------------------
// Walkability check
// ---------------------------------------------------------------------------
bool IsWalkable(const Game* game, int x, int y) {
    if (!game->map) return false;
    if (x < 0 || x >= game->map->width || y < 0 || y >= game->map->height) return false;
    
    // Check blocking map (typically layer 0)
    if (game->blocking[y][x]) return false;
    
    // Check if any enemy occupies the tile (don't walk through enemies)
    for (int i = 0; i < game->enemyCount; i++) {
        if (game->enemies[i].alive && game->enemies[i].x == x && game->enemies[i].y == y) {
            return false;
        }
    }
    
    return true;
}

// ---------------------------------------------------------------------------
// Find entity at a tile position
// ---------------------------------------------------------------------------
Entity* GetEntityAt(Game* game, int x, int y, Entity* exclude) {
    if (!exclude || !exclude->isPlayer) {
        // Check player
        if (game->player.alive && game->player.x == x && game->player.y == y) {
            return &game->player;
        }
    }
    
    for (int i = 0; i < game->enemyCount; i++) {
        if (&game->enemies[i] == exclude) continue;
        if (game->enemies[i].alive && game->enemies[i].x == x && game->enemies[i].y == y) {
            return &game->enemies[i];
        }
    }
    
    return NULL;
}

// ---------------------------------------------------------------------------
// Move an entity in a direction
// Returns true if moved, false if blocked/attacked
// ---------------------------------------------------------------------------
bool MoveEntity(Game* game, Entity* entity, Direction dir) {
    if (!entity->alive) return false;
    
    // Update facing direction based on horizontal movement
    if (dir == DIR_LEFT) entity->facingRight = false;
    else if (dir == DIR_RIGHT) entity->facingRight = true;
    
    int newX = entity->x;
    int newY = entity->y;
    
    switch (dir) {
        case DIR_UP:    newY--; break;
        case DIR_DOWN:  newY++; break;
        case DIR_LEFT:  newX--; break;
        case DIR_RIGHT: newX++; break;
        default: return false;
    }
    
    // Bounds check
    if (newX < 0 || newX >= game->map->width || newY < 0 || newY >= game->map->height) return false;
    
    // Check if there's an entity at the target
    Entity* target = GetEntityAt(game, newX, newY, entity);
    if (target) {
        // Attack!
        int damage = entity->attack - target->defense;
        if (damage < 1) damage = 1;
        
        target->hp -= damage;
        TraceLog(LOG_INFO, "%s attacks %s for %d damage! (HP: %d/%d)",
                 entity->name, target->name, damage, target->hp, target->maxHp);
        
        if (target->hp <= 0) {
            target->alive = false;
            target->hp = 0;
            TraceLog(LOG_INFO, "%s has been slain!", target->name);
            
            // Player gains EXP from kills
            if (entity->isPlayer && !target->isPlayer) {
                entity->expValue += target->expValue;
            }
        }
        
        return true; // Action was taken (attack)
    }
    
    // Check blocking
    if (game->blocking[newY][newX]) return false;
    
    // Move entity
    entity->prevX = entity->x;
    entity->prevY = entity->y;
    entity->x = newX;
    entity->y = newY;
    
    // Check for healing pickup (only for player)
    if (entity->isPlayer) {
        for (int h = 0; h < game->healingCount; h++) {
            if (!game->healingCollected[h] && game->healingTiles[h][0] == newX && game->healingTiles[h][1] == newY) {
                game->healingCollected[h] = true;
                int healAmt = 8;
                game->player.hp += healAmt;
                if (game->player.hp > game->player.maxHp) game->player.hp = game->player.maxHp;
                TraceLog(LOG_INFO, "Picked up healing! HP: %d/%d", game->player.hp, game->player.maxHp);
            }
        }
    }
    
    return true;
}

// ---------------------------------------------------------------------------
// Bresenham line-of-sight
// ---------------------------------------------------------------------------
bool HasLineOfSight(const Game* game, int x0, int y0, int x1, int y1, int maxDist) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int dist = (dx > dy) ? dx : dy;
    
    if (dist > maxDist) return false;
    
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    
    int x = x0, y = y0;
    
    while (x != x1 || y != y1) {
        // Check if current tile blocks LOS
        if ((x != x0 || y != y0) && game->blocking[y][x]) return false;
        
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x += sx; }
        if (e2 < dx) { err += dx; y += sy; }
    }
    
    return true;
}

// ---------------------------------------------------------------------------
// Simple recursive shadowcasting FOV
// Based on the recursive shadowcasting algorithm
// ---------------------------------------------------------------------------
static void CastLight(Game* game, int cx, int cy, int radius, int row, float startSlope, float endSlope, int xx, int xy, int yx, int yy) {
    if (startSlope < endSlope) return;
    
    for (int j = row; j <= radius; j++) {
        bool blocked = false;
        
        for (int i = -j; i <= 0; i++) {
            int x = cx + i * xx + j * xy;
            int y = cy + i * yx + j * yy;
            
            float leftSlope = (float)(i - 0.5f) / (float)(j + 0.5f);
            float rightSlope = (float)(i + 0.5f) / (float)(j - 0.5f);
            
            if (x < 0 || x >= game->map->width || y < 0 || y >= game->map->height) continue;
            if ((float)(i * i + j * j) > (float)(radius * radius)) continue;
            
            if (startSlope < rightSlope) continue;
            if (endSlope > leftSlope) break;
            
            // Mark as visible
            if (game->visibility[y][x] == 0) {
                game->visibility[y][x] = 1; // Visible
            }
            
            if (blocked) {
                if (game->blocking[y][x]) {
                    blocked = true;
                } else {
                    blocked = false;
                    startSlope = rightSlope;
                }
            } else if (game->blocking[y][x] && j < radius) {
                blocked = true;
                CastLight(game, cx, cy, radius, j + 1, startSlope, leftSlope, xx, xy, yx, yy);
                startSlope = rightSlope;
            }
        }
        
        if (blocked) break;
    }
}

void ComputeFOV(Game* game, int px, int py, int radius) {
    if (!game) return;
    
    // Reset visibility: set previously visible to explored
    for (int y = 0; y < game->map->height; y++) {
        for (int x = 0; x < game->map->width; x++) {
            if (game->visibility[y][x] == 1) {
                game->visibility[y][x] = 2; // Explored but not currently visible
            }
        }
    }
    
    // Player's own tile is always visible
    if (px >= 0 && px < game->map->width && py >= 0 && py < game->map->height) {
        game->visibility[py][px] = 1;
    }
    
    // Cast rays in 8 octants
    int cx = px, cy = py;
    
    CastLight(game, cx, cy, radius, 1, 1.0f, 0.0f, 1, 0, 0, 1);
    CastLight(game, cx, cy, radius, 1, 1.0f, 0.0f, 0, 1, 1, 0);
    CastLight(game, cx, cy, radius, 1, 1.0f, 0.0f, -1, 0, 0, 1);
    CastLight(game, cx, cy, radius, 1, 1.0f, 0.0f, 0, -1, 1, 0);
    CastLight(game, cx, cy, radius, 1, 1.0f, 0.0f, -1, 0, 0, -1);
    CastLight(game, cx, cy, radius, 1, 1.0f, 0.0f, 0, -1, -1, 0);
    CastLight(game, cx, cy, radius, 1, 1.0f, 0.0f, 1, 0, 0, -1);
    CastLight(game, cx, cy, radius, 1, 1.0f, 0.0f, 0, 1, -1, 0);
}

// ---------------------------------------------------------------------------
// Enemy AI
// ---------------------------------------------------------------------------
void ProcessEnemyAI(Game* game, Entity* enemy) {
    if (!enemy->alive) return;
    
    // Calculate distance to player
    int dx = game->player.x - enemy->x;
    int dy = game->player.y - enemy->y;
    int dist = abs(dx) + abs(dy); // Manhattan distance
    
    // If player is within detection range and has LOS
    if (dist <= 8 && HasLineOfSight(game, enemy->x, enemy->y, game->player.x, game->player.y, 8)) {
        // Move towards player (simple greedy pathfinding)
        Direction bestDir = DIR_NONE;
        int bestDist = dist;
        
        // Try each direction
        Direction dirs[] = { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT };
        for (int i = 0; i < 4; i++) {
            int tx = enemy->x, ty = enemy->y;
            switch (dirs[i]) {
                case DIR_UP:    ty--; break;
                case DIR_DOWN:  ty++; break;
                case DIR_LEFT:  tx--; break;
                case DIR_RIGHT: tx++; break;
                default: break;
            }
            
            // Check if we can move there OR if it's the player
            Entity* target = GetEntityAt(game, tx, ty, enemy);
            if (target && target->isPlayer) {
                // Attack player
                MoveEntity(game, enemy, dirs[i]);
                return;
            }
            
            if (IsWalkable(game, tx, ty)) {
                int newDist = abs(game->player.x - tx) + abs(game->player.y - ty);
                if (newDist < bestDist) {
                    bestDist = newDist;
                    bestDir = dirs[i];
                }
            }
        }
        
        if (bestDir != DIR_NONE) {
            MoveEntity(game, enemy, bestDir);
        }
    } else if (dist <= 12) {
        // Random wandering when player is in range but no LOS
        Direction dirs[] = { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT };
        Direction dir = dirs[GetRandomValue(0, 3)];
        MoveEntity(game, enemy, dir);
    }
}

// ---------------------------------------------------------------------------
// Handle player input
// ---------------------------------------------------------------------------
void HandleInput(Game* game) {
    if (game->state != STATE_PLAYER_TURN) return;
    
    Direction dir = DIR_NONE;
    
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) dir = DIR_UP;
    else if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) dir = DIR_DOWN;
    else if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) dir = DIR_LEFT;
    else if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) dir = DIR_RIGHT;
    else if (IsKeyPressed(KEY_PERIOD) || IsKeyPressed(KEY_SPACE)) {
        // Wait a turn
        game->state = STATE_ENEMY_TURN;
        return;
    }
    
    if (dir != DIR_NONE) {
        bool moved = MoveEntity(game, &game->player, dir);
        if (moved) {
            game->turnCount++;
            game->state = STATE_ENEMY_TURN;
        }
    }
}

// ---------------------------------------------------------------------------
// Update game (process enemy turns)
// ---------------------------------------------------------------------------
static void UpdateAnimations(Game* game) {
    if (!game) return;
    
    // Advance animation frame every 0.5 seconds (500ms per frame)
    game->animTimer += GetFrameTime();
    if (game->animTimer >= 0.5f) {
        game->animTimer -= 0.5f;
        for (int i = 0; i < game->enemyCount; i++) {
            if (game->enemies[i].alive) {
                game->enemies[i].animFrame = (game->enemies[i].animFrame + 1) % 4;
            }
        }
    }
}

void UpdateGame(Game* game) {
    if (!game) return;
    
    // Update time-based animations every frame
    UpdateAnimations(game);
    
    if (game->state == STATE_GAME_OVER || game->state == STATE_WIN) return;
    
    if (game->state == STATE_ENEMY_TURN) {
        // Process all enemies
        for (int i = 0; i < game->enemyCount; i++) {
            ProcessEnemyAI(game, &game->enemies[i]);
        }
        
        // Check if player died
        if (!game->player.alive) {
            game->state = STATE_GAME_OVER;
            return;
        }
        
        // Check if all enemies are dead (win condition)
        bool allDead = true;
        for (int i = 0; i < game->enemyCount; i++) {
            if (game->enemies[i].alive) {
                allDead = false;
                break;
            }
        }
        if (allDead) {
            game->state = STATE_WIN;
            return;
        }
        
        // Switch back to player turn
        game->state = STATE_PLAYER_TURN;
    }
}

// ---------------------------------------------------------------------------
// Draw a single tile
// ---------------------------------------------------------------------------
void DrawTile(const Game* game, int x, int y, int layerIndex) {
    if (!game->map || !game->map->layers) return;
    
    int gid = GetTileGID(game->map, layerIndex, x, y);
    if (gid <= 0) return;
    
    Rectangle src = GetTileSourceRect(game->map, gid);
    if (src.width <= 0 || src.height <= 0) return;
    
    Vector2 pos = TileToScreen(x, y, game->map->tileWidth, game->map->tileHeight);
    Rectangle dest = { pos.x, pos.y, (float)game->map->tileWidth, (float)game->map->tileHeight };
    
    DrawTexturePro(game->tilesetTexture, src, dest, (Vector2){0, 0}, 0, WHITE);
}

// ---------------------------------------------------------------------------
// Render the game
// ---------------------------------------------------------------------------
void RenderGame(const Game* game) {
    if (!game || !game->map) return;
    
    BeginMode2D(game->camera);
    
    // Draw tile layers
    for (int layer = 0; layer < game->map->layerCount; layer++) {
        if (!game->map->layers[layer].visible) continue;
        
        for (int y = 0; y < game->map->height; y++) {
            for (int x = 0; x < game->map->width; x++) {
                unsigned char vis = game->visibility[y][x];
                
                // Skip tiles that have never been seen
                if (vis == 0) continue;
                
                // Draw tile
                DrawTile(game, x, y, layer);
                
                // Apply darkness for explored but not visible tiles
                if (vis == 2) {
                    Vector2 pos = TileToScreen(x, y, game->map->tileWidth, game->map->tileHeight);
                    DrawRectangle(pos.x, pos.y, game->map->tileWidth, game->map->tileHeight, (Color){ 0, 0, 0, 180 });
                }
            }
        }
    }
    
    // Draw healing items (only if not collected)
    for (int i = 0; i < game->healingCount; i++) {
        if (game->healingCollected[i]) continue;
        int hx = game->healingTiles[i][0];
        int hy = game->healingTiles[i][1];
        if (game->visibility[hy][hx] != 1) continue;
        
        Vector2 hpos = TileToScreen(hx, hy, game->map->tileWidth, game->map->tileHeight);
        int htw = game->map->tileWidth;
        int hth = game->map->tileHeight;
        int hpad = htw / 4;
        
        // Draw green cross/plus sign for healing
        Color healColor = (Color){ 0, 220, 80, 255 };
        // Vertical bar
        DrawRectangle(hpos.x + htw/2 - 2, hpos.y + hpad, 4, hth - 2*hpad, healColor);
        // Horizontal bar
        DrawRectangle(hpos.x + hpad, hpos.y + hth/2 - 2, htw - 2*hpad, 4, healColor);
    }
    
    // Draw enemies (only if visible)
    for (int i = 0; i < game->enemyCount; i++) {
        Entity* e = &game->enemies[i];
        if (!e->alive) continue;
        if (game->visibility[e->y][e->x] != 1) continue; // Only visible enemies
        
        Vector2 pos = TileToScreen(e->x, e->y, game->map->tileWidth, game->map->tileHeight);
        int tw = game->map->tileWidth;
        int th = game->map->tileHeight;
        
        // Draw using sprite sheet if available, otherwise placeholder
        if (game->entitySprite.id > 0 && e->type == ENTITY_OGRE) {
            // Calculate frame dimensions from texture (4 frames horizontally)
            float frameW = (float)game->entitySprite.width / 4.0f;
            float frameH = (float)game->entitySprite.height;
            
            Rectangle src = {
                (float)(e->animFrame) * frameW, 0,  // frame 0-3
                frameW, frameH
            };
            
            // Draw sprite at tile position, matching pixel for pixel (no scaling)
            Rectangle dest = { pos.x, pos.y, frameW, frameH };
            DrawTexturePro(game->entitySprite, src, dest, (Vector2){0, 0}, 0, WHITE);
        } else {
            // Placeholder rectangle for enemies without sprites
            int pad = tw / 6;
            DrawRectangle(pos.x + pad, pos.y + pad, tw - 2 * pad, th - 2 * pad, e->color);
            int centerX = pos.x + tw / 2;
            int centerY = pos.y + th / 2;
            Color eyeColor = (Color){ 255, 0, 0, 255 };
            DrawCircle(centerX - 3, centerY - 2, 2, eyeColor);
            DrawCircle(centerX + 3, centerY - 2, 2, eyeColor);
        }
    }
    
    // Draw player (always visible)
    if (game->player.alive) {
        Vector2 pos = TileToScreen(game->player.x, game->player.y, game->map->tileWidth, game->map->tileHeight);
        int tw = game->map->tileWidth;
        int th = game->map->tileHeight;
        int pad = tw / 6;
        
        // Draw player body
        DrawRectangle(pos.x + pad, pos.y + pad, tw - 2 * pad, th - 2 * pad, game->player.color);
        
        // Draw face
        int centerX = pos.x + tw / 2;
        int centerY = pos.y + th / 2;
        DrawCircle(centerX - 3, centerY - 2, 2, WHITE);
        DrawCircle(centerX + 3, centerY - 2, 2, WHITE);
    }
    
    EndMode2D();
    
    // Draw HUD (screen space)
    char turnText[64];
    snprintf(turnText, sizeof(turnText), "Turn: %d", game->turnCount);
    DrawText(turnText, 10, 10, 20, WHITE);
    
    char hpText[64];
    snprintf(hpText, sizeof(hpText), "HP: %d/%d", game->player.hp, game->player.maxHp);
    DrawText(hpText, 10, 35, 20, WHITE);
    
    // Count living enemies
    int aliveEnemies = 0;
    for (int i = 0; i < game->enemyCount; i++) {
        if (game->enemies[i].alive) aliveEnemies++;
    }
    char enemyText[64];
    snprintf(enemyText, sizeof(enemyText), "Enemies: %d", aliveEnemies);
    DrawText(enemyText, 10, 60, 20, WHITE);
    
    const char* stateText = "";
    if (game->state == STATE_GAME_OVER) stateText = "GAME OVER - Press R to restart";
    else if (game->state == STATE_WIN) stateText = "YOU WIN! - Press R to restart";
    else if (game->state == STATE_PLAYER_TURN) stateText = "Your turn";
    else if (game->state == STATE_ENEMY_TURN) stateText = "Enemy turn...";
    
    if (stateText[0]) {
        int textWidth = MeasureText(stateText, 20);
        DrawText(stateText, (GetScreenWidth() - textWidth) / 2, GetScreenHeight() - 40, 20, YELLOW);
    }
}

// ---------------------------------------------------------------------------
// Build blocking map from TMX layers
// ---------------------------------------------------------------------------
static void BuildBlockingMap(Game* game) {
    if (!game || !game->map) return;
    
    int w = game->map->width;
    int h = game->map->height;
    
    // Find the collision layer by name
    int collisionLayer = -1;
    for (int i = 0; i < game->map->layerCount; i++) {
        if (strcmp(game->map->layers[i].name, "collision") == 0 ||
            strcmp(game->map->layers[i].name, "Collision") == 0) {
            collisionLayer = i;
            break;
        }
    }
    
    // Default blocking: anything in layer 1 with a GID > 0 is blocked
    // (layer 0 = floor tiles, layer 1 = wall tiles)
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            if (collisionLayer >= 0) {
                // Use explicit collision layer
                game->blocking[y][x] = (GetTileGID(game->map, collisionLayer, x, y) > 0) ? 1 : 0;
            } else if (game->map->layerCount >= 2) {
                // Auto-detect: layer 1 tiles are walls/blocking
                game->blocking[y][x] = (GetTileGID(game->map, 1, x, y) > 0) ? 1 : 0;
            } else {
                // Only one layer: treat all tiles as walkable
                game->blocking[y][x] = 0;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Find objects by type and spawn entities
// ---------------------------------------------------------------------------
static void SpawnEntitiesFromObjects(Game* game) {
    if (!game || !game->map) return;
    
    for (int i = 0; i < game->map->objectCount; i++) {
        MapObject* obj = &game->map->objects[i];
        
        // Convert pixel position to tile position
        int tileX = (int)(obj->x / game->map->tileWidth);
        int tileY = (int)(obj->y / game->map->tileHeight);
        
        if (strcmp(obj->type, "player") == 0 || strcmp(obj->name, "player") == 0 ||
            strcmp(obj->type, "Player") == 0) {
            // Set player position
            game->player.x = tileX;
            game->player.y = tileY;
            game->player.prevX = tileX;
            game->player.prevY = tileY;
            strcpy(game->player.name, "Hero");
            game->player.isPlayer = true;
            game->player.hp = 20;
            game->player.maxHp = 20;
            game->player.attack = 5;
            game->player.defense = 1;
            game->player.level = 1;
            game->player.alive = true;
            game->player.color = GetEntityColor(ENTITY_PLAYER);
            TraceLog(LOG_INFO, "Player spawned at (%d, %d)", tileX, tileY);
        }
        else if (strcmp(obj->type, "healing") == 0 || strcmp(obj->type, "Healing") == 0 || 
                 strcmp(obj->type, "health") == 0 || strcmp(obj->type, "Health") == 0) {
            if (game->healingCount >= MAX_HEALING) continue;
            game->healingTiles[game->healingCount][0] = tileX;
            game->healingTiles[game->healingCount][1] = tileY;
            game->healingCollected[game->healingCount] = false;
            game->healingCount++;
            TraceLog(LOG_INFO, "Healing item at (%d, %d)", tileX, tileY);
        }
        else if (strcmp(obj->type, "goblin") == 0 || strcmp(obj->type, "Goblin") == 0) {
            if (game->enemyCount >= MAX_ENEMIES) continue;
            Entity* e = &game->enemies[game->enemyCount++];
            memset(e, 0, sizeof(Entity));
            e->type = ENTITY_GOBLIN;
            e->x = tileX;
            e->y = tileY;
            e->prevX = tileX;
            e->prevY = tileY;
            strcpy(e->name, "Goblin");
            e->hp = 6;
            e->maxHp = 6;
            e->attack = 3;
            e->defense = 0;
            e->level = 1;
            e->expValue = 5;
            e->alive = true;
            e->color = GetEntityColor(ENTITY_GOBLIN);
            TraceLog(LOG_INFO, "Goblin spawned at (%d, %d)", tileX, tileY);
        }
        else if (strcmp(obj->type, "skeleton") == 0 || strcmp(obj->type, "Skeleton") == 0) {
            if (game->enemyCount >= MAX_ENEMIES) continue;
            Entity* e = &game->enemies[game->enemyCount++];
            memset(e, 0, sizeof(Entity));
            e->type = ENTITY_SKELETON;
            e->x = tileX;
            e->y = tileY;
            e->prevX = tileX;
            e->prevY = tileY;
            strcpy(e->name, "Skeleton");
            e->hp = 10;
            e->maxHp = 10;
            e->attack = 4;
            e->defense = 1;
            e->level = 2;
            e->expValue = 10;
            e->alive = true;
            e->color = GetEntityColor(ENTITY_SKELETON);
            TraceLog(LOG_INFO, "Skeleton spawned at (%d, %d)", tileX, tileY);
        }
        else if (strcmp(obj->type, "ogre") == 0 || strcmp(obj->type, "Ogre") == 0) {
            if (game->enemyCount >= MAX_ENEMIES) continue;
            Entity* e = &game->enemies[game->enemyCount++];
            memset(e, 0, sizeof(Entity));
            e->type = ENTITY_OGRE;
            e->x = tileX;
            e->y = tileY;
            e->prevX = tileX;
            e->prevY = tileY;
            strcpy(e->name, "Ogre");
            e->hp = 20;
            e->maxHp = 20;
            e->attack = 4;
            e->defense = 1;
            e->level = 3;
            e->expValue = 25;
            e->alive = true;
            e->facingRight = true;
            e->color = GetEntityColor(ENTITY_OGRE);
            TraceLog(LOG_INFO, "Ogre spawned at (%d, %d)", tileX, tileY);
        }
        else if (strcmp(obj->type, "orc") == 0 || strcmp(obj->type, "Orc") == 0) {
            if (game->enemyCount >= MAX_ENEMIES) continue;
            Entity* e = &game->enemies[game->enemyCount++];
            memset(e, 0, sizeof(Entity));
            e->type = ENTITY_ORC;
            e->x = tileX;
            e->y = tileY;
            e->prevX = tileX;
            e->prevY = tileY;
            strcpy(e->name, "Orc");
            e->hp = 15;
            e->maxHp = 15;
            e->attack = 6;
            e->defense = 2;
            e->level = 3;
            e->expValue = 20;
            e->alive = true;
            e->color = GetEntityColor(ENTITY_ORC);
            TraceLog(LOG_INFO, "Orc spawned at (%d, %d)", tileX, tileY);
        }
    }
}

// ---------------------------------------------------------------------------
// Initialize game
// ---------------------------------------------------------------------------
bool InitGame(Game* game, const char* tmxFile) {
    if (!game) return false;
    memset(game, 0, sizeof(Game));
    
    // Load TMX map
    game->map = LoadTMX(tmxFile);
    if (!game->map) {
        TraceLog(LOG_ERROR, "Failed to load TMX map: %s", tmxFile);
        return false;
    }
    
    // Build blocking map
    BuildBlockingMap(game);
    
    // Try to load tileset texture
    if (game->map->tilesetCount > 0) {
        // Build path to image relative to the map file
        char imgPath[512];
        // First, extract directory from tmxFile
        const char* lastSlash = strrchr(tmxFile, '/');
        const char* lastBackslash = strrchr(tmxFile, '\\');
        const char* sep = (lastSlash > lastBackslash) ? lastSlash : lastBackslash;
        
        if (sep) {
            int dirLen = (int)(sep - tmxFile) + 1;
            snprintf(imgPath, sizeof(imgPath), "%.*s%s", dirLen, tmxFile, game->map->tilesets[0].imageSource);
        } else {
            snprintf(imgPath, sizeof(imgPath), "%s", game->map->tilesets[0].imageSource);
        }
        
        TraceLog(LOG_INFO, "Loading tileset image: %s", imgPath);
        game->tilesetTexture = LoadTexture(imgPath);
        
        if (game->tilesetTexture.id == 0) {
            TraceLog(LOG_WARNING, "Could not load tileset texture: %s", imgPath);
            // Create a placeholder texture
            Image img = GenImageColor(game->map->tileWidth * 8, game->map->tileHeight * 8, (Color){ 100, 100, 100, 255 });
            // Draw grid lines
            for (int x = 0; x < 8; x++) {
                for (int y = 0; y < 8; y++) {
                    Color c = ((x + y) % 2 == 0) ? (Color){ 120, 120, 120, 255 } : (Color){ 80, 80, 80, 255 };
                    ImageDrawRectangle(&img, x * game->map->tileWidth, y * game->map->tileHeight, 
                                       game->map->tileWidth - 1, game->map->tileHeight - 1, c);
                }
            }
            game->tilesetTexture = LoadTextureFromImage(img);
            UnloadImage(img);
            
            // Update tileset info to match placeholder
            game->map->tilesets[0].columns = 8;
            game->map->tilesets[0].imageWidth = game->map->tileWidth * 8;
            game->map->tilesets[0].imageHeight = game->map->tileHeight * 8;
        }
    }
    
    // Initialize player (default position if no object found)
    game->player.x = 1;
    game->player.y = 1;
    game->player.prevX = 1;
    game->player.prevY = 1;
    strcpy(game->player.name, "Hero");
    game->player.isPlayer = true;
    game->player.hp = 20;
    game->player.maxHp = 20;
    game->player.attack = 5;
    game->player.defense = 1;
    game->player.level = 1;
    game->player.alive = true;
    game->player.color = GetEntityColor(ENTITY_PLAYER);
    
    // Spawn entities from map objects (overrides default player position)
    SpawnEntitiesFromObjects(game);
    
    // Load entity sprite sheet
    game->entitySprite = LoadTexture("resources/sprite_animations/idle/Ogre.png");
    if (game->entitySprite.id == 0) {
        TraceLog(LOG_WARNING, "Could not load entity sprite sheet: resources/sprite_animations/idle/Ogre.png");
    } else {
        TraceLog(LOG_INFO, "Loaded entity sprite sheet: %dx%d", game->entitySprite.width, game->entitySprite.height);
    }
    
    // Initialize state
    game->state = STATE_PLAYER_TURN;
    game->turnCount = 0;
    
    // Make the entire map visible (no fog of war)
    for (int y = 0; y < game->map->height; y++) {
        for (int x = 0; x < game->map->width; x++) {
            game->visibility[y][x] = 1;
        }
    }
    
    // Setup camera
    game->camera.target = (Vector2){ 
        game->player.x * game->map->tileWidth + game->map->tileWidth / 2,
        game->player.y * game->map->tileHeight + game->map->tileHeight / 2
    };
    game->camera.offset = (Vector2){ GetScreenWidth() / 2, GetScreenHeight() / 2 };
    game->camera.rotation = 0;
    game->camera.zoom = 4.0f;
    
    return true;
}

// ---------------------------------------------------------------------------
// Cleanup game
// ---------------------------------------------------------------------------
void CleanupGame(Game* game) {
    if (!game) return;
    
    if (game->tilesetTexture.id > 0) {
        UnloadTexture(game->tilesetTexture);
    }
    
    if (game->entitySprite.id > 0) {
        UnloadTexture(game->entitySprite);
    }
    
    if (game->map) {
        UnloadTMX(game->map);
        game->map = NULL;
    }
    
    memset(game, 0, sizeof(Game));
}