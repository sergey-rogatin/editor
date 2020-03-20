#include "parser.h"



// token strings always include one additional char for kerning
String token_to_string(Buffer *b, Token *t) {
  String result = buffer_part_to_string(b, t->start, t->end + 1);
  result.count--;
  return result;
}

#if 0
Token_Type get_keyword_kind(String str) {
  i32 result = 0;
  for (i32 i = T_KEYWORD_FIRST; i <= T_KEYWORD_LAST; i++) {
    String keyword_string = Token_Kind_To_String[i];
    if (string_compare(keyword_string, str)) {
      result = i;
      break;
    }
  }
  return result;
}

#endif


b32 is_digit(char c) {
  b32 result = c >= '0' && c <= '9';
  return result;
}

b32 is_alpha(char c) {
  b32 result = (c >= 'a' && c <= 'z') ||
    (c >= 'A' && c <= 'Z') || (c == '_');
  return result;
}

b32 is_whitespace(char c) {
  b32 result = c == ' ' || c == '\n' || c == '\t' || c == '\r';
  return result;
}



void set_color(Parser *p, Token *t, Syntax color) {
  for (i32 i = t->start; i < t->end; i++) {
    p->colors[i] = color;
  }
}

void set_color_by_type(Parser *p, Token *t) {
  Syntax syntax = Syntax_DEFAULT;
  if (t->kind >= T_KEYWORD_FIRST && t->kind <= T_KEYWORD_LAST ||
      t->kind == T_POUND) {
    syntax = Syntax_KEYWORD;
  } else if (t->kind >= T_OPERATOR_FIRST && t->kind <= T_OPERATOR_LAST) {
    syntax = Syntax_OPERATOR;
  } else if (t->kind == T_INT || t->kind == T_FLOAT) {
    syntax = Syntax_NUMBER;
  } else if (t->kind == T_STRING || t->kind == T_CHAR) {
    syntax = Syntax_STRING;
  }
  set_color(p, t, syntax);
}

Token *buffer_tokenize(Parser *p, Buffer *b) {
  begin_profiler_event("tokenize");
  
  push_scratch_context();
  Token *tokens = sb_new(Token, 1024);
  pop_context();
  
  i32 line = 1;
  i32 col = 1;
  i32 token_start = 0;
  
  Token t = {0};
  i32 i = 0;
  
#define get(index) get_buffer_char(b, i + index)
  
#define next(n) { \
    col += n; \
    i += n; \
  }
#define skip_syntax(n, syntax) { \
    next(n); \
    token_start += n; \
    for (i32 i = token_start-n; i < token_start; i++) { \
      p->colors[i] = syntax; \
    } \
  }
#define eat() { \
    next(1); \
  }
#define end_no_continue(tok_kind) { \
    t.kind = tok_kind; \
    t.start = token_start; \
    t.end = i; \
    set_color_by_type(p, &t); \
    token_start = i; \
    sb_push(tokens, t); \
  }
#define end(tok_kind) end_no_continue(tok_kind); continue;
  
#define case1(ch0, kind0) \
  case ch0: { \
    eat(); \
    end(kind0); \
  } break;
#define case2(ch0, kind0, ch1, kind1) \
  case ch0: { \
    eat(); \
    if (get(0) == ch1) { \
      eat(); \
      end(kind1); \
    } else { \
      end(kind0); \
    } \
  } break;
