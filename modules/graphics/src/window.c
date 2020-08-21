#include <mruby.h>
#include <mruby/data.h>
#include <mruby/variable.h>
#include <mruby/class.h>

#include <rayfork.h>

#include <ogss/drawable.h>
#include <ogss/point.h>
#include <ogss/rect.h>
#include <ogss/color.h>
#include <ogss/bitmap.h>
#include <ogss/window.h>
#include <ogss/viewport.h>
#include <ogss/graphics.h>

static void
free_window(mrb_state *mrb, void *p)
{
  if (p)
  {
    rf_window *window = p;
    rf_unload_render_texture(window->render);
    mrb_container_remove_child(mrb, window->base.container, p);
    mrb_free(mrb, p);
  }
}

#define CURSOR_RECT mrb_intern_lit(mrb, "#cursor_rect")
#define OFFSET mrb_intern_lit(mrb, "#offset")
#define RECT mrb_intern_lit(mrb, "#rect")
#define TONE mrb_intern_lit(mrb, "#tone")
#define SKIN mrb_intern_lit(mrb, "#skin")
#define CONTENTS mrb_intern_lit(mrb, "#contents")
#define VIEWPORT mrb_intern_lit(mrb, "#viewport")
#define PADDING mrb_intern_lit(mrb, "#padding")
#define WINDOW mrb_intern_lit(mrb, "#window")

#define PAUSE_DELAY 0.2

static void no_free(mrb_state *mrb, void *p) {}

const struct mrb_data_type mrb_window_data_type = { "Window", free_window };

const struct mrb_data_type mrb_window_padding_data_type = { "Window::Padding", no_free };


static inline void
draw_cursor(rf_window *window)
{
  if (!window->cursor_rect) return;
  if (!window->skin) return;

  int w = window->cursor_rect->width;
  int h = window->cursor_rect->height;
  if (w <= 0 || h <= 0) return;
  rf_color color = (rf_color){
    255, 255, 255, (unsigned char)(window->opacity * window->cursor_opacity / 255)
  };
  rf_rec dst = *(window->cursor_rect);
  dst.x += window->padding.left;
  dst.y += window->padding.top;
  rf_draw_texture_npatch(
    window->skin->texture, window->skin_rects.cursor, dst,
    (rf_vec2){0, 0}, 0, color
  );
}

static const float corners[4][2] = {
  // Bottom-left corner for texture and quad
  { 0, 1 },
  // Bottom-right corner for texture and quad
  { 0, 0 },
  // Top-right corner for texture and quad
  { 1, 0 },
  // Top-left corner for texture and quad
  { 1, 1 }          
};

static const char * frag =
#if defined(RAYFORK_GRAPHICS_BACKEND_GL_ES3)
"#version 100\n"
"precision mediump float;"
"varying vec2 frag_tex_coord;"
"varying vec4 frag_color;"
#elif defined(RAYFORK_GRAPHICS_BACKEND_GL_33)
"#version 330\n"
"precision mediump float;"
"in vec2 frag_tex_coord;"
"in vec4 frag_color;"
"out vec4 final_color;"
#endif
"uniform sampler2D texture0;"
"uniform vec4 col_diffuse;"
"uniform vec4 tone;"
"void main()"
"{"
#if defined(RAYFORK_GRAPHICS_BACKEND_GL_ES3)
"    vec4 texel_color = texture2D(texture0, frag_tex_coord);" // NOTE: texture2D() is deprecated on OpenGL 3.3 and ES 3.0
#elif defined(RAYFORK_GRAPHICS_BACKEND_GL_33)
"    vec4 texel_color = texture(texture0, frag_tex_coord);"
#endif
"    float ta = 1 - tone.a;"
"    texel_color.r = texel_color.r + tone.r;"
"    texel_color.g = texel_color.g + tone.g;"
"    texel_color.b = texel_color.b + tone.b;"
"    float gray = (0.3 * texel_color.r) + (0.59 * texel_color.g) + (0.11 * texel_color.b);"
"    texel_color.r = texel_color.r * ta + gray * tone.a;"
"    texel_color.g = texel_color.g * ta + gray * tone.a;"
"    texel_color.b = texel_color.b * ta + gray * tone.a;"
#if defined(RAYFORK_GRAPHICS_BACKEND_GL_ES3)
"    frag_color = texel_color*col_diffuse*frag_color;"
#elif defined(RAYFORK_GRAPHICS_BACKEND_GL_33)
"    final_color = texel_color*col_diffuse*frag_color;"
#endif
"}"
;

