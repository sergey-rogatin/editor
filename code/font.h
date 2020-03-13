#ifndef FONT_H
#include <lvl5_types.h>

typedef struct {
  Bitmap bmp;
  Rect2i *rects;
  i32 count;
} Texture_Atlas;

typedef struct {
  Texture_Atlas atlas;
  char first_codepoint;
  i32 codepoint_count;
  V2 *origins;
  
  i8 *advance;
  i8 *kerning;
  i8 line_spacing;
  i8 space_width;
} Font;


#define FONT_H
#endif