#define case3(ch0, kind0, ch1, kind1, ch2, kind2) \
  case ch0: { \
    eat(); \
    if (get(0) == ch1) { \
      eat(); \
      end(kind1); \
    } else if (get(0) == ch2) { \
      eat(); \
      end(kind2); \
    } else { \
      end(kind0); \
    } \
  } break;
  
  while (true) {
    switch (get(0)) {
      case 0: {
        goto end;
      } break;
      
      case '\r': {
        skip_syntax(1, Syntax_DEFAULT);
      } break;
      case ' ': {
        skip_syntax(1, Syntax_DEFAULT);
      } break;
      case '\n': {
        line++;
        col = 1;
        skip_syntax(1, Syntax_DEFAULT);
      } break;
      case '"': {
        eat();
        while (true) {
          if (get(0) == '\\') {
            eat();
            eat();
          } else if (get(0) == '"') {
            break;
          } else if (get(0) == '\n') {
            break;
          } else if (get(0) == '\0') {
            i--;
            break;
          } else {
            eat();
          }
        }
        eat();
        end(T_STRING);
      } break;
      case '\'': {
        eat();
        
        while (true) {
          if (get(0) == '\\') {
            eat();
            eat();
          } else if (get(0) == '\'') {
            break;
          } else if (get(0) == '\n') {
            break;
          } else if (get(0) == '\0') {
            i--;
            break;
          } else {
            eat();
          }
        }
        eat();
        end(T_CHAR);
      } break;
      
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9': {
        eat();
        while (is_digit(get(0))) eat();
        
        // float
        if (get(0) == '.') {
          eat();
          //if (!is_digit((get0))) syntax_error("Unexpected symbol %c", *stream);
          
          while (is_digit(get(0))) eat();
          end(T_FLOAT);
        } else {
          end(T_INT);
        }
      } break;
      case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
      case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
      case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
      case 'v': case 'w': case 'x': case 'y': case 'z':
      
      case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
      case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
      case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
      case 'V': case 'W': case 'X': case 'Y': case 'Z':
      
      case '_': {
        eat();
        while (is_digit(get(0)) || is_alpha(get(0))) eat();
        
        
        String token_string = {0};
        i32 count = i - token_start;
        Token_Type kind = T_NONE;
        i32 result = 0;
        {
          for (i32 j = T_KEYWORD_FIRST; j <= T_KEYWORD_LAST; j++) {
            String keyword_string = Token_Kind_To_String[j];
            if ((i32)keyword_string.count == count) {
              if (token_string.count == 0) {
                token_string = buffer_part_to_string(b, token_start, i);
              }
              if (string_compare(keyword_string, token_string)) {
                kind = j;
                break;
              }
            }
          }
        }
        
        if (kind) {
          t.kind = kind;
          t.start = token_start;
          t.end = i;
          set_color(p, &t, Syntax_KEYWORD);
          token_start = i; 
          sb_push(tokens, t); 
          continue;
        } else {
          end(T_NAME);
        }
      } break;
      
      case '/': {
        if (get(1) == '/') {
          skip_syntax(2, Syntax_COMMENT);
          while (get(0) != '\n') {
            skip_syntax(1, Syntax_COMMENT);
          }
        } else if (get(1) == '*') {
          skip_syntax(2, Syntax_COMMENT);
          while (!(get(0) == '*' && get(1) == '/')) {
            if (get(0) == '\0') {
              goto end;
            } else {
              skip_syntax(1, Syntax_COMMENT);
            }
          }
          skip_syntax(2, Syntax_COMMENT);
        } else {
          eat();
          if (get(0) == '=') {
            end(T_ASSIGN_SLASH);
          } else {
            end(T_SLASH);
          }
        }
      } break;
      
      case '#': {
        eat();
        while (is_digit(get(0)) || is_alpha(get(0))) eat();
        end_no_continue(T_POUND);
        
        i32 j = 0;
        while (get(j) == ' ') {
          skip_syntax(1, T_DEFAULT);
        }
        
        if (get(j) == '<') {
          skip_syntax(j, T_DEFAULT);
          while (get(0) != '>') {
            eat();
          }
          eat();
          end(T_STRING);
        }
      } break;
      
      case1('\\', T_BACKSLASH);
      case1(',', T_COMMA);
      case1(';', T_SEMI);
      case1('(', T_LPAREN);
      case1(')', T_RPAREN);
      case1('[', T_LBRACKET);
      case1(']', T_RBRACKET);
      case1('{', T_LCURLY);
      case1('}', T_RCURLY);
      case1('~', T_BIT_NOT);
      case1(':', T_COLON);
      
      case2('=', T_ASSIGN, '=', T_EQUALS);
      case2('!', T_NOT, '=', T_NOT_EQUALS);
      case2('+', T_PLUS, '=', T_ASSIGN_PLUS);
      case2('*', T_STAR, '=', T_ASSIGN_STAR);
      case2('^', T_BIT_XOR, '=', T_ASSIGN_BIT_XOR);
      case2('%', T_PERCENT, '=', T_ASSIGN_PERCENT);
      
      case3('-', T_MINUS, '=', T_ASSIGN_MINUS, '>', T_ARROW);
      case3('.', T_DOT, '.', T_DOUBLE_DOT, '.', T_TRIPLE_DOT);
      case3('>', T_GREATER, '=', T_GREATER_EQUALS, '>', T_RSHIFT);
      case3('<', T_LESS, '=', T_LESS_EQUALS, '<', T_LSHIFT);
      case3('|', T_BIT_OR, '|', T_OR, '=', T_ASSIGN_BIT_OR);
      case3('&', T_BIT_AND, '&', T_AND, '=', T_ASSIGN_BIT_AND);
      
      default: {
        eat();
        end(T_NONE);
      } break;
    }
  }
  
  end:
  
  end_profiler_event("tokenize");
  
  return tokens;
}


