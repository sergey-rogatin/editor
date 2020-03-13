#include "renderer.h"
#include <lvl5_random.h>

void init_renderer(Renderer *r, GLuint shader, Font *font, V2 window_size) {
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_MULTISAMPLE);
  
  GLuint texture;
  gl.GenTextures(1, &texture);
  gl.BindTexture(GL_TEXTURE_2D, texture);
  
  gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
  gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
  
  // disables texture multisampling
  // GL_LINEAR to enable
  gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  
  gl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, font->atlas.bmp.width, font->atlas.bmp.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, font->atlas.bmp.data);
  
  f32 quad_vertices[] = {
    0.0f, 0.0f,
    0.0f, 1.0f,
    1.0f,  1.0f,
    
    0.0f, 0.0f,
    1.0f, 1.0f,
    1.0f, 0.0f,
  };
  u32 quad_vbo;
  gl.GenBuffers(1, &quad_vbo);
  
  u32 vertex_vbo;
  gl.GenBuffers(1, &vertex_vbo);
  
  
  GLuint vao;
  gl.GenVertexArrays(1, &vao);
  gl.BindVertexArray(vao);
  
  gl.UseProgram(shader);
  
  gl.BindBuffer(GL_ARRAY_BUFFER, quad_vbo);
  gl.EnableVertexAttribArray(0);
  gl.VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);
  
  gl.BufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);
  
  gl.BindBuffer(GL_ARRAY_BUFFER, vertex_vbo);
  gl.EnableVertexAttribArray(1);
  gl.EnableVertexAttribArray(2);
  gl.EnableVertexAttribArray(3);
  gl.EnableVertexAttribArray(4);
  gl.EnableVertexAttribArray(5);
  gl.EnableVertexAttribArray(6);
  
  gl.VertexAttribDivisor(1, 1);
  gl.VertexAttribDivisor(2, 1);
  gl.VertexAttribDivisor(3, 1);
  gl.VertexAttribDivisor(4, 1);
  gl.VertexAttribDivisor(5, 1);
  gl.VertexAttribDivisor(6, 1);
  
  gl.VertexAttribPointer(
    1, 4, GL_SHORT, GL_FALSE, sizeof(Quad_Instance), (void *)offsetof(Quad_Instance, texture_x));
  
  gl.VertexAttribPointer(
    2, 4, GL_FLOAT, GL_FALSE, sizeof(Quad_Instance),
    (void *)offsetof(Quad_Instance, matrix));
  gl.VertexAttribPointer(
    3, 4, GL_FLOAT, GL_FALSE, sizeof(Quad_Instance),
    (void *)(offsetof(Quad_Instance, matrix) + sizeof(f32)*4));
  gl.VertexAttribPointer(
    4, 4, GL_FLOAT, GL_FALSE, sizeof(Quad_Instance),
    (void *)(offsetof(Quad_Instance, matrix) + sizeof(f32)*8));
  gl.VertexAttribPointer(
    5, 4, GL_FLOAT, GL_FALSE, sizeof(Quad_Instance),
    (void *)(offsetof(Quad_Instance, matrix) + sizeof(f32)*12));
  gl.VertexAttribPointer(
    6, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(Quad_Instance),
    (void *)offsetof(Quad_Instance, color));
  
  
  
  r->state.matrix = m4_identity();
  r->state.font = font;
  r->window_size = window_size;
  r->vertex_vbo = vertex_vbo;
  r->items = sb_new(Render_Item, 1024);
}


void render_scale(Renderer *r, V3 scale) {
  r->state.matrix = m4_mul_m4(r->state.matrix, m4_scaled(scale));
}

void render_translate(Renderer *r, V3 p) {
  r->state.matrix = m4_mul_m4(r->state.matrix, m4_translated(p));
}

void render_rotate(Renderer *r, f32 angle) {
  r->state.matrix = m4_mul_m4(r->state.matrix, m4_rotated(angle));
}


f32 measure_string_width(Renderer *r, String s) {
  Font *font = r->state.font;
  f32 result = 0;
  for (u32 char_index = 0; char_index < s.count; char_index++) {
    char first = s.data[char_index] - font->first_codepoint;
    char second = s.data[char_index+1] - font->first_codepoint;
    i8 advance = font->advance[first*font->codepoint_count+second];
    result += advance;
  }
  return result;
}

