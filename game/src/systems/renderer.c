#include "game.h"
#include "systems/renderer.h"
#include "systems/world_renderer.h"
#include "systems/hud_renderer.h"
#include "systems/overlay_renderer.h"

void Draw9Slice(Texture2D tex, Rectangle dest, int l, int t, int r, int b) {
    float sw = (float)tex.width, sh = (float)tex.height;
    float x = dest.x, y = dest.y, w = dest.width, h = dest.height;
    float mw = sw - l - r, mh = sh - t - b;
    float dw = w - l - r, dh = h - t - b;

    DrawTexturePro(tex, (Rectangle){0,0,l,t},  (Rectangle){x,y,l,t},  (Vector2){0},0,WHITE);
    DrawTexturePro(tex, (Rectangle){sw-r,0,r,t},(Rectangle){x+w-r,y,r,t},(Vector2){0},0,WHITE);
    DrawTexturePro(tex, (Rectangle){0,sh-b,l,b},(Rectangle){x,y+h-b,l,b},(Vector2){0},0,WHITE);
    DrawTexturePro(tex, (Rectangle){sw-r,sh-b,r,b},(Rectangle){x+w-r,y+h-b,r,b},(Vector2){0},0,WHITE);
    DrawTexturePro(tex, (Rectangle){l,0,mw,t},  (Rectangle){x+l,y,dw,t},  (Vector2){0},0,WHITE);
    DrawTexturePro(tex, (Rectangle){l,sh-b,mw,b},(Rectangle){x+l,y+h-b,dw,b},(Vector2){0},0,WHITE);
    DrawTexturePro(tex, (Rectangle){0,t,l,mh},  (Rectangle){x,y+l,l,dh},  (Vector2){0},0,WHITE);
    DrawTexturePro(tex, (Rectangle){sw-r,t,r,mh},(Rectangle){x+w-r,y+l,r,dh},(Vector2){0},0,WHITE);
    DrawTexturePro(tex, (Rectangle){l,t,mw,mh}, (Rectangle){x+l,y+l,dw,dh}, (Vector2){0},0,WHITE);
}

void RenderGame(GameWorld* game, const InventoryUIState* ui) {
    if (!game || !game->map) return;

    BeginMode2D(game->camera);
    WorldRenderer_Render(game);
    EndMode2D();

    HUDRenderer_Render(game);
    OverlayRenderer_Render(game, ui);
}