rf_shader window_shader;
mrb_bool shader_ready = FALSE;
struct
{
  int tone;
} shader_locations;

static void
init_shader(mrb_state *mrb)
{
  window_shader = rf_gfx_load_shader(NULL, frag);
  shader_locations.tone = rf_gfx_get_shader_location(window_shader, "tone");
  shader_ready = TRUE;
}

static inline void
bind_shader(rf_window *window)
{
  float tone[] = {
    (float)window->tone->r / 255.f,
    (float)window->tone->g / 255.f,
    (float)window->tone->b / 255.f,
    (float)window->tone->a / 255.f
  };
  rf_gfx_set_shader_value(window_shader, shader_locations.tone, tone, RF_UNIFORM_VEC4);
}

static void
update_window(mrb_state *mrb, rf_window *window)
{
  int px = window->skin_rects.borders.top_left.width / 6;
  int py = window->skin_rects.borders.top_left.height / 6;
  int w = window->rect->width - px * 2;
  int h = window->rect->height - py * 2;
  if (w != window->render.texture.width || h != window->render.texture.height)
  {
    rf_unload_render_texture(window->render);
    window->render.id = 0;
    if (w && h) window->render = rf_load_render_texture(w, h);
  }
  rf_rec src = window->skin_rects.backgrounds[0];
  rf_rec dst = (rf_rec){0, 0, w, h};
  rf_color color = (rf_color){255, 255, 255, (unsigned char)(window->opacity * window->back_opacity / 255)};
  rf_vec2 origin = *(window->offset);
  rf_gfx_push_matrix();
  rf_begin_render_to_texture(window->render);
    rf_clear(RF_BLANK);
    rf_begin_blend_mode(RF_BLEND_ALPHA);
      if (window->skin)
      {
        mrb_refresh_bitmap(window->skin);
        rf_texture2d texture = window->skin->texture;
        rf_begin_shader(window_shader);
          bind_shader(window);        
          rf_draw_texture_region(texture, src, dst, (rf_vec2){0, 0}, 0, color);
        rf_end_shader();
        src = window->skin_rects.backgrounds[1];
        int tx = w / src.width  + 1;
        int ty = h / src.height + 1;
        for (int y = 0; y < ty; ++y)
        {
          for (int x = 0; x < tx; ++x)
          {
            rf_rec dst2 = (rf_rec){ x * src.width, y * src.height, src.width, src.height };
            rf_draw_texture_region(texture, src, dst2, (rf_vec2){0, 0}, 0, color);
          }
        }
      }
      draw_cursor(window);
      if (window->contents)
      {
        mrb_refresh_bitmap(window->contents);
        rf_color color = (rf_color){255, 255, 255, (unsigned char)(window->opacity * window->contents_opacity / 255)};
        int w2 = w - (window->padding.left + window->padding.right);
        int h2 = h - (window->padding.top + window->padding.bottom);
        if (w2 <= 0 || h2 <= 0) return;
        src = (rf_rec){ window->offset->x, window->offset->y, w2, h2 };
        dst = (rf_rec){ window->padding.left + px, window->padding.top + py, w2, h2 };
        rf_draw_texture_region(window->contents->texture, src, dst, (rf_vec2){0, 0}, 0, color);
      }
    rf_end_blend_mode();
  rf_end_render_to_texture();
  rf_gfx_pop_matrix();
}

static void
draw_contents(rf_window *window)
{
  int w = window->rect->width - window->skin_rects.borders.top_left.width;
  int h = window->rect->height - window->skin_rects.borders.top_left.height;
  if (w <= 0 || h <= 0) return;
  int x = window->skin_rects.borders.top_left.width / 6;
  int y = window->skin_rects.borders.top_left.height / 6;
  w = window->render.texture.width;
  h = window->render.texture.height;
  rf_gfx_enable_texture(window->render.texture.id);
  rf_gfx_push_matrix();
    rf_gfx_translatef(0, h / 2, 0);
    rf_gfx_scalef(1, ((float)window->openness / 255.0f), 1);
    rf_gfx_translatef(0, -h / 2, 0);
    rf_gfx_begin(RF_QUADS);
      rf_gfx_color4ub(255, 255, 255, 255);
      // Bottom-left corner for texture and quad
      rf_gfx_tex_coord2f(corners[0][0], corners[0][1]);
      rf_gfx_vertex2f(0.0f, 0.0f);
      // Bottom-right corner for texture and quad
      rf_gfx_tex_coord2f(corners[1][0], corners[1][1]);
      rf_gfx_vertex2f(0.0f, h);
      // Top-right corner for texture and quad
      rf_gfx_tex_coord2f(corners[2][0], corners[2][1]);
      rf_gfx_vertex2f(w, h);
      // Top-left corner for texture and quad
      rf_gfx_tex_coord2f(corners[3][0], corners[3][1]);
      rf_gfx_vertex2f(w, 0.0f);
    rf_gfx_end();
  rf_gfx_pop_matrix();
  rf_gfx_disable_texture();
}

