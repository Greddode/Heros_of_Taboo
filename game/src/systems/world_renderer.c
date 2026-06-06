#include "systems/world_renderer.h"
#include "game.h"
#include "systems/render_system.h"
#include "inventory.h"
#include "resources.h"
#include "map/map_helpers.h"
#include "game_balance.h"
#include "atlas.h"
#include <stdio.h>
#include <math.h>

void WorldRenderer_Render(GameWorld* game) {
    GameWorld_RefreshVisibleMonsters(game);

    int tw = game->map->tileWidth;
    int th = game->map->tileHeight;

    RenderSystem_DrawMap(game);

    // Pickups (ECS)
    {
        bool renderedEquipTiles[MAP_HEIGHT][MAP_WIDTH] = { false };

        for (EntityId eid = 1; eid < (EntityId)game->ecs.count; eid++) {
            if (!game->ecs.alive[eid]) continue;
            if (!World_HasComponents(&game->ecs, eid, COMP_POSITION | COMP_PICKUP)) continue;

            CPosition* pos = World_GetPosition(&game->ecs, eid);
            CPickup* pk = World_GetPickup(&game->ecs, eid);
            if (pk->quantity <= 0) continue;
            if (pos->y < 0 || pos->y >= game->map->height || pos->x < 0 || pos->x >= game->map->width) continue;
            if (game->visibility[pos->y][pos->x] != 1) continue;

            if (!pk->isEquip) {
                const AtlasEntry* ae = Atlas_GetItem(pk->itemType);
                if (ae && ae->texture && ae->texture->id > 0) {
                    Vector2 hpos = TileToScreen(pos->x, pos->y, tw, th);
                    Rectangle src = { (float)ae->x, (float)ae->y, (float)ae->w, (float)ae->h };
                    Rectangle dest = { hpos.x, hpos.y, (float)tw, (float)th };
                    DrawTexturePro(*ae->texture, src, dest, (Vector2){ 0, 0 }, 0, WHITE);
                    if (pk->quantity > 1) {
                        char qty[8];
                        snprintf(qty, sizeof(qty), "x%d", pk->quantity);
                        DrawText(qty, hpos.x + 2, hpos.y + 2, 10, YELLOW);
                    }
                } else {
                    Texture2D* pTex = Inventory_LoadPotionTexture(pk->itemType);
                    if (pTex && pTex->id > 0) {
                        Vector2 hpos = TileToScreen(pos->x, pos->y, tw, th);
                        Rectangle dest = { hpos.x, hpos.y, (float)tw, (float)th };
                        DrawTexturePro(*pTex,
                                       (Rectangle){ 0, 0, (float)pTex->width, (float)pTex->height },
                                       dest, (Vector2){ 0, 0 }, 0, WHITE);
                        if (pk->quantity > 1) {
                            char qty[8];
                            snprintf(qty, sizeof(qty), "x%d", pk->quantity);
                            DrawText(qty, hpos.x + 2, hpos.y + 2, 10, YELLOW);
                        }
                    }
                }
            } else if (!renderedEquipTiles[pos->y][pos->x]) {
                renderedEquipTiles[pos->y][pos->x] = true;
                int ex = pos->x;
                int ey = pos->y;
                int entityCount = 0;
                int stackQty = 0;
                EquipType singleType = EQUIP_NONE;
                for (EntityId ej = 1; ej < (EntityId)game->ecs.count; ej++) {
                    if (!game->ecs.alive[ej]) continue;
                    if (!World_HasComponents(&game->ecs, ej, COMP_POSITION | COMP_PICKUP)) continue;
                    CPosition* pj = World_GetPosition(&game->ecs, ej);
                    CPickup* pkj = World_GetPickup(&game->ecs, ej);
                    if (pkj->quantity <= 0 || !pkj->isEquip || pj->x != ex || pj->y != ey) continue;
                    entityCount++;
                    stackQty += pkj->quantity;
                    singleType = pkj->equipType;
                }

                Vector2 epos = TileToScreen(ex, ey, tw, th);
                if (entityCount > 1) {
                    Texture2D* lootTex = Resources_LoadTexture("resources/sprites/items/loot.png");
                    if (lootTex && lootTex->id > 0) {
                        Rectangle dest = { epos.x, epos.y, (float)tw, (float)th };
                        DrawTexturePro(*lootTex,
                                       (Rectangle){ 0, 0, (float)lootTex->width, (float)lootTex->height },
                                       dest, (Vector2){ 0, 0 }, 0, WHITE);
                    }
                    char qty[8];
                    snprintf(qty, sizeof(qty), "x%d", entityCount);
                    DrawText(qty, epos.x + 2, epos.y + 2, 10, YELLOW);
                } else {
                    const AtlasEntry* eae = Atlas_GetEquip(singleType);
                    if (eae && eae->texture && eae->texture->id > 0) {
                        Rectangle src = { (float)eae->x, (float)eae->y, (float)eae->w, (float)eae->h };
                        Rectangle dest = { epos.x, epos.y, (float)tw, (float)th };
                        DrawTexturePro(*eae->texture, src, dest, (Vector2){ 0 }, 0, WHITE);
                    } else {
                        Texture2D* tex = Inventory_LoadEquipTexture(singleType);
                        if (tex && tex->id > 0) {
                            Rectangle src = { 0, 0, (float)tex->width, (float)tex->height };
                            Rectangle dest = { epos.x, epos.y, (float)tw, (float)th };
                            DrawTexturePro(*tex, src, dest, (Vector2){ 0 }, 0, WHITE);
                        }
                    }
                    if (stackQty > 1) {
                        char buf[8];
                        snprintf(buf, sizeof(buf), "x%d", stackQty);
                        DrawText(buf, epos.x + 2, epos.y + 2, 10, YELLOW);
                    }
                }
            }
        }
    }

    // Interpolation factors
    float pd = (game->animDuration > 0.0f) ? game->animDuration : MOVE_ANIM_DURATION;
    float md = (game->monsterAnimDuration > 0.0f) ? game->monsterAnimDuration : MOVE_ANIM_DURATION;
    float playerT = (game->animTimer <= 0.0f) ? 1.0f : 1.0f - (game->animTimer / pd);
    float monsterT = (game->monsterAnimTimer <= 0.0f) ? 1.0f : 1.0f - (game->monsterAnimTimer / md);
    RenderSystem_DrawMonsters(game, monsterT);

    // Player rendering
    if (game->playerEntity != ENTITY_NONE) {
        World* w = &(game)->ecs;
        EntityId pe = game->playerEntity;
        const CStats* pstats = World_GetStats(w, pe);
        const CPosition* ppos = World_GetPosition(w, pe);
        bool hasSprite = World_HasComponents(w, pe, COMP_SPRITE_ANIM);
        bool hasColor = World_HasComponents(w, pe, COMP_FALLBACK_COLOR);
        bool hasHitFlash = World_HasComponents(w, pe, COMP_HIT_FLASH);
        float hitFlashTimer = hasHitFlash ? World_GetHitFlash(w, pe)->timer : 0.0f;

        if (pstats->alive) {
            float px = (float)(ppos->prevX * tw) * (1.0f - playerT) + (float)(ppos->x * tw) * playerT;
            float py = (float)(ppos->prevY * th) * (1.0f - playerT) + (float)(ppos->y * th) * playerT;

            if (hasSprite && World_GetSprite(w, pe)->tex && World_GetSprite(w, pe)->tex->id > 0) {
                const CSpriteAnim* sa = World_GetSprite(w, pe);
                int frameCount = 4;
                float frameW = (float)sa->tex->width / (float)frameCount;
                float frameH = (float)sa->tex->height;
                Rectangle src = {
                    (float)(sa->frame * frameW), 0, frameW, frameH
                };
                Rectangle dest = { px, py, (float)tw, (float)th };
                Color tint = (hitFlashTimer > 0.0f) ? (Color){ 255, 255, 255, 200 } : WHITE;
                DrawTexturePro(*sa->tex, src, dest, (Vector2){ 0, 0 }, 0, tint);
            } else {
                int pad = tw / 6;
                Color playerColor = (hitFlashTimer > 0.0f) ? WHITE
                    : (hasColor ? World_GetColor(w, pe)->color : (Color){ 50, 200, 255, 255 });
                DrawRectangle(px + pad, py + pad, tw - 2*pad, th - 2*pad, playerColor);

                int cx = px + tw / 2;
                int cy = py + th / 2;
                DrawCircle(cx - 3, cy - 2, 2, WHITE);
                DrawCircle(cx + 3, cy - 2, 2, WHITE);

                if (ppos->facingDir == DIR_RIGHT) {
                    DrawCircle(cx + 5, cy + 4, 1, WHITE);
                } else if (ppos->facingDir == DIR_LEFT) {
                    DrawCircle(cx - 5, cy + 4, 1, WHITE);
                } else if (ppos->facingDir == DIR_UP) {
                    DrawCircle(cx, cy - 5, 1, WHITE);
                } else {
                    DrawCircle(cx, cy + 5, 1, WHITE);
                }
            }

            // Direction indicator
            if (game->state == STATE_PLAYER_TURN) {
                int dx = ppos->x, dy = ppos->y;
                switch (ppos->facingDir) {
                    case DIR_UP:    dy--; break;
                    case DIR_DOWN:  dy++; break;
                    case DIR_LEFT:  dx--; break;
                    case DIR_RIGHT: dx++; break;
                    default: break;
                }
                if (dx >= 0 && dx < game->map->width && dy >= 0 && dy < game->map->height) {
                    Vector2 dpos = TileToScreen(dx, dy, tw, th);
                    DrawRectangle(dpos.x, dpos.y, tw, th, (Color){ 255, 50, 50, 100 });
                }
            }
        }
    }

    // Shopkeeper rendering
    if (game->shopkeeperEntity != ENTITY_NONE) {
        World* w = &(game)->ecs;
        CPosition* sp = World_GetPosition(w, game->shopkeeperEntity);
        if (sp && game->visibility[sp->y][sp->x] > 0) {
            int sx = sp->x * tw;
            int sy = sp->y * th;
            DrawRectangle(sx + 2, sy + 2, tw - 4, th - 4, (Color){ 255, 215, 0, 255 });
            int labelSize = 12;
            DrawText("Shop", sx + tw / 2 - MeasureText("Shop", labelSize) / 2, sy - labelSize - 2, labelSize, GOLD);
        }
    }

    // Projectile rendering
    if (game->projectile.active && game->projectileDuration > 0.0f) {
        float pt = (game->projectileTimer <= 0.0f) ? 1.0f : 1.0f - (game->projectileTimer / game->projectileDuration);
        float cx = game->projectile.sx + (game->projectile.ex - game->projectile.sx) * pt;
        float cy = game->projectile.sy + (game->projectile.ey - game->projectile.sy) * pt;

        Texture2D* magicTex = Resources_LoadTexture("resources/tilesets/magic_attacks.png");
        if (game->projectile.attackType == ATTACK_MAGIC &&
            magicTex && magicTex->id > 0 && game->projectile.animFrameCount > 0) {
            int x0 = game->projectile.tileSX, y0 = game->projectile.tileSY;
            int x1 = game->projectile.tileEX, y1 = game->projectile.tileEY;
            int mapTw = game->map->tileWidth, mapTh = game->map->tileHeight;
            int totalDist = abs(x1 - x0) + abs(y1 - y0);
            int cols = 10, tileSize = 64;
            int animLen = game->projectile.animFrameCount;
            int bdx = abs(x1 - x0), bdy = abs(y1 - y0);
            int sxDir = (x0 < x1) ? 1 : -1, syDir = (y0 < y1) ? 1 : -1;
            int err = bdx - bdy;
            int x = x0, y = y0;
            while (1) {
                int tx = x, ty = y;
                int d = abs(tx - x0) + abs(ty - y0);
                float wavePos = pt * (totalDist + animLen);
                float waveOffset = wavePos - d;
                if (waveOffset >= 0) {
                    int frame = (int)waveOffset;
                    if (frame >= animLen) frame = animLen - 1;
                    int fIndex = game->projectile.startFrame + frame;
                    Rectangle src = {
                        (float)((fIndex % cols) * tileSize),
                        (float)((fIndex / cols) * tileSize),
                        (float)tileSize, (float)tileSize
                    };
                    if (tx >= 0 && tx < game->map->width && ty >= 0 && ty < game->map->height) {
                        Rectangle dest = { (float)(tx * mapTw), (float)(ty * mapTh), (float)mapTw, (float)mapTh };
                        DrawTexturePro(*magicTex, src, dest, (Vector2){ 0 }, 0, WHITE);
                    }
                }
                if (tx == x1 && ty == y1) break;
                int e2 = 2 * err;
                if (e2 > -bdy) { err -= bdy; x += sxDir; }
                if (e2 < bdx)  { err += bdx; y += syDir; }
            }
        } else if (game->projectile.attackType == ATTACK_THROW &&
                   game->projectile.throwTex && game->projectile.throwTex->id > 0) {
            int mapTw = game->map->tileWidth, mapTh = game->map->tileHeight;
            Rectangle src  = { 0, 0, (float)game->projectile.throwTex->width,
                                      (float)game->projectile.throwTex->height };
            Rectangle dest = { cx, cy, (float)mapTw, (float)mapTh };
            Vector2   origin = { mapTw / 2.0f, mapTh / 2.0f };
            float rotation = pt * 720.0f;
            DrawTexturePro(*game->projectile.throwTex, src, dest, origin, rotation, WHITE);
        } else {
            float sx = game->projectile.sx, sy = game->projectile.sy;
            float dx = cx - sx, dy = cy - sy;
            float len = sqrtf(dx * dx + dy * dy);
            switch (game->projectile.projectileVisual) {
                case PROJ_SPORE:
                    DrawCircle((int)cx, (int)cy, 3, GREEN);
                    break;
                case PROJ_FIREBALL:
                    DrawCircle((int)cx, (int)cy, 4, ORANGE);
                    break;
                case PROJ_ROCK:
                    DrawCircle((int)cx, (int)cy, 5, GRAY);
                    break;
                case PROJ_SHADOW:
                    DrawCircle((int)cx, (int)cy, 3, (Color){ 80, 0, 120, 255 });
                    break;
                default:
                    if (len > 4.0f) {
                        float nx = dx / len, ny = dy / len;
                        float headLen = 12.0f;
                        float shaftEnd = len - headLen;
                        if (shaftEnd < 0) shaftEnd = 0;
                        float sx2 = sx + nx * shaftEnd, sy2 = sy + ny * shaftEnd;
                        DrawLineEx((Vector2){ sx, sy }, (Vector2){ sx2, sy2 }, 3.0f, game->projectile.color);
                        float tipX = sx + nx * len, tipY = sy + ny * len;
                        float perpX = -ny, perpY = nx;
                        float headW = 6.0f;
                        Vector2 v1 = { tipX, tipY };
                        Vector2 v2 = { sx2 + perpX * headW, sy2 + perpY * headW };
                        Vector2 v3 = { sx2 - perpX * headW, sy2 - perpY * headW };
                        DrawTriangle(v1, v2, v3, game->projectile.color);
                    } else {
                        DrawRectangle((int)sx - 2, (int)sy - 2, 4, 4, game->projectile.color);
                    }
                    break;
            }
        }
    }

    // Floating damage numbers (world-space)
    for (int i = 0; i < MAX_DAMAGE_NUMBERS; i++) {
        if (!game->damageNumbers.entries[i].active) continue;
        DamageNumber* dn = &game->damageNumbers.entries[i];
        float alpha = dn->timer / DAMAGE_NUMBER_LIFETIME;
        if (alpha > 1.0f) alpha = 1.0f;
        Color c = dn->color;
        c.a = (unsigned char)(255.0f * alpha);
        int fs = (int)(14.0f * game->camera.zoom / DEFAULT_CAMERA_ZOOM);
        if (fs < 10) fs = 10;
        int tw = MeasureText(dn->text, fs);
        DrawText(dn->text, (int)(dn->pos.x - tw / 2), (int)dn->pos.y, fs, c);
    }

    // Floating status messages (world-space)
    for (int i = 0; i < MAX_FLOAT_MSGS; i++) {
        if (!game->floatMsgs.entries[i].active) continue;
        FloatMsg* fm = &game->floatMsgs.entries[i];
        Color c = fm->color;
        c.a = (unsigned char)(255.0f * fm->alpha);
        int fs = (int)(16.0f * game->camera.zoom / DEFAULT_CAMERA_ZOOM);
        if (fs < 12) fs = 12;
        int tw = MeasureText(fm->text, fs);
        DrawText(fm->text, (int)(fm->worldX - tw / 2), (int)fm->worldY, fs, c);
    }
}
