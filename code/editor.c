#include "editor.h"
#include "buffer.c"


Keybind *get_keybind(Settings *settings, os_Keycode keycode, 
                     bool shift, bool ctrl, bool alt) 
{
  Keybind *result = null;
  for (u32 i = 0; i < sb_count(settings->keybinds); i++) {
    Keybind *k = settings->keybinds + i;
    if (k->keycode == keycode &&
        k->shift == shift && 
        k->ctrl == ctrl && 
        k->alt == alt)
    {
      result = k;
      break;
    }
  }
  return result;
}

void execute_command(Editor *editor, Renderer *renderer, Command command) {
  Text_Buffer *buffer = &editor->buffer;
  Font *font = renderer->state.font;
  
  switch (command) {
    case Command_COPY: {
      buffer_copy(buffer);
    } break;
    case Command_PASTE: {
      buffer_paste(buffer);
    } break;
    case Command_CUT: {
      buffer_cut(buffer);
    } break;
    
    case Command_MOVE_CURSOR_RIGHT:
    case Command_MOVE_CURSOR_UP:
    case Command_MOVE_CURSOR_DOWN:
    case Command_MOVE_CURSOR_LEFT: {
      move_cursor_direction(font, buffer, command);
    } break;
    
    case Command_REMOVE_BACKWARD: {
      buffer_remove_backward(buffer, 1);
    } break;
    case Command_REMOVE_FORWARD: {
      buffer_remove_forward(buffer, 1);
    } break;
    case Command_NEWLINE: {
      buffer_newline(buffer);
    } break;
    case Command_TAB: {
      buffer_indent(buffer);
    } break;
  }
}