Symbol *get_symbol(Parser *p, Token *t) {
  begin_profiler_event("get_symbol");
  Symbol *result = null;
  String token_string = token_to_string(p->buffer, t);
  
  for (u32 i = 0; i < sb_count(p->symbols); i++) {
    Symbol *s = p->symbols + i;
    if (string_compare(s->name, token_string)) {
      result = s;
      break;
    }
  }
  
  end_profiler_event("get_symbol");
  return result;
}

Token *peek_token(Parser *p, i32 offset) {
  Token *result = p->tokens + p->i + offset;
  return result;
}

void next_token(Parser *p) {
  p->i++;
  assert(p->i <= (i32)sb_count(p->tokens));
}

bool accept_token(Parser *p, Token_Type kind) {
  bool result = false;
  if (kind == peek_token(p, 0)->kind) {
    result = true;
    next_token(p);
  }
  return result;
}



void add_symbol(Parser *p, String name, Syntax kind) {
  Symbol s = (Symbol){ .name = name, .kind = kind };
  sb_push(p->symbols, s);
}



Token *parse_declarator(Parser *, bool);
bool parse_decl_specifier(Parser *);
bool parse_decl(Parser *);
bool parse_decl_specifier(Parser *);
void parse_any(Parser *);


void parse_misc(Parser *p) {
  Token *t = peek_token(p, 0);
  if (t->kind == T_NAME) {
    Symbol *s = get_symbol(p, t);
    if (s) {
      if (s->kind == Syntax_FUNCTION) {
        set_color(p, t, Syntax_FUNCTION);
      } else if (s->kind == Syntax_MACRO) {
        set_color(p, t, Syntax_MACRO);
      } else if (s->kind == Syntax_ENUM_MEMBER) {
        set_color(p, t, Syntax_ENUM_MEMBER);
      }
    }
  }
  next_token(p);
}

Token *parse_direct_declarator(Parser *p, bool is_typedef) {
  Token *result = null;
  
  if (accept_token(p, T_NAME)) {
    result = peek_token(p, -1);
    
    if (is_typedef) {
      // token_to_string is scratch
      add_symbol(p, token_to_string(p->buffer, result), Syntax_TYPE);
      set_color(p, result, Syntax_TYPE);
    }
  } else if (accept_token(p, T_LPAREN)) {
    result = parse_declarator(p, is_typedef);
    accept_token(p, T_RPAREN);
  }
  
  if (accept_token(p, T_LBRACKET)) {
    while (!accept_token(p, T_RBRACKET)) {
      parse_misc(p);
    }
  } else if (accept_token(p, T_LPAREN)) {
    // function decl
    if (!is_typedef) {
      add_symbol(p, token_to_string(p->buffer, result), Syntax_FUNCTION);
      set_color(p, result, Syntax_FUNCTION);
    }
    if (!accept_token(p, T_RPAREN)) {
      do {
        while (parse_decl_specifier(p));
        parse_declarator(p, false);
      } while (accept_token(p, T_COMMA));
    }
  }
  
  return result;
}

Token *parse_declarator(Parser *p, bool is_typedef) {
  accept_token(p, T_STAR);
  return parse_direct_declarator(p, is_typedef);
} 


bool parse_initializer(Parser *p) {
  while (!accept_token(p, T_SEMI)) {
    parse_any(p);
  }
  return true;
}

bool parse_init_declarator(Parser *p, bool is_typedef) {
  bool result = false;
  if (parse_declarator(p, is_typedef)) {
    if (accept_token(p, T_ASSIGN)) {
      if (parse_initializer(p)) {
        result = true;
      }
    } else {
      result = true;
    }
  }
  return result;
}

bool parse_struct_specifier(Parser *p) {
  bool result = false;
  
  if (accept_token(p, T_STRUCT) || accept_token(p, T_UNION)) {
    bool has_name = false, has_body = false;
    if (accept_token(p, T_NAME)) {
      Token *struct_name = peek_token(p, -1);
      set_color(p, struct_name, Syntax_TYPE);
      // token_to_string is scratch
      add_symbol(p, token_to_string(p->buffer, struct_name), Syntax_TYPE);
      has_name = true;
    }
    
    if (accept_token(p, T_LCURLY)) {
      while (!accept_token(p, T_RCURLY)) {
        if (parse_decl_specifier(p)) {
          while (parse_decl_specifier(p));
          do {
            parse_declarator(p, false);
          } while (accept_token(p, T_COMMA));
          accept_token(p, T_SEMI);
        } else {
          parse_any(p);
        }
      }
      has_body = true;
    }
    result = has_name || has_body;
  }
  
  return result;
}