V2 draw_string(Renderer *r, String s, V2 p, V4 color, bool scared) {
  V3 pos = v2_to_v3(p, 0);
  render_translate(r, pos);
  
  Render_Item item = {
    .type = Render_Type_STRING,
    .state = r->state,
    .string = s,
  };
  item.state.color = color;
  item.scared = scared;
  sb_push(r->items, item);
  
  render_translate(r, v3_negate(pos));
  
  V2 result = v2(measure_string_width(r, s), 0);
  return result;
}

void draw_rect(Renderer *r, Rect2 rect, V4 color) {
  Render_Item item = {
    .type = Render_Type_RECT,
    .state = r->state,
    .rect = rect,
  };
  item.state.color = color;
  sb_push(r->items, item);
}


void renderer_output(Renderer *r) {
  static Rand seq = {0};
  if (seq.seed[0] == 0) {
    seq = make_random_sequence(371284738);
  }
  
  
  push_scratch_context();
  
  Quad_Instance *instances = sb_new(Quad_Instance, 10000);
  
  for (u32 item_index = 0; item_index < sb_count(r->items); item_index++) {
    Render_Item *item = r->items + item_index;
    Font *font = item->state.font;
    
    switch (item->type) {
      case Render_Type_STRING: {
        M4 matrix = item->state.matrix;
        String s = item->string;
        V2 offset = v2_zero();
        
        for (u32 char_index = 0; char_index < s.count; char_index++) {
          char first = s.data[char_index] - font->first_codepoint;
          char second = s.data[char_index+1] - font->first_codepoint;
          
          Rect2i rect = font->atlas.rects[first];
          V2 origin = font->origins[first];
          
          u16 width = (u16)(rect.max.x - rect.min.x);
          u16 height = (u16)(rect.max.y - rect.min.y);
          
          if (item->scared) {
            origin.x += random_range(&seq, -2, 2);
            origin.y += random_range(&seq, -2, 2);
          }
          
          M4 m = matrix;
          m = m4_mul_m4(m, m4_translated(v3(offset.x + origin.x,
                                            offset.y + origin.y, 
                                            0)));
          m = m4_mul_m4(m, m4_scaled(v3(width, height, 1)));
          
#if 0          
          V2 tr = v2(m.e30, m.e31);
          m.e30 = 0;
          m.e31 = 0;
          m = m4_scale(m, v3(width, height, 1));
          m = m4_translate(m, v3(tr.x + (offset.x + origin.x)*m.e00/width,
                                 tr.y + (offset.y + origin.y)*m.e11/height,
                                 0));
#endif
          
#if 0          
          m.e30 += (offset.x + origin.x)*m.e00;
          m.e31 += (offset.y + origin.y)*m.e11;
          
          m.e00 *= width;
          m.e11 *= height;
#endif
          
          Quad_Instance inst = {
            .matrix = m,
            .texture_x = (u16)rect.min.x,
            .texture_y = (u16)rect.min.y,
            .texture_w = width,
            .texture_h = height,
            .color = color_v4_to_opengl_u32(item->state.color),
          };
          
          sb_push(instances, inst);
          
          i8 advance = font->advance[first*font->codepoint_count+second];
          offset.x += advance;
        }
      } break;
      
      case Render_Type_RECT: {
        Rect2 rect = item->rect;
        
        V2 size = rect2_get_size(rect);
        
        M4 m = item->state.matrix;
        m = m4_scale(m, v3(size.x, size.y, 1));
        m = m4_translate(m, v3(rect.min.x*m.e00/size.x, 
                               rect.min.y*m.e11/size.y, 0));
        
        Rect2i sprite_rect = r->state.font->atlas.rects[r->state.font->atlas.count-1];
        
        Quad_Instance inst = {
          .matrix = m,
          .texture_x = (u16)sprite_rect.min.x,
          .texture_y = (u16)sprite_rect.min.y,
          .texture_w = 1,
          .texture_h = 1,
          .color = color_v4_to_opengl_u32(item->state.color),
        };
        sb_push(instances, inst);
      } break;
    }
  }
  
  
  gl.BindBuffer(GL_ARRAY_BUFFER, r->vertex_vbo);
  gl.BufferData(GL_ARRAY_BUFFER, sizeof(Quad_Instance)*sb_count(instances), instances, GL_DYNAMIC_DRAW);
  
  gl.DrawArraysInstanced(GL_TRIANGLES, 0, 6, sb_count(instances));
  
  pop_context();
}
