#include "config.h"
#include "cJSON.h"
#include "raylib.h"
#include <string.h>

static cJSON* s_root = NULL;
static char s_path[256] = "";

void Config_Load(const char* path) {
    if (!path) return;
    strncpy(s_path, path, sizeof(s_path) - 1);
    s_path[sizeof(s_path) - 1] = '\0';

    Config_Unload();

    char* text = LoadFileText(path);
    if (!text) {
        TraceLog(LOG_WARNING, "Config_Load: could not read '%s' — using defaults", path);
        return;
    }

    s_root = cJSON_Parse(text);
    UnloadFileText(text);

    if (!s_root) {
        TraceLog(LOG_WARNING, "Config_Load: JSON parse error in '%s' — using defaults", path);
    } else {
        TraceLog(LOG_INFO, "Config_Load: loaded '%s'", path);
    }
}

void Config_LoadFromString(const char* json) {
    Config_Unload();
    if (!json) return;
    s_root = cJSON_Parse(json);
    s_path[0] = '\0';
}

void Config_Reload(void) {
    if (s_path[0]) {
        TraceLog(LOG_INFO, "Config_Reload: reloading '%s'", s_path);
        Config_Load(s_path);
    }
}

void Config_Unload(void) {
    if (s_root) {
        cJSON_Delete(s_root);
        s_root = NULL;
    }
}

static cJSON* FindValue(const char* section, const char* key) {
    if (!s_root || !section || !key) return NULL;
    cJSON* sec = cJSON_GetObjectItem(s_root, section);
    if (!sec) return NULL;
    return cJSON_GetObjectItem(sec, key);
}

int Config_GetInt(const char* section, const char* key, int defaultVal) {
    cJSON* val = FindValue(section, key);
    if (!val || !cJSON_IsNumber(val)) return defaultVal;
    return val->valueint;
}

float Config_GetFloat(const char* section, const char* key, float defaultVal) {
    cJSON* val = FindValue(section, key);
    if (!val || !cJSON_IsNumber(val)) return defaultVal;
    return (float)val->valuedouble;
}

bool Config_GetBool(const char* section, const char* key, bool defaultVal) {
    cJSON* val = FindValue(section, key);
    if (!val) return defaultVal;
    if (cJSON_IsBool(val)) return cJSON_IsTrue(val);
    return defaultVal;
}
