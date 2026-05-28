#ifndef TEXT_DATA_H
#define TEXT_DATA_H

extern const unsigned char s_controls_data[886];
extern const unsigned char s_credits_data[553];
extern const unsigned char s_story_data[1116];

void RenderTextScreen(const unsigned char* data, int len, const char* backHint, int scrollOffset);

#endif