static inline void
draw_border(rf_window *window)
{
  int w = window->rect->width;
  int h = window->rect->height;
  if (!window->skin) return;

  rf_color color = (rf_color){255, 255, 255, (unsigned char)window->opacity};
  rf_rec src;
  rf_rec dst;
  rf_gfx_push_matrix();
    rf_gfx_translatef(0, h / 2, 0);
    rf_gfx_scalef(1, ((float)window->openness / 255.0f), 1);
    rf_gfx_translatef(0, -h / 2, 0);
    src = window->skin_rects.borders.top_left;
    dst = (rf_rec){ 0, 0, src.width, src.height };
    rf_draw_texture_region(window->skin->texture, src, dst, (rf_vec2){0, 0}, 0, color);
    src = window->skin_rects.borders.top_right;
    dst = (rf_rec){ w - src.width, 0, src.width, src.height };
    rf_draw_texture_region(window->skin->texture, src, dst, (rf_vec2){0, 0}, 0, color);
    src = window->skin_rects.borders.bottom_right;
    dst = (rf_rec){ w - src.width, h - src.height, src.width, src.height };
    rf_draw_texture_region(window->skin->texture, src, dst, (rf_vec2){0, 0}, 0, color);
    src = window->skin_rects.borders.bottom_left;
    dst = (rf_rec){ 0, h - src.height, src.width, src.height };
    rf_draw_texture_region(window->skin->texture, src, dst, (rf_vec2){0, 0}, 0, color);
    int rw = w - (window->skin_rects.borders.top_right.width + window->skin_rects.borders.top_left.width);
    int tx = rw / (int)window->skin_rects.borders.top.width;
    int left = rw - (tx * window->skin_rects.borders.top.width);
    int tw = window->skin_rects.borders.top_left.width;
    for (int x = 0; x < tx; ++x)
    {
      src = window->skin_rects.borders.top;
      dst = (rf_rec){x * src.width + tw, 0, src.width, src.height};
      rf_draw_texture_region(window->skin->texture, src, dst, (rf_vec2){0, 0}, 0, color);
      src = window->skin_rects.borders.bottom;
      dst = (rf_rec){x * src.width + tw, h - src.height, src.width, src.height};
      rf_draw_texture_region(window->skin->texture, src, dst, (rf_vec2){0, 0}, 0, color);
    }
    if (left > 0)
    {
      src = window->skin_rects.borders.top;
      src.width = left;
      dst = (rf_rec){w - tw - left, 0, src.width, src.height};
      rf_draw_texture_region(window->skin->texture, src, dst, (rf_vec2){0, 0}, 0, color);
      src = window->skin_rects.borders.bottom;
      src.width = left;
      dst = (rf_rec){w - tw - left, h - src.height, src.width, src.height};
      rf_draw_texture_region(window->skin->texture, src, dst, (rf_vec2){0, 0}, 0, color);
    }
    int rh = h - (window->skin_rects.borders.top_left.height + window->skin_rects.borders.bottom_left.height);
    int ty = rh / (int)window->skin_rects.borders.left.height;
    int th = window->skin_rects.borders.top_left.height;
    for (int y = 0; y < ty; ++y)
    {
      src = window->skin_rects.borders.left;
      dst = (rf_rec){0, y * src.height + th, src.width, src.height};
      rf_draw_texture_region(window->skin->texture, src, dst, (rf_vec2){0, 0}, 0, color);
      src = window->skin_rects.borders.right;
      dst = (rf_rec){w - src.width, y * src.height + th, src.width, src.height};
      rf_draw_texture_region(window->skin->texture, src, dst, (rf_vec2){0, 0}, 0, color);
    }
    left = rh - (ty * window->skin_rects.borders.left.height);
    if (left > 0)
    {
      src = window->skin_rects.borders.left;
      src.height = left;
      dst = (rf_rec){0, h - left - th, src.width, src.height};
      rf_draw_texture_region(window->skin->texture, src, dst, (rf_vec2){0, 0}, 0, color);
      src = window->skin_rects.borders.right;
      src.height = left;
      dst = (rf_rec){w - src.width, h - left - th, src.width, src.height};
      rf_draw_texture_region(window->skin->texture, src, dst, (rf_vec2){0, 0}, 0, color);
    }
  rf_gfx_pop_matrix();
}

