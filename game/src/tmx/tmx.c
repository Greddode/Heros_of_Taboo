#include "tmx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// ---------------------------------------------------------------------------
// Lightweight TMX (XML) parser
// TMX is the Tiled Map Editor format. Handles orthogonal maps with CSV layers,
// inline/external tilesets, and object groups for entity placement.
// ---------------------------------------------------------------------------

// Case-insensitive substring search
static const char* stristr(const char* haystack, const char* needle) {
    if (!*needle) return haystack;
    for (; *haystack; ++haystack) {
        if (tolower((unsigned char)*haystack) == tolower((unsigned char)*needle)) {
            const char* h = haystack;
            const char* n = needle;
            while (*h && *n && tolower((unsigned char)*h) == tolower((unsigned char)*n)) {
                ++h; ++n;
            }
            if (!*n) return haystack;
        }
    }
    return NULL;
}

// Advance pointer past the first occurrence of a substring
static const char* skipPast(const char* ptr, const char* tag) {
    const char* found = strstr(ptr, tag);
    return found ? found + strlen(tag) : ptr;
}

// Extract an integer attribute from an XML tag
// e.g. getIntAttr(ptr, "width") on '<map width="40">' returns 40
static int getIntAttr(const char* tagStart, const char* attrName) {
    char search[64];
    snprintf(search, sizeof(search), "%s=\"", attrName);
    const char* valStart = stristr(tagStart, search);
    if (!valStart) return 0;
    valStart += strlen(search);
    return atoi(valStart);
}

// Extract a string attribute from an XML tag into a fixed-size buffer
static const char* getStringAttr(const char* tagStart, const char* attrName, char* out, int outLen) {
    char search[64];
    snprintf(search, sizeof(search), "%s=\"", attrName);
    const char* valStart = stristr(tagStart, search);
    if (!valStart) { out[0] = '\0'; return NULL; }
    valStart += strlen(search);
    int i = 0;
    while (*valStart && *valStart != '\"' && i < outLen - 1) {
        out[i++] = *valStart++;
    }
    out[i] = '\0';
    return valStart;
}

// Resolve a relative asset path against the TMX file's directory.
// Returns a malloc'd string; caller must free().
static char* resolvePath(const char* basePath, const char* relativePath) {
    const char* lastSep = NULL;
    const char* p = basePath;
    while (*p) {
        if (*p == '/' || *p == '\\') lastSep = p;
        p++;
    }

    if (lastSep) {
        int dirLen = (int)(lastSep - basePath) + 1;
        char* result = (char*)malloc(dirLen + strlen(relativePath) + 1);
        if (result) {
            snprintf(result, dirLen + strlen(relativePath) + 1, "%.*s%s", dirLen, basePath, relativePath);
        }
        return result;
    } else {
        char* result = (char*)malloc(strlen(relativePath) + 1);
        if (result) strcpy(result, relativePath);
        return result;
    }
}

