#include "procedural.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Room size constraints
#define MIN_ROOM_W  5
#define MAX_ROOM_W  10
#define MIN_ROOM_H  5
#define MAX_ROOM_H  10
#define MAX_ROOMS   8

// Padding: empty space forced between rooms / halls and other features
#define ROOM_PAD    2
#define HALL_PAD    3
#define HALL_MIN    5

#define MAX_ATTEMPTS 200

// Internal room representation with connection tracking
typedef struct {
    int x, y, w, h;      // Bounding box (tile coordinates)
    int cx, cy;           // Centre tile
    int usedDirs;         // Bitmask: which directions have sprouted branches
} Room;

// Copy of the generated rooms exposed via GetGeneratedRooms() for the spawner
static ProceduralRoom s_generatedRooms[MAX_GENERATED_ROOMS];
static int s_generatedRoomCount = 0;

// Stair tile position on the current generated map
static int s_stairX = -1;
static int s_stairY = -1;

int GetStairX(void) { return s_stairX; }
int GetStairY(void) { return s_stairY; }

// Returns 1 if tile (x, y) is still void (un-carved)
static int TileFree(int* data, int mapW, int mapH, int x, int y) {
    if (x < 0 || x >= mapW || y < 0 || y >= mapH) return 0;
    return data[y * mapW + x] == 0;
}

// Returns 1 if the entire rectangle (rx,ry)-(rx+rw-1,ry+rh-1) is void
static int RectFree(int* data, int mapW, int mapH, int rx, int ry, int rw, int rh) {
    for (int y = ry; y < ry + rh; y++)
        for (int x = rx; x < rx + rw; x++)
            if (!TileFree(data, mapW, mapH, x, y)) return 0;
    return 1;
}

// ---- Floor helpers ----

int IsFloorGID(int gid) {
    if (gid == TILE_FLOOR) return 1;
    if (gid == TILE_STAIRS) return 1;
    if (gid == TILE_ESCAPE) return 1;
    for (int i = 0; i < FLOOR_VARIANT_COUNT; i++)
        if (FLOOR_VARIANTS[i] == gid) return 1;
    return 0;
}

static int RandomFloorGID(void) {
    if (GetRandomValue(0, 3) != 0) return TILE_FLOOR;
    return FLOOR_VARIANTS[GetRandomValue(0, FLOOR_VARIANT_COUNT - 1)];
}

// Carve a horizontal corridor from x1 to x2 at row y
static void CarveHLine(int* data, int mapW, int mapH, int x1, int x2, int y) {
    if (y < 0 || y >= mapH) return;
    if (x1 > x2) { int t = x1; x1 = x2; x2 = t; }
    for (int x = x1; x <= x2; x++)
        if (x >= 0 && x < mapW) data[y * mapW + x] = RandomFloorGID();
}

// Carve a vertical corridor from y1 to y2 at column x
static void CarveVLine(int* data, int mapW, int mapH, int y1, int y2, int x) {
    if (x < 0 || x >= mapW) return;
    if (y1 > y2) { int t = y1; y1 = y2; y2 = t; }
    for (int y = y1; y <= y2; y++)
        if (y >= 0 && y < mapH) data[y * mapW + x] = RandomFloorGID();
}

// Check that a rectangle is entirely void, ignoring a sub-rectangle to skip
// (allows hall clearance to overlap with the parent room).
static int RectClearSkip(int* data, int mapW, int mapH,
                         int rx, int ry, int rw, int rh,
                         int skipX, int skipY, int skipW, int skipH) {
    for (int y = ry; y < ry + rh; y++) {
        for (int x = rx; x < rx + rw; x++) {
            if (x < 0 || x >= mapW || y < 0 || y >= mapH) return 0;
            if (x >= skipX && x < skipX + skipW && y >= skipY && y < skipY + skipH) continue;
            if (data[y * mapW + x] != 0) return 0;
        }
    }
    return 1;
}