static void
draw_pause_cursor(rf_window *window)
{
  if (!window->pause) return;
  mrb_int i = window->pause_frame;
  rf_rec src = window->skin_rects.pause[i];
  rf_rec dst = (rf_rec){
    (window->rect->width - src.width) / 2,
    window->rect->height - src.height,
    src.width,
    src.height
  };
  rf_color color = (rf_color){255, 255, 255, window->opacity };
  rf_draw_texture_region(window->skin->texture, src, dst, (rf_vec2){0, 0}, 0, color);
}

static void
draw_arrows(rf_window *window)
{
  if (!window->arrows_visible) return;
}

static void
draw_cursors(rf_window *window)
{
  draw_arrows(window);
  draw_pause_cursor(window);
}

static void
draw_window(mrb_state *mrb, rf_window *window)
{
  if (window->rect->width <= 0 || window->rect->height <= 0) return;
  rf_begin_blend_mode(RF_BLEND_ALPHA);
    rf_gfx_translatef(window->rect->x, window->rect->y, 0);
    draw_contents(window);
    draw_border(window);
    draw_cursors(window);
  rf_end_blend_mode();
}

static mrb_value
new_padding(mrb_state *mrb, mrb_value self, rf_window_padding *padding)
{
  struct RClass *window = mrb_class_get(mrb, "Window");
  mrb_value result = mrb_obj_new(mrb, mrb_class_get_under(mrb, window, "Padding"), 1, &self);
  DATA_PTR(result) = padding;
  return result;
}

static mrb_value
padding_initialize(mrb_state *mrb, mrb_value self)
{
  mrb_value window;
  mrb_get_args(mrb, "o", &window);
  if (!mrb_window_p(window)) mrb_raise(mrb, E_ARGUMENT_ERROR, "value is not a Window");
  DATA_TYPE(self) = &mrb_window_padding_data_type;
  mrb_iv_set(mrb, self, WINDOW, window);
  return self;
}

static mrb_value
padding_get_top(mrb_state *mrb, mrb_value self)
{
  mrb_get_window(mrb, mrb_iv_get(mrb, self, WINDOW));
  rf_window_padding *padding = (rf_window_padding *)DATA_PTR(self);
  return mrb_fixnum_value(padding->top);
}

static mrb_value
padding_get_bottom(mrb_state *mrb, mrb_value self)
{
  mrb_get_window(mrb, mrb_iv_get(mrb, self, WINDOW));
  rf_window_padding *padding = (rf_window_padding *)DATA_PTR(self);
  return mrb_fixnum_value(padding->bottom);
}

static mrb_value
padding_get_left(mrb_state *mrb, mrb_value self)
{
  mrb_get_window(mrb, mrb_iv_get(mrb, self, WINDOW));
  rf_window_padding *padding = (rf_window_padding *)DATA_PTR(self);
  return mrb_fixnum_value(padding->left);
}

static mrb_value
padding_get_right(mrb_state *mrb, mrb_value self)
{
  mrb_get_window(mrb, mrb_iv_get(mrb, self, WINDOW));
  rf_window_padding *padding = (rf_window_padding *)DATA_PTR(self);
  return mrb_fixnum_value(padding->right);
}

static mrb_value
padding_set_top(mrb_state *mrb, mrb_value self)
{
  mrb_get_window(mrb, mrb_iv_get(mrb, self, WINDOW));
  rf_window_padding *padding = (rf_window_padding *)DATA_PTR(self);
  mrb_int value;
  mrb_get_args(mrb, "i", &value);
  padding->top = value < 0 ? 0 : value;
  return mrb_fixnum_value(padding->top);
}

static mrb_value
padding_set_bottom(mrb_state *mrb, mrb_value self)
{
  mrb_get_window(mrb, mrb_iv_get(mrb, self, WINDOW));
  rf_window_padding *padding = (rf_window_padding *)DATA_PTR(self);
  mrb_int value;
  mrb_get_args(mrb, "i", &value);
  padding->bottom = value < 0 ? 0 : value;
  return mrb_fixnum_value(padding->top);
}

