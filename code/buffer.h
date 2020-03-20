#ifndef EDITOR_H
#include <lvl5_types.h>
#include "font.h"
#include <lvl5_string.h>

typedef struct Token Token;


typedef enum {
  Syntax_DEFAULT,
  Syntax_BACKGROUND,
  Syntax_COMMENT,
  Syntax_TYPE,
  Syntax_MACRO,
  Syntax_FUNCTION,
  Syntax_ARG,
  Syntax_OPERATOR,
  Syntax_KEYWORD,
  Syntax_CURSOR,
  Syntax_NUMBER,
  Syntax_STRING,
  Syntax_ENUM_MEMBER,
  
  Syntax_count,
} Syntax;


#define MAX_EXCHANGE_COUNT 1024
typedef struct {
  char data[MAX_EXCHANGE_COUNT];
  i32 count;
} Exchange;

typedef struct {
  String file_name;
  
  char *data;
  i32 count;
  i32 capacity;
  
  i32 cursor;
  i32 mark;
  i32 preferred_col_pos;
  
  i8 *colors;
} Buffer;


String buffer_part_to_string(Buffer *, i32, i32);
char get_buffer_char(Buffer *, i32);

#define EDITOR_H
#endif