// Sprout a new branch from an existing room.
// The branch runs in primDir (0=N,1=S,2=W,3=E), takes a 90-degree turn
// (0=left,1=right), and places a room at the end.
// Returns 1 on success, 0 if the space was occupied.
static int GrowBranch(int* data, int mapW, int mapH,
                      Room* rooms, int* roomCount,
                      int parentIdx, int primDir, int turnDir) {
    Room* p = &rooms[parentIdx];

    int ex = p->cx, ey = p->cy;
    int stepX = 0, stepY = 0;
    int turnStepX = 0, turnStepY = 0;

    switch (primDir) {
        case 0: // N
            ey = p->y - 1; stepY = -1;
            turnStepX = (turnDir == 0) ? -1 : 1;
            break;
        case 1: // S
            ey = p->y + p->h; stepY = 1;
            turnStepX = (turnDir == 0) ? 1 : -1;
            break;
        case 2: // W
            ex = p->x - 1; stepX = -1;
            turnStepY = (turnDir == 0) ? -1 : 1;
            break;
        default: // E
            ex = p->x + p->w; stepX = 1;
            turnStepY = (turnDir == 0) ? 1 : -1;
            break;
    }

    // Pick room size first so we can calculate required hall clearance
    int rw = GetRandomValue(MIN_ROOM_W, MAX_ROOM_W);
    int rh = GetRandomValue(MIN_ROOM_H, MAX_ROOM_H);

    // Calculate minimum primary segment length so a centered room clears parent by > ROOM_PAD
    int minHall;
    switch (primDir) {
        case 0: minHall = ROOM_PAD + rh/2; break;      // N: parent south
        case 1: minHall = ROOM_PAD + rh/2 + 1; break;  // S: parent north
        case 2: minHall = ROOM_PAD + rw/2; break;      // W: parent east
        default: minHall = ROOM_PAD + rw/2 + 1; break; // E: parent west
    }
    if (minHall < HALL_MIN) minHall = HALL_MIN;
    if (minHall > 12) return 0; // too long, give up

    int hallLen = minHall + GetRandomValue(0, 2);

    int tx = ex + stepX * hallLen;
    int ty = ey + stepY * hallLen;
    if (tx < 1 || tx >= mapW - 1 || ty < 1 || ty >= mapH - 1) return 0;

    int seg2Len = HALL_MIN + 1 + GetRandomValue(0, 4);
    int rx = tx + turnStepX * seg2Len;
    int ry = ty + turnStepY * seg2Len;
    if (rx < 1 || rx >= mapW - 1 || ry < 1 || ry >= mapH - 1) return 0;

    // Position room centered on hall end (rx, ry) so the hall enters
    // the middle of a room wall, not a corner.
    int roomX, roomY;
    if (turnStepX != 0) {
        // Turn is horizontal: room extends left/right from hall end,
        // centered vertically on ry
        if (turnStepX > 0) roomX = rx + 1;
        else roomX = rx - rw;
        roomY = ry - rh / 2;
    } else {
        // Turn is vertical: room extends up/down from hall end,
        // centered horizontally on rx
        if (turnStepY > 0) roomY = ry + 1;
        else roomY = ry - rh;
        roomX = rx - rw / 2;
    }

    if (roomX < 1 || roomX + rw >= mapW - 1 ||
        roomY < 1 || roomY + rh >= mapH - 1) return 0;

    int roomCX = roomX + rw / 2;
    int roomCY = roomY + rh / 2;

    // HALL_PAD clearance check for hall segment 1 (skip parent room)
    {
        int sx = (ex < tx ? ex : tx) - HALL_PAD;
        int sy = (ey < ty ? ey : ty) - HALL_PAD;
        int sw = abs(ex - tx) + 1 + 2 * HALL_PAD;
        int sh = abs(ey - ty) + 1 + 2 * HALL_PAD;
        if (!RectClearSkip(data, mapW, mapH, sx, sy, sw, sh,
                           p->x, p->y, p->w, p->h)) return 0;
    }

    // HALL_PAD clearance check for hall segment 2 (skip parent room)
    {
        int sx = (tx < rx ? tx : rx) - HALL_PAD;
        int sy = (ty < ry ? ty : ry) - HALL_PAD;
        int sw = abs(tx - rx) + 1 + 2 * HALL_PAD;
        int sh = abs(ty - ry) + 1 + 2 * HALL_PAD;
        if (!RectClearSkip(data, mapW, mapH, sx, sy, sw, sh,
                           p->x, p->y, p->w, p->h)) return 0;
    }

    // HALL_PAD clearance check for the room itself (skip parent room)
    if (!RectClearSkip(data, mapW, mapH,
                       roomX - HALL_PAD, roomY - HALL_PAD,
                       rw + 2 * HALL_PAD, rh + 2 * HALL_PAD,
                       p->x, p->y, p->w, p->h)) return 0;

    // Room padding collision with all existing rooms (ROOM_PAD)
    for (int i = 0; i < *roomCount; i++) {
        Room* a = &rooms[i];
        if (a->x - ROOM_PAD < roomX + rw + ROOM_PAD &&
            a->x + a->w + ROOM_PAD > roomX - ROOM_PAD &&
            a->y - ROOM_PAD < roomY + rh + ROOM_PAD &&
            a->y + a->h + ROOM_PAD > roomY - ROOM_PAD) return 0;
    }

    // --- Carve ---
    // Segment 1: primary direction
    if (stepY != 0) CarveVLine(data, mapW, mapH, ey, ty, ex);
    else CarveHLine(data, mapW, mapH, ex, tx, ey);

    // Segment 2: turn direction
    if (turnStepX != 0) CarveHLine(data, mapW, mapH, tx, rx, ty);
    else CarveVLine(data, mapW, mapH, ty, ry, tx);

    // Carve the room
    for (int y = roomY; y < roomY + rh; y++)
        for (int x = roomX; x < roomX + rw; x++)
            data[y * mapW + x] = RandomFloorGID();

    rooms[*roomCount] = (Room){ roomX, roomY, rw, rh, roomCX, roomCY, 0 };
    (*roomCount)++;

    TraceLog(LOG_INFO, "Procedural: Branch room %d dir %d -> room %d at (%d,%d) %dx%d hallLen=%d",
             parentIdx, primDir, *roomCount - 1, roomX, roomY, rw, rh, hallLen);
    return 1;
}