bool parse_typename(Parser *p) {
  Token *t = peek_token(p, 0);
  Symbol *s = get_symbol(p, t);
  if (s && s->kind == Syntax_TYPE) {
    set_color(p, t, Syntax_TYPE);
    next_token(p);
    return true;
  }
  return false;
}

bool parse_enum_specifier(Parser *p) {
  bool result = false;
  if (accept_token(p, T_ENUM)) {
    bool has_name = false, has_body = false;
    if (accept_token(p, T_NAME)) {
      Token *struct_name = peek_token(p, -1);
      set_color(p, struct_name, Syntax_TYPE);
      // token_to_string is scratch
      add_symbol(p, token_to_string(p->buffer, struct_name), Syntax_TYPE);
      has_name = true;
    }
    
    if (accept_token(p, T_LCURLY)) {
      while (!accept_token(p, T_RCURLY)) {
        if (accept_token(p, T_NAME)) {
          Token *name = peek_token(p, -1);
          set_color(p, name, Syntax_ENUM_MEMBER);
          add_symbol(p, token_to_string(p->buffer, name), Syntax_ENUM_MEMBER);
          if (accept_token(p, T_ASSIGN)) {
            while (!accept_token(p, T_COMMA)) {
              next_token(p);
            }
          }
        } else {
          parse_any(p);
        }
      }
      has_body = true;
    }
    result = has_name || has_body;
  }
  
  return result;
}

bool parse_decl_specifier(Parser *p) {
  Token *t = peek_token(p, 0);
  switch (t->kind) {
    case T_STATIC:
    case T_TYPEDEF:
    case T_EXTERN:
    case T_AUTO:
    case T_REGISTER:
    
    case T_SIGNED:
    case T_UNSIGNED:
    
    case T_INLINE:
    case T_CONST: 
    case T_VOLATILE: {
      next_token(p);
      return true;
    } break;
    
    case T_NAME: {
      return parse_typename(p);
    } break;
    
    case T_STRUCT:
    case T_UNION: {
      return parse_struct_specifier(p);
    } break;
    
    case T_ENUM: {
      return parse_enum_specifier(p);
    } break;
  }
  return false;
}

bool parse_decl(Parser *p) {
  i32 saved = p->i;
  
  bool is_typedef = false;
  if (parse_decl_specifier(p)) {
    if (peek_token(p, -1)->kind == T_TYPEDEF) {
      is_typedef = true;
    }
    while (parse_decl_specifier(p));
    do {
      parse_init_declarator(p, is_typedef);
    } while (accept_token(p, T_COMMA));
    accept_token(p, T_SEMI);
    return true;
  }
  
  p->i = saved;
  return false;
}

void parse_any(Parser *p) {
  Token *t = peek_token(p, 0);
  switch (t->kind) {
    case T_POUND: {
      set_color(p, t, Syntax_KEYWORD);
      if (string_compare(token_to_string(p->buffer, t),
                         const_string("#define"))) {
        next_token(p);
        Token *macro = peek_token(p, 0);
        set_color(p, macro, Syntax_MACRO);
        add_symbol(p, token_to_string(p->buffer, macro), Syntax_MACRO);
        next_token(p);
      } else {
        parse_misc(p);
      }
    } break;
    
    default: {
      if (parse_decl(p)) {
        
      } else {
        parse_misc(p);
      }
    } break;
  }
}

void parse_program(Parser *p) {
  begin_profiler_event("parse");
  while (p->i < (i32)sb_count(p->tokens)) {
    parse_any(p);
  }
  end_profiler_event("parse");
}

i8 *buffer_parse(Buffer *b) {
  Parser parser = {
    .i = 0,
    .tokens = null,
    .buffer = b,
    .symbols = sb_new(Symbol, 32),
    .colors = sb_new(i8, b->count),
  };
  parser.tokens = buffer_tokenize(&parser, b);
  Parser *p = &parser;
  add_symbol(p, const_string("char"), Syntax_TYPE);
  add_symbol(p, const_string("void"), Syntax_TYPE);
  add_symbol(p, const_string("short"), Syntax_TYPE);
  add_symbol(p, const_string("int"), Syntax_TYPE);
  add_symbol(p, const_string("long"), Syntax_TYPE);
  add_symbol(p, const_string("float"), Syntax_TYPE);
  add_symbol(p, const_string("double"), Syntax_TYPE);
  
  parse_program(&parser);
  
  
  return parser.colors;
}