static mrb_value
padding_set_left(mrb_state *mrb, mrb_value self)
{
  mrb_get_window(mrb, mrb_iv_get(mrb, self, WINDOW));
  rf_window_padding *padding = (rf_window_padding *)DATA_PTR(self);
  mrb_int value;
  mrb_get_args(mrb, "i", &value);
  padding->left = value < 0 ? 0 : value;
  return mrb_fixnum_value(padding->top);
}

static mrb_value
padding_set_right(mrb_state *mrb, mrb_value self)
{
  mrb_get_window(mrb, mrb_iv_get(mrb, self, WINDOW));
  rf_window_padding *padding = (rf_window_padding *)DATA_PTR(self);
  mrb_int value;
  mrb_get_args(mrb, "i", &value);
  padding->right = value < 0 ? 0 : value;
  return mrb_fixnum_value(padding->top);
}

static mrb_value
window_initialize(mrb_state *mrb, mrb_value self)
{
  if (!shader_ready)
  {
    init_shader(mrb);
    shader_ready = TRUE;
  }
  DATA_TYPE(self) = &mrb_window_data_type;
  rf_window *window = mrb_malloc(mrb, sizeof *window);
  mrb_value cursor_rect = mrb_rect_new(mrb, 0, 0 , 0, 0);
  mrb_value offset = mrb_point_new(mrb, 0, 0);
  mrb_value rect = mrb_rect_new(mrb, 0, 0, 0, 0);
  mrb_value tone = mrb_tone_neutral(mrb);
  DATA_PTR(self) = window;
  window->base.z = 0;
  window->base.container = NULL;
  window->base.update = NULL;
  window->base.visible = TRUE;
  window->base.update = (rf_drawable_update_callback)update_window;
  window->base.draw = (rf_drawable_draw_callback)draw_window;
  window->active = TRUE;
  window->arrows_visible = FALSE;
  window->contents = NULL;
  window->contents_opacity = 255;
  window->cursor_opacity   = 255;
  window->back_opacity     = 255;
  window->cursor_rect = mrb_get_rect(mrb, cursor_rect);
  window->offset = mrb_get_point(mrb, offset);
  window->opacity = 255;
  window->openness = 255;
  window->padding = (rf_window_padding){12, 12, 12, 12};
  window->pause = FALSE;
  window->rect = mrb_get_rect(mrb, rect);
  window->skin = NULL;
  window->tone = mrb_get_tone(mrb, tone);
  window->pause_frame = 0;
  window->pause_count = 0;
  window->cursor_opacity = 55;
  mrb_int argc = mrb_get_argc(mrb);
  mrb_iv_set(mrb, self, CURSOR_RECT, cursor_rect);
  mrb_iv_set(mrb, self, OFFSET, offset);
  mrb_iv_set(mrb, self, RECT, rect);
  mrb_iv_set(mrb, self, TONE, tone);
  mrb_iv_set(mrb, self, SKIN, mrb_nil_value());
  mrb_iv_set(mrb, self, CONTENTS, mrb_nil_value());
  mrb_iv_set(mrb, self, VIEWPORT, mrb_nil_value());
  mrb_iv_set(mrb, self, PADDING, new_padding(mrb, self, &(window->padding)));
  switch (argc)
  {
    case 0: case 2: case 3: case 4:
    {
      mrb_float x = 0, y = 0, w = 0, h = 0;
      mrb_get_args(mrb, "|ffff", &x, &y, &w, &h);
      if (w < 0) mrb_raise(mrb, E_ARGUMENT_ERROR, "width must be positive");
      if (h < 0) mrb_raise(mrb, E_ARGUMENT_ERROR, "height must be positive.");
      window->rect->x = (float)x;
      window->rect->y = (float)y;
      window->rect->width = (float)w;
      window->rect->height = (float)h;
      break;
    }
    case 1:
    {
      mrb_value other;
      mrb_get_args(mrb, "o", &other);
      if (mrb_fixnum_p(other) || mrb_float_p(other))
      {
        window->rect->x = (float)mrb_float(mrb_Float(mrb, other));
      }
      else if (mrb_rect_p(other))
      {
        *(window->rect) = *mrb_get_rect(mrb, other);
      }
      else
      {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "argument is neither a Numeric not a Rect");
      }
      break;
    }
    default:
    {
      mrb_raisef(mrb, E_ARGUMENT_ERROR, "wrong number of arguments expected 0..4 but %d was given.", argc);
      break;
    }
  }
  int w = (int)max(0, window->rect->width - 24);
  int h = (int)max(0, window->rect->height - 24);
  if (w && h)
  {
    window->render = rf_load_render_texture(w, h);
  }
  else
  {
    window->render.id = 0;
  }
  mrb_container_add_child(mrb, mrb_get_graphics_container(mrb), &(window->base));
  return self;
}