// ---- Wall placement ----

static void PlaceNorthWalls(int* data, int mapW, int mapH) {
    for (int y = 1; y < mapH; y++)
        for (int x = 0; x < mapW; x++)
            if (IsFloorGID(data[y * mapW + x]) && data[(y - 1) * mapW + x] == 0)
                data[(y - 1) * mapW + x] = TILE_WALL_NORTH;
}

static void PlaceWestWalls(int* data, int mapW, int mapH) {
    for (int y = 0; y < mapH; y++)
        for (int x = 1; x < mapW; x++)
            if (IsFloorGID(data[y * mapW + x]) && data[y * mapW + (x - 1)] == 0)
                data[y * mapW + (x - 1)] = TILE_WALL_WEST;
}

static void PlaceEastWalls(int* data, int mapW, int mapH) {
    for (int y = 0; y < mapH; y++)
        for (int x = 0; x < mapW - 1; x++)
            if (IsFloorGID(data[y * mapW + x]) && data[y * mapW + (x + 1)] == 0)
                data[y * mapW + (x + 1)] = TILE_WALL_EAST;
}

static void PlaceSouthWalls(int* data, int mapW, int mapH) {
    for (int y = 0; y < mapH - 1; y++)
        for (int x = 0; x < mapW; x++)
            if (IsFloorGID(data[y * mapW + x]) && data[(y + 1) * mapW + x] == 0)
                data[(y + 1) * mapW + x] = TILE_WALL_SOUTH;
}