// ---------------------------------------------------------------------------
// Load a TMX file and parse it into a MapData structure
// ---------------------------------------------------------------------------
MapData* LoadTMX(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        TraceLog(LOG_WARNING, "TMX: Could not open file '%s'", filename);
        return NULL;
    }

    // Read the whole file into memory for string-based parsing
    fseek(f, 0, SEEK_END);
    long fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* xml = (char*)malloc(fileSize + 1);
    if (!xml) { fclose(f); return NULL; }
    fread(xml, 1, fileSize, f);
    xml[fileSize] = '\0';
    fclose(f);

    MapData* map = (MapData*)calloc(1, sizeof(MapData));
    if (!map) { free(xml); return NULL; }

    // -------- <map> tag: dimensions, orientation, tile size --------
    const char* mapTag = stristr(xml, "<map");
    if (!mapTag) {
        TraceLog(LOG_WARNING, "TMX: No <map> tag found");
        free(xml); free(map); return NULL;
    }

    getStringAttr(mapTag, "orientation", map->orientation, sizeof(map->orientation));
    map->width      = getIntAttr(mapTag, "width");
    map->height     = getIntAttr(mapTag, "height");
    map->tileWidth  = getIntAttr(mapTag, "tilewidth");
    map->tileHeight = getIntAttr(mapTag, "tileheight");

    TraceLog(LOG_INFO, "TMX: Loaded map %dx%d tiles, tile size %dx%d, orientation=%s",
             map->width, map->height, map->tileWidth, map->tileHeight, map->orientation);

    // -------- <tileset> tags: inline or external (.tsx) --------
    const char* tsPtr = xml;
    while ((tsPtr = stristr(tsPtr, "<tileset")) != NULL && map->tilesetCount < MAX_TILESETS) {
        TilesetDef* ts = &map->tilesets[map->tilesetCount];
        memset(ts, 0, sizeof(TilesetDef));

        const char* tagClose = strchr(tsPtr, '>');
        if (!tagClose) break;

        ts->firstGID = getIntAttr(tsPtr, "firstgid");

        // External tileset reference (self-closing <tileset source="..." />)
        char sourceAttr[256] = {0};
        getStringAttr(tsPtr, "source", sourceAttr, sizeof(sourceAttr));
        bool isExternal = (*(tagClose - 1) == '/') && (strlen(sourceAttr) > 0);

        // Read basic attributes (may be overridden by .tsx content)
        getStringAttr(tsPtr, "name", ts->name, sizeof(ts->name));
        ts->tileWidth  = getIntAttr(tsPtr, "tilewidth");
        ts->tileHeight = getIntAttr(tsPtr, "tileheight");
        ts->tileCount  = getIntAttr(tsPtr, "tilecount");
        ts->columns    = getIntAttr(tsPtr, "columns");

        if (isExternal) {
            // Load external .tsx tileset definition file
            char* tsxPath = resolvePath(filename, sourceAttr);
            if (tsxPath) {
                TraceLog(LOG_INFO, "TMX: Loading external tileset: %s", tsxPath);

                FILE* tsxF = fopen(tsxPath, "rb");
                if (tsxF) {
                    fseek(tsxF, 0, SEEK_END);
                    long tsxSize = ftell(tsxF);
                    fseek(tsxF, 0, SEEK_SET);

                    char* tsxXml = (char*)malloc(tsxSize + 1);
                    if (tsxXml) {
                        fread(tsxXml, 1, tsxSize, tsxF);
                        tsxXml[tsxSize] = '\0';

                        const char* tsxTsTag = stristr(tsxXml, "<tileset");
                        if (tsxTsTag) {
                            char tsxName[64] = {0};
                            getStringAttr(tsxTsTag, "name", tsxName, sizeof(tsxName));
                            if (strlen(tsxName) > 0) strcpy(ts->name, tsxName);

                            int tw = getIntAttr(tsxTsTag, "tilewidth");
                            int th = getIntAttr(tsxTsTag, "tileheight");
                            int tc = getIntAttr(tsxTsTag, "tilecount");
                            int col = getIntAttr(tsxTsTag, "columns");
                            if (tw > 0) ts->tileWidth = tw;
                            if (th > 0) ts->tileHeight = th;
                            if (tc > 0) ts->tileCount = tc;
                            if (col > 0) ts->columns = col;

                            // Extract the <image> tag inside the .tsx for the sprite sheet path
                            const char* tsxClose = strstr(tsxTsTag, "</tileset>");
                            const char* imgPtr = tsxTsTag;
                            while ((imgPtr = stristr(imgPtr, "<image")) != NULL &&
                                   (!tsxClose || imgPtr < tsxClose)) {
                                getStringAttr(imgPtr, "source", ts->imageSource, sizeof(ts->imageSource));
                                ts->imageWidth  = getIntAttr(imgPtr, "width");
                                ts->imageHeight = getIntAttr(imgPtr, "height");

                                const char* imgEnd = strchr(imgPtr, '>');
                                if (!imgEnd) break;
                                imgPtr = imgEnd + 1;
                                break;
                            }
                        }

                        free(tsxXml);
                    }
                    fclose(tsxF);
                } else {
                    TraceLog(LOG_WARNING, "TMX: Could not open external tileset: %s", tsxPath);
                }

                free(tsxPath);
            }

            TraceLog(LOG_INFO, "TMX: Tileset '%s' firstGID=%d, image='%s'",
                     ts->name, ts->firstGID, ts->imageSource);

            map->tilesetCount++;
            tsPtr = tagClose + 1;
            continue;
        }

        // Inline tileset (content between <tileset> and </tileset>)
        const char* tsClose = strstr(tsPtr, "</tileset>");
        if (!tsClose) {
            if (*(tagClose - 1) == '/') {
                tsPtr = tagClose + 1;
                map->tilesetCount++;
                continue;
            }
            break;
        }

        const char* imgPtr = tsPtr;
        while ((imgPtr = stristr(imgPtr, "<image")) != NULL && imgPtr < tsClose) {
            getStringAttr(imgPtr, "source", ts->imageSource, sizeof(ts->imageSource));
            ts->imageWidth  = getIntAttr(imgPtr, "width");
            ts->imageHeight = getIntAttr(imgPtr, "height");

            const char* imgEnd = strchr(imgPtr, '>');
            if (!imgEnd) break;
            imgPtr = imgEnd + 1;
            break;
        }

        TraceLog(LOG_INFO, "TMX: Tileset '%s' firstGID=%d, image='%s'",
                 ts->name, ts->firstGID, ts->imageSource);

        map->tilesetCount++;
        tsPtr = tsClose + 10;
    }

    // -------- <layer> tags: tile data for each map layer --------
    const char* layerPtr = xml;
    while ((layerPtr = stristr(layerPtr, "<layer")) != NULL && map->layerCount < MAX_LAYERS) {
        TileLayer* layer = &map->layers[map->layerCount];
        memset(layer, 0, sizeof(TileLayer));

        getStringAttr(layerPtr, "name", layer->name, sizeof(layer->name));
        layer->width  = getIntAttr(layerPtr, "width");
        layer->height = getIntAttr(layerPtr, "height");
        layer->visible = true;

        // Parse visible="0" attribute
        const char* visStr = stristr(layerPtr, "visible");
        if (visStr && visStr < strstr(layerPtr, ">")) {
            if (stristr(visStr, "0")) layer->visible = false;
        }

        TraceLog(LOG_INFO, "TMX: Layer '%s' %dx%d", layer->name, layer->width, layer->height);

        // Find <data> block between <layer> and </layer>
        const char* layerClose = strstr(layerPtr, "</layer>");
        if (!layerClose) break;

        const char* dataTag = stristr(layerPtr, "<data");
        if (!dataTag || dataTag > layerClose) {
            layerPtr = layerClose + 8;
            map->layerCount++;
            continue;
        }

        char encoding[32] = {0};
        getStringAttr(dataTag, "encoding", encoding, sizeof(encoding));

        // Text content of <data> tag
        const char* dataStart = strchr(dataTag, '>');
        if (!dataStart) { layerPtr = layerClose + 8; map->layerCount++; continue; }
        dataStart++;

        const char* dataEnd = strstr(dataStart, "</data>");
        if (!dataEnd) { layerPtr = layerClose + 8; map->layerCount++; continue; }

        int tileCount = layer->width * layer->height;
        layer->data = (int*)calloc(tileCount, sizeof(int));
        if (!layer->data) { layerPtr = layerClose + 8; map->layerCount++; continue; }

        if (strcmp(encoding, "csv") == 0 || strlen(encoding) == 0) {
            // Parse comma-separated values
            const char* csv = dataStart;
            int idx = 0;
            while (csv < dataEnd && idx < tileCount) {
                while (csv < dataEnd && (*csv == ' ' || *csv == '\t' || *csv == '\n' || *csv == '\r')) csv++;
                if (csv >= dataEnd) break;
                layer->data[idx++] = atoi(csv);
                while (csv < dataEnd && (*csv >= '0' && *csv <= '9')) csv++;
                if (csv < dataEnd && *csv == ',') csv++;
            }
        } else if (strcmp(encoding, "base64") == 0) {
            TraceLog(LOG_WARNING, "TMX: base64 encoding not supported for layer '%s'", layer->name);
        } else {
            // Raw <tile gid="N"/> elements inside the data block
            int idx = 0;
            const char* tilePtr = dataStart;
            while ((tilePtr = stristr(tilePtr, "<tile")) != NULL && tilePtr < dataEnd && idx < tileCount) {
                int gid = getIntAttr(tilePtr, "gid");
                layer->data[idx++] = gid;
                tilePtr++;
            }
        }

        map->layerCount++;
        layerPtr = layerClose + 8;
    }

    // -------- <objectgroup> tags: entity / item spawn points --------
    const char* objPtr = xml;
    while ((objPtr = stristr(objPtr, "<objectgroup")) != NULL) {
        const char* ogClose = strstr(objPtr, "</objectgroup>");
        if (!ogClose) break;

        const char* objTag = objPtr;
        while ((objTag = stristr(objTag, "<object")) != NULL && objTag < ogClose && map->objectCount < MAX_OBJECTS) {
            MapObject* obj = &map->objects[map->objectCount];
            memset(obj, 0, sizeof(MapObject));

            // Skip closing tags
            if (*(objTag + 7) == '/' || *(objTag + 7) == '>') { objTag += 8; continue; }

            getStringAttr(objTag, "name", obj->name, sizeof(obj->name));
            getStringAttr(objTag, "type", obj->type, sizeof(obj->type));
            obj->x = (float)getIntAttr(objTag, "x");
            obj->y = (float)getIntAttr(objTag, "y");

            int gid = getIntAttr(objTag, "gid");
            if (gid > 0) { obj->gid = gid; obj->hasGID = true; }

            map->objectCount++;

            const char* objClose = strstr(objTag, "</object>");
            objTag = (objClose && objClose < ogClose) ? (objClose + 9) : (objTag + 1);
        }

        objPtr = ogClose + 13;
    }

    free(xml);
    return map;
}