static mrb_value
window_get_cursor_rect(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, CURSOR_RECT);
}

static mrb_value
window_get_offset(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, OFFSET);
}

static mrb_value
window_get_rect(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, RECT);
}

static mrb_value
window_get_tone(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, TONE);
}

static mrb_value
window_disposedQ(mrb_state *mrb, mrb_value self)
{
  return mrb_bool_value(DATA_PTR(self) ? 0 : 1);
}

static mrb_value
window_dispose(mrb_state *mrb, mrb_value self)
{
  rf_window *window = mrb_get_window(mrb, self);
  free_window(mrb, window);
  DATA_PTR(self) = NULL;
  return mrb_nil_value();
}

static mrb_value
window_get_z(mrb_state *mrb, mrb_value self)
{
  rf_window *window = mrb_get_window(mrb, self);
  return mrb_fixnum_value(window->base.z);
}

static mrb_value
window_set_z(mrb_state *mrb, mrb_value self)
{
  mrb_int value;
  rf_window *window = mrb_get_window(mrb, self);
  mrb_get_args(mrb, "i", &value);
  if (window->base.z != value)
  {
    window->base.z = value;
    mrb_container_invalidate(mrb, window->base.container);
  }
  return mrb_fixnum_value(window->base.z);
}

static mrb_value
window_get_viewport(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, VIEWPORT);
}

static mrb_value
window_set_viewport(mrb_state *mrb, mrb_value self)
{
  mrb_value parent_value;
  mrb_get_args(mrb, "o", &parent_value);
  rf_window *window = mrb_get_window(mrb, self);
  if (mrb_nil_p(parent_value))
  {
    mrb_container_add_child(mrb, mrb_get_graphics_container(mrb), &(window->base));
    mrb_iv_set(mrb, self, VIEWPORT, parent_value);
    return parent_value;
  }
  if (!mrb_viewport_p(parent_value))
  {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "value is not a Viewport");
  }
  rf_viewport *view = mrb_get_viewport(mrb, parent_value);
  mrb_container_add_child(mrb, &(view->base), &(window->base));
  mrb_iv_set(mrb, self, VIEWPORT, parent_value);
  return parent_value;
}

static mrb_value
window_get_skin(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, SKIN);
}

static void
set_skin_rects(rf_window *window, rf_bitmap *skin)
{
  float w = skin->texture.width;
  float h = skin->texture.height;
  float x = w / 24;
  float y = h / 24;
  float x3 = x * 3;
  float y3 = y * 3;
  float hw = w / 2;
  float hh = h / 2;
  float x3_2 = x * 3 / 2;
  float y3_2 = y * 3 / 2;
  float x4 = x * 3;
  float y4 = y * 4;
  float x6 = x * 6;
  float y6 = y * 6;
  float x9 = x * 9;
  float y9 = y * 9;

  // backgrounds
  window->skin_rects.backgrounds[0] = (rf_rec){0,  0, hw, hh};
  window->skin_rects.backgrounds[1] = (rf_rec){0, hh, hw, hh};

  // window arrows
  window->skin_rects.arrows.top    = (rf_rec){       hw + x4,             y3,    x4, y3_2 };
  window->skin_rects.arrows.bottom = (rf_rec){       hw + x4, hh - y3 - y3_2,    x4, y3_2 };
  window->skin_rects.arrows.left   = (rf_rec){       hw + x3,             y3,  x3_2,   y4 };
  window->skin_rects.arrows.right  = (rf_rec){ w - x3 - x3_2,             y3,  x3_2,   y4 };

  // skin borders
  window->skin_rects.borders.top_left      = (rf_rec){      hw,       0, x3, y3 };
  window->skin_rects.borders.top_right     = (rf_rec){  w - x3,       0, x3, y3 };
  window->skin_rects.borders.bottom_left   = (rf_rec){      hw, hh - x3, x3, y3 };
  window->skin_rects.borders.bottom_right  = (rf_rec){  w - x3, hh - x3, x3, y3 };
  window->skin_rects.borders.top           = (rf_rec){ hw + x3,       0, x6, y3 };
  window->skin_rects.borders.bottom        = (rf_rec){ hw + x3, hh - x3, x6, y3 };
  window->skin_rects.borders.left          = (rf_rec){      hw,      x3, x3, y6 };
  window->skin_rects.borders.right         = (rf_rec){  w - x3,      x3, x3, y6 };

  // command cursor
  window->skin_rects.cursor.type = RF_NPT_9PATCH;
  window->skin_rects.cursor.top = x / 2;
  window->skin_rects.cursor.bottom = x / 2;
  window->skin_rects.cursor.left = x / 2;
  window->skin_rects.cursor.right = x / 2;
  window->skin_rects.cursor.source_rec = (rf_rec){ hw, hh, x6, y6 };

  // pause cursor
  window->skin_rects.pause[0] = (rf_rec){ hw + x6,      hh, x3, y3 };
  window->skin_rects.pause[1] = (rf_rec){ hw + x9,      hh, x3, y3 };
  window->skin_rects.pause[2] = (rf_rec){ hw + x6, hh + y3, x3, y3 };
  window->skin_rects.pause[3] = (rf_rec){ hw + x9, hh + y3, x3, y3 };
}