extern void editor_update(Os os, Editor_Memory *memory, os_Input *input) {
  Editor_State *state = (Editor_State *)memory->data;
  
  if(memory->reloaded) {
    global_context_info = os.context_info;
  }
  
  if (!memory->initialized) {
    memory->initialized = true;
    
    GLuint shader = gl_create_shader_from_file(os.gl, os.read_entire_file, const_string("shader.glsl"));
    Renderer *renderer = &state->renderer;
    
    
    
    state->font = os.load_font(const_string("ubuntu_mono.ttf"), 
                               const_string("Ubuntu Mono"),
                               24);
    
    V2 window_size = os.get_window_size(memory->window);
    init_renderer(os.gl, renderer, shader, &state->font, window_size);
    
    
    Text_Buffer *buffer = &state->editor.buffer;
    buffer->data = alloc_array(char, 128);
    buffer->capacity = 128;
    buffer->count = 0;
    buffer->cursor = 0;
    
    String str;
    // TODO: all files should end with \0
    {
      FILE *file;
      errno_t err = fopen_s(&file, "test.c", "rb");
      fseek(file, 0, SEEK_END);
      i32 file_size = ftell(file);
      fseek(file, 0, SEEK_SET);
      
      char *file_memory = alloc_array(char, file_size+1);
      size_t read = fread(file_memory, 1, file_size, file);
      assert(read == file_size);
      fclose(file);
      str = make_string(file_memory, file_size+1);
      str.data[file_size] = 0;
    }
    
    buffer_insert_string(buffer, str);
    set_cursor(buffer, 0);
    
    
    
    Editor *editor = &state->editor;
    editor->settings.keybinds = sb_new(Keybind, 32);
    Keybind *keybinds = editor->settings.keybinds;
    
    sb_push(keybinds, ((Keybind){
                       .command = Command_COPY,
                       .keycode = 'C',
                       .ctrl = true,
                       }));
    sb_push(keybinds, ((Keybind){
                       .command = Command_PASTE,
                       .keycode = 'V',
                       .ctrl = true,
                       }));
    sb_push(keybinds, ((Keybind){
                       .command = Command_CUT,
                       .keycode = 'X',
                       .ctrl = true,
                       }));
    sb_push(keybinds, ((Keybind){
                       .command = Command_MOVE_CURSOR_LEFT,
                       .keycode = os_Keycode_ARROW_LEFT,
                       }));
    sb_push(keybinds, ((Keybind){
                       .command = Command_MOVE_CURSOR_RIGHT,
                       .keycode = os_Keycode_ARROW_RIGHT,
                       }));
    sb_push(keybinds, ((Keybind){
                       .command = Command_MOVE_CURSOR_UP,
                       .keycode = os_Keycode_ARROW_UP,
                       }));
    sb_push(keybinds, ((Keybind){
                       .command = Command_MOVE_CURSOR_DOWN,
                       .keycode = os_Keycode_ARROW_DOWN,
                       }));
    sb_push(keybinds, ((Keybind){
                       .command = Command_REMOVE_BACKWARD,
                       .keycode = os_Keycode_BACKSPACE,
                       }));
    sb_push(keybinds, ((Keybind){
                       .command = Command_REMOVE_FORWARD,
                       .keycode = os_Keycode_DELETE,
                       }));
    sb_push(keybinds, ((Keybind){
                       .command = Command_NEWLINE,
                       .keycode = os_Keycode_ENTER,
                       }));
    sb_push(keybinds, ((Keybind){
                       .command = Command_TAB,
                       .keycode = os_Keycode_TAB,
                       }));
    
    
    Color_Theme monokai = (Color_Theme){
      .name = const_string("Monokai"),
    };
    monokai.colors[Text_Color_Type_BACKGROUND] = 0xFF272822;
    monokai.colors[Text_Color_Type_DEFAULT] = 0xFFF8F8F2;
    monokai.colors[Text_Color_Type_COMMENT] = 0xFF88846F;
    monokai.colors[Text_Color_Type_TYPE] = 0xFF66D9EF;
    monokai.colors[Text_Color_Type_MACRO] = 0xFFAE81FF;
    monokai.colors[Text_Color_Type_FUNCTION] = 0xFFA6E22E;
    monokai.colors[Text_Color_Type_ARG] = 0xFFFD911F;
    monokai.colors[Text_Color_Type_OPERATOR] = 0xFFF92672;
    monokai.colors[Text_Color_Type_KEYWORD] = 0xFFF92672;
    monokai.colors[Text_Color_Type_CURSOR] = 0xFF00FF00;
    monokai.colors[Text_Color_Type_NUMBER] = monokai.colors[Text_Color_Type_MACRO];
    monokai.colors[Text_Color_Type_STRING] = 0xFFE6DB74;
    editor->settings.theme = monokai;
  }
  Editor *editor = &state->editor;
  Renderer *renderer = &state->renderer;
  Font *font = renderer->state.font;
  Text_Buffer *buffer = &editor->buffer;
  gl_Funcs gl = os.gl;
  
  scratch_reset();
  os.collect_messages(memory->window, input);
  
  begin_profiler_event("input");
  os_Event event;
  while (os.pop_event(&event)) {
    switch (event.type) {
      case os_Event_Type_CLOSE: {
        memory->running = false;
      } break;
      
      case os_Event_Type_RESIZE: {
        V2 window_size = os.get_window_size(memory->window);
        V2i size = v2_to_v2i(window_size);
        gl.Viewport(0, 0, size.x, size.y);
        renderer->window_size = window_size;
      } break;
      
      case os_Event_Type_BUTTON: {
        os_Keycode keycode = event.button.keycode;
        os_Button key = input->keys[keycode];
        
        if (key.pressed) {
          Keybind *bind = get_keybind(&editor->settings, keycode, 
                                      input->shift, input->ctrl, input->alt);
          if (bind) {
            execute_command(editor, renderer, bind->command);
          }
        }
      } break;
      
      default: {} break;
    }
  }
  
  
  if (!input->ctrl && !input->alt) {
    if (input->char_count > 0) {
      String str = make_string(input->chars, input->char_count);
      buffer_input_string(buffer, str);
    }
  }
  
  end_profiler_event("input");
  
  renderer->state.matrix = m4_identity();
  sb_count(renderer->items) = 0;
  
  render_scale(renderer, v3(2/renderer->window_size.x,
                            2/renderer->window_size.y, 1));
  
  V2 bottom_left = v2(-renderer->window_size.x*0.5f,
                      -renderer->window_size.y*0.5f);
  buffer_draw(renderer, buffer, rect2_min_size(bottom_left, 
                                               renderer->window_size), editor->settings.theme);
  renderer_output(gl, renderer);
}