// ---------------------------------------------------------------------------
// Free a loaded map (including per-layer tile data arrays)
// ---------------------------------------------------------------------------
void UnloadTMX(MapData* map) {
    if (!map) return;
    for (int i = 0; i < map->layerCount; i++) {
        if (map->layers[i].data) free(map->layers[i].data);
    }
    free(map);
}

// ---------------------------------------------------------------------------
// Return the GID at tile (x, y) within a given layer, or 0 if out of bounds
// ---------------------------------------------------------------------------
int GetTileGID(const MapData* map, int layerIndex, int x, int y) {
    if (!map || layerIndex < 0 || layerIndex >= map->layerCount) return 0;
    if (x < 0 || x >= map->layers[layerIndex].width || y < 0 || y >= map->layers[layerIndex].height) return 0;
    return map->layers[layerIndex].data[y * map->layers[layerIndex].width + x];
}

// ---------------------------------------------------------------------------
// Find the index of the tileset that contains a given global tile ID
// ---------------------------------------------------------------------------
int FindTilesetForGID(const MapData* map, int gid) {
    if (!map || gid <= 0) return -1;
    for (int i = map->tilesetCount - 1; i >= 0; i--) {
        if (gid >= map->tilesets[i].firstGID) return i;
    }
    return -1;
}

// ---------------------------------------------------------------------------
// Get the source rectangle within the tileset image for a given tile GID
// ---------------------------------------------------------------------------
Rectangle GetTileSourceRect(const MapData* map, int gid) {
    Rectangle rect = { 0, 0, 0, 0 };
    if (!map || gid <= 0) return rect;

    int tsIndex = FindTilesetForGID(map, gid);
    if (tsIndex < 0) return rect;

    const TilesetDef* ts = &map->tilesets[tsIndex];
    int localID = gid - ts->firstGID;

    rect.width  = (float)ts->tileWidth;
    rect.height = (float)ts->tileHeight;

    if (ts->columns > 0) {
        rect.x = (float)((localID % ts->columns) * ts->tileWidth);
        rect.y = (float)((localID / ts->columns) * ts->tileHeight);
    }

    return rect;
}