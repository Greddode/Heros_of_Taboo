#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

void  Config_Load(const char* path);
void  Config_Reload(void);
void  Config_Unload(void);
int   Config_GetInt(const char* section, const char* key, int defaultVal);
float Config_GetFloat(const char* section, const char* key, float defaultVal);
bool  Config_GetBool(const char* section, const char* key, bool defaultVal);

// For testing only — load from a string instead of a file
void  Config_LoadFromString(const char* json);

#endif
