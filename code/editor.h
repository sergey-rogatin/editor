#ifndef EDITOR_H
#include "buffer.h"
#include "common.h"
#include "renderer.h"
#include "lvl5_stretchy_buffer.h"

typedef enum {
  Command_NONE,
  Command_COPY,
  Command_SET_MARK,
  Command_PASTE,
  Command_CUT,
  Command_MOVE_CURSOR_LEFT,
  Command_MOVE_CURSOR_RIGHT,
  Command_MOVE_CURSOR_UP,
  Command_MOVE_CURSOR_DOWN,
  Command_MOVE_CURSOR_LINE_START,
  Command_MOVE_CURSOR_LINE_END,
  Command_MOVE_CURSOR_WORD_START,
  Command_MOVE_CURSOR_WORD_END,
  Command_REMOVE_BACKWARD,
  Command_REMOVE_FORWARD,
  Command_NEWLINE,
  Command_TAB,
  Command_OPEN_FILE_DIALOG,
  Command_LISTER_MOVE_UP,
  Command_LISTER_MOVE_DOWN,
  Command_LISTER_SELECT,
  Command_SAVE_BUFFER,
} Command;

typedef struct Color_Theme {
  String name;
  u32 colors[Syntax_count];
} Color_Theme;



typedef enum {
  Panel_Type_NONE,
  Panel_Type_BUFFER,
  Panel_Type_FILE_DIALOG_OPEN,
  Panel_Type_FILE_DIALOG_NEW,
  Panel_Type_SETTINGS,
} Panel_Type;


// TODO: hashtable this shit
typedef struct {
  bool shift;
  bool ctrl;
  bool alt;
  os_Keycode keycode;
  Panel_Type views;
  
  Command command;
} Keybind;

typedef struct {
  Keybind *keybinds;
  Color_Theme theme;
} Settings;

typedef struct {
  String *items;
  i32 index;
} Lister;


typedef struct Buffer_View {
  Buffer *buffer;
  i32 visible_cursor;
  i32 preferred_col_pos;
  i32 visible_mark;
} Buffer_View;

typedef struct {
  String path;
  Lister files;
} File_Dialog;

typedef struct {
  union {
    Buffer_View buffer_view;
    File_Dialog file_dialog;
  };
  
  Panel_Type type;
  Rect2 rect;
  String name;
  V2 scroll;
} Panel;




typedef enum {
  ui_VERTICAL,
  ui_HORIZONTAL,
  ui_STRETCH,
  ui_IGNORE_LAYOUT,
} ui_Enum;

#define Size_STRETCH INFINITY
#define Size_AUTO 0

typedef struct {
  ui_Enum direction;
  ui_Enum align_content;
  ui_Enum position;
  
  f32 width;
  f32 height;
  f32 padding_left;
  f32 padding_right;
  f32 padding_top;
  f32 padding_bottom;
  
  u32 bg_color;
  f32 layer;
} Style;


#if 0
typedef enum {
  Layout_Mode_NONE,
  Layout_Mode_RESOLVE_STRETCHES,
  Layout_Mode_DRAW,
} Layout_Mode;
#endif


typedef enum {
  Item_Type_NONE,
  Item_Type_FLEX,
  Item_Type_BUTTON,
  Item_Type_PANEL,
} Item_Type;

typedef struct {
  u8 func;
  u8 loop;
  u8 call;
} ui_Id;

#define INVALID_UI_ID (ui_Id){.call = 255, .loop = 255}

typedef struct Item Item;
typedef struct Item {
  V2 p;
  V2 size;
  
  ui_Id id;
  Item_Type type;
  Style style;
  Item **children;
  Item *parent;
  
  // TODO(lvl5): this stuff should be separated
  bool is_active;
  String label;
} Item;

#define LAYOUT_ITEM_MAX 512

typedef struct {
  V2 p;
  //Layout_Mode mode;
  
  ui_Id keys[LAYOUT_ITEM_MAX];
  Item values[LAYOUT_ITEM_MAX];
  bool occupancy[LAYOUT_ITEM_MAX];
  u32 count;
  
  Item *current_container;
  Renderer *renderer;
  os_Input *input;
  
  ui_Id hot;
  ui_Id active;
} Layout;


typedef struct Editor {
  Buffer *buffers;
  Panel *panels;
  i32 active_panel_index;
  
  Settings settings;
  Exchange exchange;
  
  i32 generation;
  
  i32 menu_index;
  Layout layout;
} Editor;

#include "renderer.h"

typedef struct {
  Renderer renderer;
  Font font;
  Editor editor;
} App_State;


#define EDITOR_H
#endif