static mrb_value
window_set_skin(mrb_state *mrb, mrb_value self)
{
  mrb_value skin_value;
  mrb_get_args(mrb, "o", &skin_value);
  rf_window *window = mrb_get_window(mrb, self);
  if (mrb_nil_p(skin_value))
  {
    window->skin = NULL;
    mrb_iv_set(mrb, self, SKIN, skin_value);
    return skin_value;
  }
  if (!mrb_bitmap_p(skin_value)) mrb_raise(mrb, E_ARGUMENT_ERROR, "value is not a Bitmap.");
  window->skin = mrb_get_bitmap(mrb, skin_value);
  set_skin_rects(window, window->skin);
  mrb_iv_set(mrb, self, SKIN, skin_value);
  return skin_value;
}

static mrb_value
window_get_contents(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, CONTENTS);
}

static mrb_value
window_set_contents(mrb_state *mrb, mrb_value self)
{
  mrb_value contents_value;
  mrb_get_args(mrb, "o", &contents_value);
  rf_window *window = mrb_get_window(mrb, self);
  if (mrb_nil_p(contents_value))
  {
    window->contents = NULL;
    mrb_iv_set(mrb, self, CONTENTS, contents_value);
    return contents_value;
  }
  if (!mrb_bitmap_p(contents_value)) mrb_raise(mrb, E_ARGUMENT_ERROR, "value is not a Bitmap.");
  window->contents = mrb_get_bitmap(mrb, contents_value);
  mrb_iv_set(mrb, self, SKIN, contents_value);
  return contents_value;
}

static mrb_value
window_get_visible(mrb_state *mrb, mrb_value self)
{
  rf_window *window = mrb_get_window(mrb, self);
  return mrb_bool_value(window->base.visible);
}

static mrb_value
window_set_visible(mrb_state *mrb, mrb_value self)
{
  mrb_bool value;
  rf_window *window = mrb_get_window(mrb, self);
  mrb_get_args(mrb, "b", &value);
  window->base.visible = value;
  return mrb_bool_value(window->base.visible);
}

static mrb_value
window_get_openness(mrb_state *mrb, mrb_value self)
{
  rf_window *window = mrb_get_window(mrb, self);
  return mrb_fixnum_value(window->openness);
}

static mrb_value
window_set_openness(mrb_state *mrb, mrb_value self)
{
  mrb_int value;
  rf_window *window = mrb_get_window(mrb, self);
  mrb_get_args(mrb, "i", &value);
  window->openness = value < 0 ? 0 : (value > 255 ? 255 : value);
  return mrb_fixnum_value(window->base.z);
}

static mrb_value
window_get_pause(mrb_state *mrb, mrb_value self)
{
  rf_window *window = mrb_get_window(mrb, self);
  return mrb_bool_value(window->pause);
}

static mrb_value
window_set_pause(mrb_state *mrb, mrb_value self)
{
  mrb_bool value;
  rf_window *window = mrb_get_window(mrb, self);
  mrb_get_args(mrb, "b", &value);
  window->pause = value;
  return mrb_bool_value(window->pause);
}

static mrb_value
window_get_active(mrb_state *mrb, mrb_value self)
{
  rf_window *window = mrb_get_window(mrb, self);
  return mrb_bool_value(window->active);
}