static void ClearInsideCorners(int* data, int mapW, int mapH) {
    for (int y = 0; y < mapH; y++) {
        for (int x = 0; x < mapW; x++) {
            int t = data[y * mapW + x];
            if (t == 0 || IsFloorGID(t)) continue;
            if (t == TILE_WALL_CORNER_NW || t == TILE_WALL_CORNER_NE ||
                t == TILE_WALL_CORNER_SW || t == TILE_WALL_CORNER_SE) continue;
            int fc = 0;
            if (y > 0 && IsFloorGID(data[(y - 1) * mapW + x])) fc++;
            if (y < mapH - 1 && IsFloorGID(data[(y + 1) * mapW + x])) fc++;
            if (x > 0 && IsFloorGID(data[y * mapW + (x - 1)])) fc++;
            if (x < mapW - 1 && IsFloorGID(data[y * mapW + (x + 1)])) fc++;
            if (fc >= 2) data[y * mapW + x] = 0;
        }
    }
}

static void PlaceCornersNW(int* data, int mapW, int mapH) {
    for (int y = 0; y < mapH - 1; y++)
        for (int x = 0; x < mapW - 1; x++)
            if (data[y * mapW + x] == 0 &&
                data[y * mapW + (x + 1)] == TILE_WALL_NORTH &&
                data[(y + 1) * mapW + x] == TILE_WALL_WEST)
                data[y * mapW + x] = TILE_WALL_CORNER_NW;
}

static void PlaceCornersNE(int* data, int mapW, int mapH) {
    for (int y = 0; y < mapH - 1; y++)
        for (int x = 1; x < mapW; x++)
            if (data[y * mapW + x] == 0 &&
                data[y * mapW + (x - 1)] == TILE_WALL_NORTH &&
                data[(y + 1) * mapW + x] == TILE_WALL_EAST)
                data[y * mapW + x] = TILE_WALL_CORNER_NE;
}

static void PlaceCornersSW(int* data, int mapW, int mapH) {
    for (int y = 1; y < mapH; y++)
        for (int x = 0; x < mapW - 1; x++)
            if (data[y * mapW + x] == 0 &&
                data[(y - 1) * mapW + x] == TILE_WALL_WEST &&
                data[y * mapW + (x + 1)] == TILE_WALL_SOUTH)
                data[y * mapW + x] = TILE_WALL_CORNER_SW;
}

static void PlaceCornersSE(int* data, int mapW, int mapH) {
    for (int y = 1; y < mapH; y++)
        for (int x = 1; x < mapW; x++)
            if (data[y * mapW + x] == 0 &&
                data[(y - 1) * mapW + x] == TILE_WALL_EAST &&
                data[y * mapW + (x - 1)] == TILE_WALL_SOUTH)
                data[y * mapW + x] = TILE_WALL_CORNER_SE;
}

static void PlaceICornersNE(int* data, int mapW, int mapH) {
    for (int y = 1; y < mapH; y++)
        for (int x = 0; x < mapW - 1; x++)
            if (data[y * mapW + x] == 0 &&
                IsFloorGID(data[(y - 1) * mapW + x]) &&
                IsFloorGID(data[y * mapW + (x + 1)]))
                data[y * mapW + x] = TILE_WALL_ICORNER_NE;
}

static void PlaceICornersNW(int* data, int mapW, int mapH) {
    for (int y = 1; y < mapH; y++)
        for (int x = 1; x < mapW; x++)
            if (data[y * mapW + x] == 0 &&
                IsFloorGID(data[(y - 1) * mapW + x]) &&
                IsFloorGID(data[y * mapW + (x - 1)]))
                data[y * mapW + x] = TILE_WALL_ICORNER_NW;
}

static void PlaceICornersSE(int* data, int mapW, int mapH) {
    for (int y = 0; y < mapH - 1; y++)
        for (int x = 0; x < mapW - 1; x++)
            if (data[y * mapW + x] == 0 &&
                IsFloorGID(data[(y + 1) * mapW + x]) &&
                IsFloorGID(data[y * mapW + (x + 1)]))
                data[y * mapW + x] = TILE_WALL_ICORNER_SE;
}

static void PlaceICornersSW(int* data, int mapW, int mapH) {
    for (int y = 0; y < mapH - 1; y++)
        for (int x = 1; x < mapW; x++)
            if (data[y * mapW + x] == 0 &&
                IsFloorGID(data[(y + 1) * mapW + x]) &&
                IsFloorGID(data[y * mapW + (x - 1)]))
                data[y * mapW + x] = TILE_WALL_ICORNER_SW;
}

