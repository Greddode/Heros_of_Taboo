#ifndef TEXT_DATA_H
#define TEXT_DATA_H

extern const unsigned char s_controls_data[274];
extern const unsigned char s_credits_data[560];

void RenderTextScreen(const unsigned char* data, int len, const char* backHint);

#endif