static mrb_value
window_set_active(mrb_state *mrb, mrb_value self)
{
  mrb_bool value;
  rf_window *window = mrb_get_window(mrb, self);
  mrb_get_args(mrb, "b", &value);
  window->active = value;
  return mrb_bool_value(window->pause);
}


static mrb_value
window_update(mrb_state *mrb, mrb_value self)
{
  rf_window *window = mrb_get_window(mrb, self);
  if (!window->active) return;
  mrb_float dt = mrb_get_dt(mrb);
  window->cursor_count += dt;
  if (window->cursor_count > 2 * RF_PI) window->cursor_count -= 2 * RF_PI; 
  window->cursor_opacity = 55 + (mrb_int)(200.0 * fabs(sin(window->cursor_count)));
  window->pause_count += dt;
  if (window->pause_count >= PAUSE_DELAY)
  {
    window->pause_count -= PAUSE_DELAY;
    window->pause_frame = (window->pause_frame + 1) % 4;
  }
  return mrb_nil_value();
}

void
mrb_init_ogss_window(mrb_state *mrb)
{
  struct RClass *window = mrb_define_class(mrb, "Window", mrb->object_class);
  MRB_SET_INSTANCE_TT(window, MRB_TT_DATA);

  mrb_define_method(mrb, window, "initialize", window_initialize, MRB_ARGS_OPT(4));

  mrb_define_method(mrb, window, "cursor_rect", window_get_cursor_rect, MRB_ARGS_NONE());
  mrb_define_method(mrb, window, "offset", window_get_offset, MRB_ARGS_NONE());
  mrb_define_method(mrb, window, "rect", window_get_rect, MRB_ARGS_NONE());
  mrb_define_method(mrb, window, "tone", window_get_tone, MRB_ARGS_NONE());

  mrb_define_method(mrb, window, "disposed?", window_disposedQ, MRB_ARGS_NONE());
  mrb_define_method(mrb, window, "dispose", window_dispose, MRB_ARGS_NONE());

  mrb_define_method(mrb, window, "z", window_get_z, MRB_ARGS_NONE());
  mrb_define_method(mrb, window, "z=", window_set_z, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, window, "viewport", window_get_viewport, MRB_ARGS_NONE());
  mrb_define_method(mrb, window, "viewport=", window_set_viewport, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, window, "skin", window_get_skin, MRB_ARGS_NONE());
  mrb_define_method(mrb, window, "skin=", window_set_skin, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, window, "windowskin", window_get_skin, MRB_ARGS_NONE());
  mrb_define_method(mrb, window, "windowskin=", window_set_skin, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, window, "contents", window_get_contents, MRB_ARGS_NONE());
  mrb_define_method(mrb, window, "contents=", window_set_contents, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, window, "visible", window_get_visible, MRB_ARGS_NONE());
  mrb_define_method(mrb, window, "visible?", window_get_visible, MRB_ARGS_NONE());
  mrb_define_method(mrb, window, "visible=", window_set_visible, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, window, "pause", window_get_pause, MRB_ARGS_NONE());
  mrb_define_method(mrb, window, "pause?", window_get_pause, MRB_ARGS_NONE());
  mrb_define_method(mrb, window, "pause=", window_set_pause, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, window, "active", window_get_active, MRB_ARGS_NONE());
  mrb_define_method(mrb, window, "active?", window_get_active, MRB_ARGS_NONE());
  mrb_define_method(mrb, window, "active=", window_set_active, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, window, "openness", window_get_openness, MRB_ARGS_NONE());
  mrb_define_method(mrb, window, "openness=", window_set_openness, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, window, "update", window_update, MRB_ARGS_NONE());

  struct RClass *padding = mrb_define_class_under(mrb, window, "Padding", mrb->object_class);
  MRB_SET_INSTANCE_TT(padding, MRB_TT_DATA);

  mrb_define_method(mrb, padding, "initialize", padding_initialize, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, padding, "top", padding_get_top, MRB_ARGS_NONE());
  mrb_define_method(mrb, padding, "bottom", padding_get_bottom, MRB_ARGS_NONE());
  mrb_define_method(mrb, padding, "left", padding_get_left, MRB_ARGS_NONE());
  mrb_define_method(mrb, padding, "right", padding_get_right, MRB_ARGS_NONE());

  mrb_define_method(mrb, padding, "top=", padding_set_top, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, padding, "bottom=", padding_set_bottom, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, padding, "left=", padding_set_left, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, padding, "right=", padding_set_right, MRB_ARGS_REQ(1));
}