// ---- Main entry point: generates a dungeon with branching corridors ----

MapData* GenerateProceduralMap(int width, int height, int generateStairs) {
    MapData* map = (MapData*)calloc(1, sizeof(MapData));
    if (!map) return NULL;

    map->width = width; map->height = height;
    map->tileWidth = 16; map->tileHeight = 16;
    strcpy(map->orientation, "orthogonal");

    TilesetDef* ts = &map->tilesets[0];
    ts->firstGID = 1; strcpy(ts->name, "VelmoraRealms-TileSet");
    ts->tileWidth = 16; ts->tileHeight = 16; ts->tileCount = 700; ts->columns = 28;
    strcpy(ts->imageSource, "tilesets/VelmoraRealms-TileSet.png");
    ts->imageWidth = 448; ts->imageHeight = 400;
    map->tilesetCount = 1;

    int tileCount = width * height;

    TileLayer* layer = &map->layers[0];
    strcpy(layer->name, "ground"); layer->width = width; layer->height = height;
    layer->visible = true;
    layer->data = (int*)calloc(tileCount, sizeof(int));
    if (!layer->data) { free(map); return NULL; }

    TileLayer* collLayer = &map->layers[1];
    strcpy(collLayer->name, "collision"); collLayer->width = width; collLayer->height = height;
    collLayer->visible = false;
    collLayer->data = (int*)calloc(tileCount, sizeof(int));
    if (!collLayer->data) { free(layer->data); free(map); return NULL; }
    map->layerCount = 2;

    // ---- Place the first (spawn) room at the map centre ----
    Room rooms[MAX_ROOMS];
    int roomCount = 0;

    int sw = GetRandomValue(MIN_ROOM_W, MAX_ROOM_W);
    int sh = GetRandomValue(MIN_ROOM_H, MAX_ROOM_H);
    int sx = width / 2 - sw / 2;
    int sy = height / 2 - sh / 2;
    rooms[roomCount++] = (Room){ sx, sy, sw, sh, sx + sw / 2, sy + sh / 2, 0 };

    for (int y = sy; y < sy + sh; y++)
        for (int x = sx; x < sx + sw; x++)
            layer->data[y * width + x] = RandomFloorGID();

    // ---- Grow branches like a plant: each room tries to sprout in unused directions ----
    for (int attempt = 0; attempt < MAX_ATTEMPTS && roomCount < MAX_ROOMS; attempt++) {
        int candidates[MAX_ROOMS];
        int candCount = 0;
        for (int i = 0; i < roomCount; i++)
            if (rooms[i].usedDirs != 15)
                candidates[candCount++] = i;
        if (candCount == 0) break;

        int ri = candidates[GetRandomValue(0, candCount - 1)];
        Room* r = &rooms[ri];

        int avail[4], availCount = 0;
        for (int d = 0; d < 4; d++)
            if (!(r->usedDirs & (1 << d)))
                avail[availCount++] = d;
        for (int i = availCount - 1; i > 0; i--) {
            int j = GetRandomValue(0, i);
            int t = avail[i]; avail[i] = avail[j]; avail[j] = t;
        }

        int grown = 0;
        for (int a = 0; a < availCount && !grown; a++) {
            int primDir = avail[a];
            int turnDir = GetRandomValue(0, 1);
            for (int t = 0; t < 2; t++) {
                int td = (turnDir + t) % 2;
                if (GrowBranch(layer->data, width, height, rooms, &roomCount, ri, primDir, td)) {
                    r->usedDirs |= (1 << primDir);
                    grown = 1;
                    break;
                }
            }
        }
        if (!grown) r->usedDirs |= 15; // mark all used so we don't retry
    }

    TraceLog(LOG_INFO, "Procedural: Placed %d rooms", roomCount);

    // ---- Place directional wall tiles around all carved areas ----
    // Each step adds a specific wall type; multiple passes are needed so
    // that walls register along every edge and corners are resolved correctly.
    PlaceNorthWalls(layer->data, width, height);
    PlaceWestWalls(layer->data, width, height);
    PlaceEastWalls(layer->data, width, height);
    PlaceSouthWalls(layer->data, width, height);
    PlaceCornersNW(layer->data, width, height);
    PlaceCornersNE(layer->data, width, height);
    PlaceCornersSW(layer->data, width, height);
    PlaceCornersSE(layer->data, width, height);
    ClearInsideCorners(layer->data, width, height);
    PlaceICornersNE(layer->data, width, height);
    PlaceICornersNW(layer->data, width, height);
    PlaceICornersSE(layer->data, width, height);
    PlaceICornersSW(layer->data, width, height);

    // Sync the collision layer: anything that is not void and not floor is a wall
    for (int i = 0; i < tileCount; i++)
        collLayer->data[i] = (layer->data[i] != 0 && !IsFloorGID(layer->data[i])) ? 1 : 0;

    // Store room metadata so Spawner_Populate knows where to place entities
    s_generatedRoomCount = roomCount < MAX_GENERATED_ROOMS ? roomCount : MAX_GENERATED_ROOMS;
    for (int i = 0; i < s_generatedRoomCount; i++) {
        s_generatedRooms[i].x  = rooms[i].x;
        s_generatedRooms[i].y  = rooms[i].y;
        s_generatedRooms[i].w  = rooms[i].w;
        s_generatedRooms[i].h  = rooms[i].h;
        s_generatedRooms[i].cx = rooms[i].cx;
        s_generatedRooms[i].cy = rooms[i].cy;
    }

    // Place stairs in a random room (preferring non-spawn rooms)
    s_stairX = -1;
    s_stairY = -1;
    if (generateStairs && roomCount > 0) {
        int stairRoomIdx = 0;
        if (roomCount >= 2) {
            do {
                stairRoomIdx = GetRandomValue(0, roomCount - 1);
            } while (stairRoomIdx == 0);
        }
        Room* sr = &rooms[stairRoomIdx];
        int floorTilesInRoom[100];
        int floorTileCount = 0;
        for (int y = sr->y; y < sr->y + sr->h; y++) {
            for (int x = sr->x; x < sr->x + sr->w; x++) {
                if (IsFloorGID(layer->data[y * width + x])) {
                    floorTilesInRoom[floorTileCount++] = y * width + x;
                }
            }
        }
        if (floorTileCount > 0) {
            int picked = floorTilesInRoom[GetRandomValue(0, floorTileCount - 1)];
            // Avoid placing stairs directly on the player spawn (room-0 centre)
            if (stairRoomIdx == 0 && floorTileCount > 1) {
                int centreIdx = rooms[0].cy * width + rooms[0].cx;
                if (picked == centreIdx) {
                    for (int f = 0; f < floorTileCount; f++) {
                        if (floorTilesInRoom[f] != centreIdx) {
                            picked = floorTilesInRoom[f];
                            break;
                        }
                    }
                }
            }
            layer->data[picked] = TILE_STAIRS;
            s_stairX = picked % width;
            s_stairY = picked / width;
            TraceLog(LOG_INFO, "Procedural: Stairs placed at (%d,%d) in room %d",
                     s_stairX, s_stairY, stairRoomIdx);
        }
    }

    // Place a MapObject for the player at the centre of the first (spawn) room
    MapObject* obj = &map->objects[0];
    strcpy(obj->name, "player");
    strcpy(obj->type, "Player");
    obj->x = (float)(rooms[0].cx * map->tileWidth);
    obj->y = (float)(rooms[0].cy * map->tileHeight);
    map->objectCount = 1;

    TraceLog(LOG_INFO, "Procedural: Generated %dx%d map", width, height);
    return map;
}

int GetGeneratedRooms(ProceduralRoom* outRooms, int maxRooms) {
    int n = s_generatedRoomCount < maxRooms ? s_generatedRoomCount : maxRooms;
    for (int i = 0; i < n; i++) outRooms[i] = s_generatedRooms[i];
    return n;
}
