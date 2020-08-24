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
#include <ogss/sprite.h>
#include <ogss/viewport.h>
#include <ogss/graphics.h>

#define BITMAP mrb_intern_cstr(mrb, "#bitmap")
#define SRC_RECT mrb_intern_cstr(mrb, "#src_rect")
#define POSITION mrb_intern_cstr(mrb, "#position")
#define ANCHOR mrb_intern_cstr(mrb, "#anchor")
#define SCALE mrb_intern_cstr(mrb, "#scale")
#define COLOR mrb_intern_cstr(mrb, "#color")
#define TONE mrb_intern_cstr(mrb, "#tone")
#define VIEWPORT mrb_intern_cstr(mrb, "#viewport")

static void
free_sprite(mrb_state *mrb, void *p)
{
  if (p)
  {
    rf_sprite *sprite = p;
    mrb_container_remove_child(mrb, sprite->base.container, p);
    mrb_free(mrb, sprite);
  }
}

const struct mrb_data_type mrb_sprite_data_type = { "Sprite", free_sprite };

static rf_shader sprite_shader;
static mrb_bool shader_ready = FALSE;

static struct
{
  int flash_color, bush, tone;
} shader_locations;

static const char * sprite_fshader =
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
"uniform vec4 flash_color;"
"uniform vec4 tone;"
"uniform vec2 bush;"
"void main()"
"{"
#if defined(RAYFORK_GRAPHICS_BACKEND_GL_ES3)
"    vec4 texel_color = texture2D(texture0, frag_tex_coord);" // NOTE: texture2D() is deprecated on OpenGL 3.3 and ES 3.0
#elif defined(RAYFORK_GRAPHICS_BACKEND_GL_33)
"    vec4 texel_color = texture(texture0, frag_tex_coord);"
#endif
"    float bush_op = 1;"
"    if (frag_tex_coord.y > bush.y) bush_op = bush.x;"
"    float a = 1 - flash_color.a;"
"    float ta = 1 - tone.a;"
"    texel_color.r = texel_color.r + tone.r;"
"    texel_color.g = texel_color.g + tone.g;"
"    texel_color.b = texel_color.b + tone.b;"
"    float gray = (0.3 * texel_color.r) + (0.59 * texel_color.g) + (0.11 * texel_color.b);"
"    texel_color.r = texel_color.r * a + flash_color.r * flash_color.a;"
"    texel_color.g = texel_color.g * a + flash_color.g * flash_color.a;"
"    texel_color.b = texel_color.b * a + flash_color.b * flash_color.a;"
"    texel_color.r = texel_color.r * ta + gray * tone.a;"
"    texel_color.g = texel_color.g * ta + gray * tone.a;"
"    texel_color.b = texel_color.b * ta + gray * tone.a;"
"    texel_color.a = texel_color.a * bush_op;"
#if defined(RAYFORK_GRAPHICS_BACKEND_GL_ES3)
"    frag_color = texel_color*col_diffuse*frag_color;"
#elif defined(RAYFORK_GRAPHICS_BACKEND_GL_33)
"    final_color = texel_color*col_diffuse*frag_color;"
#endif
"}"
;

static void
init_shader(mrb_state *mrb)
{
  rf_get_default_shader();
  sprite_shader = rf_gfx_load_shader(NULL, sprite_fshader);
  shader_locations.flash_color = rf_gfx_get_shader_location(sprite_shader, "flash_color");
  shader_locations.bush = rf_gfx_get_shader_location(sprite_shader, "bush");
  shader_locations.tone = rf_gfx_get_shader_location(sprite_shader, "tone");
  shader_ready = TRUE;
}

static inline void
bind_shader(rf_sprite *sprite)
{
  float rgba[] = {
    (float)sprite->flash_color.r / 255.0f, 
    (float)sprite->flash_color.g / 255.0f, 
    (float)sprite->flash_color.b / 255.0f, 
    (float)sprite->flash_color.a / 255.0f,
  };
  float tone[] = {
    (float)sprite->tone->r / 255.f,
    (float)sprite->tone->g / 255.f,
    (float)sprite->tone->b / 255.f,
    (float)sprite->tone->a / 255.f
  };
  float h = sprite->src_rect->height;
  h = (sprite->src_rect->height - sprite->bush.y) / h;
  float th = sprite->bitmap->texture.height;
  float bush[] = {
    sprite->bush.x,
    (sprite->src_rect->y + h * sprite->src_rect->height) / th
  };
  rf_gfx_set_shader_value(sprite_shader, shader_locations.flash_color, rgba, RF_UNIFORM_VEC4);
  rf_gfx_set_shader_value(sprite_shader, shader_locations.tone, tone, RF_UNIFORM_VEC4);
  rf_gfx_set_shader_value(sprite_shader, shader_locations.bush, bush, RF_UNIFORM_VEC2);
}

static inline void
swap_point(float *p1, float *p2)
{
  float tmp[2] = { p1[0], p1[1] };
  p1[0] = p2[0];
  p1[1] = p2[1];
  p2[0] = tmp[0];
  p2[1] = tmp[1];
}

static void
rf_draw_sprite(mrb_state *mrb, rf_sprite *sprite)
{
  if (!sprite->bitmap) return;

  mrb_refresh_bitmap(sprite->bitmap);
  rf_texture2d texture = sprite->bitmap->texture;
  if (texture.id <= 0) return;
  if (!texture.valid) return;

  mrb_bool flip_x = FALSE, flip_y = FALSE;

  float sx = sprite->scale->x, sy = sprite->scale->y;

  if (!sx || !sy) return;

  if (sx < 0) { flip_x = true; sx *= -1; }
  if (sy < 0) { flip_y = true; sy *= -1; }

  rf_rec src = *(sprite->src_rect);
  float width  = (float)texture.width;
  float height = (float)texture.height;
  rf_rec dst = (rf_rec){
    sprite->position->x, sprite->position->y, src.width * sx, src.height * sy
  };

  float corners[4][2] = {
    // Bottom-left corner for texture and quad
    { src.x/width, src.y/height },
    // Bottom-right corner for texture and quad
    { src.x/width, (src.y + src.height)/height },
    // Top-right corner for texture and quad
    { (src.x + src.width)/width, (src.y + src.height)/height },
    // Top-left corner for texture and quad
    { (src.x + src.width)/width, src.y/height }          
  };

  if (flip_x)
  {
    src.width *= -1;
    swap_point(corners[0], corners[3]);
    swap_point(corners[1], corners[2]);
  }
  if (flip_y)
  {
    src.height *= -1;
    swap_point(corners[0], corners[1]);
    swap_point(corners[2], corners[3]);
  }

  float ox = sprite->anchor->x * dst.width;
  float oy = sprite->anchor->y * dst.height;

  rf_color color = *(sprite->color);

  if (!color.a) return;

  rf_gfx_enable_texture(texture.id);
  rf_gfx_push_matrix();
    rf_begin_blend_mode(sprite->blend_mode);
    rf_gfx_translatef(dst.x, dst.y, 0);
    rf_gfx_rotatef(sprite->rotation, 0, 0, 1);
    rf_gfx_translatef(-ox, -oy, 0);
    rf_begin_shader(sprite_shader);
    bind_shader(sprite);
    rf_gfx_begin(RF_QUADS);
      rf_gfx_color4ub(color.r, color.g, color.b, color.a);
      // Bottom-left corner for texture and quad
      rf_gfx_tex_coord2f(corners[0][0], corners[0][1]);
      rf_gfx_vertex2f(0.0f, 0.0f);
      // Bottom-right corner for texture and quad
      rf_gfx_tex_coord2f(corners[1][0], corners[1][1]);
      rf_gfx_vertex2f(0.0f, dst.height);
      // Top-right corner for texture and quad
      rf_gfx_tex_coord2f(corners[2][0], corners[2][1]);
      rf_gfx_vertex2f(dst.width, dst.height);
      // Top-left corner for texture and quad
      rf_gfx_tex_coord2f(corners[3][0], corners[3][1]);
      rf_gfx_vertex2f(dst.width, 0.0f);
    rf_gfx_end();
    rf_end_shader();
    rf_end_blend_mode();
  rf_gfx_pop_matrix();
  rf_gfx_disable_texture();
}

static mrb_value
mrb_sprite_initialize(mrb_state *mrb, mrb_value self)
{
  DATA_TYPE(self) = &mrb_sprite_data_type;
  rf_sprite *sprite = mrb_malloc(mrb, sizeof *sprite);
  DATA_PTR(self) = sprite;
  rf_container *parent;
  if (!shader_ready)
  {
    init_shader(mrb);
  }
  sprite->base.container = NULL;
  sprite->base.z = 0;
  sprite->base.draw = (rf_drawable_draw_callback)rf_draw_sprite;
  sprite->base.update = NULL;
  sprite->base.visible = TRUE;
  sprite->bitmap = NULL;
  sprite->rotation = 0;
  sprite->blend_mode = RF_BLEND_ALPHA;
  sprite->flash_time = 0;
  sprite->flash_color.a = 0;
  sprite->bush.y = 0;
  sprite->bush.x = 0.5;
  sprite->wave_amp = sprite->wave_length = sprite->wave_phase = sprite->wave_speed = 0;
  mrb_value position = mrb_point_new(mrb, 0, 0);
  mrb_value anchor = mrb_point_new(mrb, 0, 0);
  mrb_value scale = mrb_point_new(mrb, 1 , 1);
  mrb_value color = mrb_color_white(mrb);
  mrb_value src_rect = mrb_rect_new(mrb, 0, 0, 0, 0);
  mrb_value tone = mrb_tone_neutral(mrb);
  sprite->position = mrb_get_point(mrb, position);
  sprite->anchor = mrb_get_point(mrb, anchor);
  sprite->scale = mrb_get_point(mrb, scale);
  sprite->color = mrb_get_color(mrb, color);
  sprite->src_rect = mrb_get_rect(mrb, src_rect);
  sprite->tone = mrb_get_tone(mrb, tone);
  mrb_iv_set(mrb, self, POSITION, position);
  mrb_iv_set(mrb, self, ANCHOR, anchor);
  mrb_iv_set(mrb, self, SCALE, scale);
  mrb_iv_set(mrb, self, COLOR, color);
  mrb_iv_set(mrb, self, TONE, tone);
  mrb_iv_set(mrb, self, BITMAP, mrb_nil_value());
  mrb_iv_set(mrb, self, SRC_RECT, src_rect);
  mrb_value parent_value = mrb_nil_value();
  mrb_int argc = mrb_get_args(mrb, "|o", &parent_value);
  if (argc && !mrb_nil_p(parent_value))
  {
    if (!mrb_viewport_p(parent_value)) mrb_raise(mrb, E_ARGUMENT_ERROR, "parent must be a Viewport");
    rf_viewport *viewport = mrb_get_viewport(mrb, parent_value);
    parent = &(viewport->base);
    mrb_iv_set(mrb, self, VIEWPORT, parent_value);
  }
  else
  {
    parent = mrb_get_graphics_container(mrb);
    mrb_iv_set(mrb, self, VIEWPORT, mrb_nil_value());
  }
  mrb_container_add_child(mrb, parent, &(sprite->base));
  return self;
}

static mrb_value
mrb_sprite_disposedQ(mrb_state *mrb, mrb_value self)
{
  return mrb_bool_value(DATA_PTR(self) ? 0 : 1);
}

static mrb_value
mrb_sprite_dispose(mrb_state *mrb, mrb_value self)
{
  rf_sprite *sprite = mrb_get_sprite(mrb, self);
  free_sprite(mrb, sprite);
  DATA_PTR(self) = NULL;
  return mrb_nil_value();
}

static mrb_value
mrb_sprite_get_z(mrb_state *mrb, mrb_value self)
{
  rf_sprite *sprite = mrb_get_sprite(mrb, self);
  return mrb_fixnum_value(sprite->base.z);
}

static mrb_value
mrb_sprite_set_z(mrb_state *mrb, mrb_value self)
{
  mrb_int value;
  rf_sprite *sprite = mrb_get_sprite(mrb, self);
  mrb_get_args(mrb, "i", &value);
  if (sprite->base.z != value)
  {
    sprite->base.z = value;
    mrb_container_invalidate(mrb, sprite->base.container);
  }
  return mrb_fixnum_value(value);
}

static mrb_value
mrb_sprite_get_visible(mrb_state *mrb, mrb_value self)
{
  rf_sprite *sprite = mrb_get_sprite(mrb, self);
  return mrb_bool_value(sprite->base.visible);
}

static mrb_value
mrb_sprite_set_visible(mrb_state *mrb, mrb_value self)
{
  mrb_bool value;
  rf_sprite *sprite = mrb_get_sprite(mrb, self);
  mrb_get_args(mrb, "b", &value); 
  sprite->base.visible = value;
  return mrb_bool_value(value);
}

static mrb_value
mrb_sprite_get_rotation(mrb_state *mrb, mrb_value self)
{
  rf_sprite *sprite = mrb_get_sprite(mrb, self);
  return mrb_float_value(mrb, sprite->rotation);
}

static mrb_value
mrb_sprite_set_rotation(mrb_state *mrb, mrb_value self)
{
  mrb_float value;
  rf_sprite *sprite = mrb_get_sprite(mrb, self);
  mrb_get_args(mrb, "f", &value); 
  sprite->rotation = (float)value;
  return mrb_float_value(mrb, value);
}

static mrb_value
mrb_sprite_get_blend_mode(mrb_state *mrb, mrb_value self)
{
  rf_sprite *sprite = mrb_get_sprite(mrb, self);
  return mrb_fixnum_value(sprite->blend_mode);
}

static mrb_value
mrb_sprite_set_blend_mode(mrb_state *mrb, mrb_value self)
{
  mrb_int value;
  rf_sprite *sprite = mrb_get_sprite(mrb, self);
  mrb_get_args(mrb, "i", &value); 
  sprite->blend_mode = (rf_blend_mode)value;
  return mrb_fixnum_value(value);
}

static mrb_value
mrb_sprite_get_viewport(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, VIEWPORT);
}

static mrb_value
mrb_sprite_set_viewport(mrb_state *mrb, mrb_value self)
{
  mrb_value parent_value;
  mrb_get_args(mrb, "o", &parent_value);
  rf_sprite *sprite = mrb_get_sprite(mrb, self);
  if (mrb_nil_p(parent_value))
  {
    mrb_container_add_child(mrb, mrb_get_graphics_container(mrb), &(sprite->base));
    mrb_iv_set(mrb, self, VIEWPORT, parent_value);
    return parent_value;
  }
  if (!mrb_viewport_p(parent_value))
  {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "value is not a Viewport");
  }
  rf_viewport *view = mrb_get_viewport(mrb, parent_value);
  mrb_container_add_child(mrb, &(view->base), &(sprite->base));
  mrb_iv_set(mrb, self, VIEWPORT, parent_value);
  return parent_value;
}

static mrb_value
mrb_sprite_get_bitmap(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, BITMAP);
}

static mrb_value
mrb_sprite_set_bitmap(mrb_state *mrb, mrb_value self)
{
  mrb_value value;
  rf_sprite *sprite = mrb_get_sprite(mrb, self);
  mrb_get_args(mrb, "o", &value);
  if (mrb_nil_p(value))
  {
    sprite->bitmap = NULL;
  }
  else
  {
    if (!mrb_bitmap_p(value)) mrb_raise(mrb, E_ARGUMENT_ERROR, "value is not a Bitmap");
    sprite->bitmap = mrb_get_bitmap(mrb, value);
    mrb_value src_rect = mrb_funcall(mrb, value, "rect", 0);
    mrb_funcall(mrb, mrb_iv_get(mrb, self, SRC_RECT), "set", 1, src_rect);
  }
  mrb_iv_set(mrb, self, BITMAP, value);
  return value;
}

static mrb_value
mrb_sprite_get_src_rect(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, SRC_RECT);
}

static mrb_value
mrb_sprite_get_position(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, POSITION);
}

static mrb_value
mrb_sprite_get_anchor(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, ANCHOR);
}

static mrb_value
mrb_sprite_get_scale(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, SCALE);
}

static mrb_value
mrb_sprite_get_color(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, COLOR);
}

static mrb_value
mrb_sprite_get_tone(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, TONE);
}

static mrb_value
mrb_sprite_flash(mrb_state *mrb, mrb_value self)
{
  mrb_float t;
  rf_color *color;
  rf_sprite *sprite = mrb_get_sprite(mrb, self);
  mrb_get_args(mrb, "df", &color, &mrb_color_data_type, &t);
  sprite->flash_time = sprite->total_flash_time = t;
  sprite->original_flash_color = *color;
  return mrb_nil_value();
}

static mrb_value
mrb_sprite_update(mrb_state *mrb, mrb_value self)
{
  rf_sprite *sprite = mrb_get_sprite(mrb, self);
  if (sprite->flash_time > 0)
  {
    mrb_float dt = mrb_get_dt(mrb);
    sprite->flash_time -= dt;
    if (sprite->flash_time < 0) sprite->flash_time = 0;
    mrb_float left = sprite->flash_time / sprite->total_flash_time;
    sprite->flash_color.a = (unsigned char)((mrb_float)sprite->original_flash_color.a * left);
    sprite->flash_color.r = sprite->original_flash_color.r;
    sprite->flash_color.g = sprite->original_flash_color.g;
    sprite->flash_color.b = sprite->original_flash_color.b;
  }
  return mrb_nil_value();
}

static mrb_value
mrb_sprite_get_bush_depth(mrb_state *mrb, mrb_value self)
{
  rf_sprite *sprite = mrb_get_sprite(mrb, self);
  return mrb_fixnum_value((mrb_int)sprite->bush.y);
}

static mrb_value
mrb_sprite_set_bush_depth(mrb_state *mrb, mrb_value self)
{
  mrb_float depth;
  mrb_get_args(mrb, "f", &depth);
  rf_sprite *sprite = mrb_get_sprite(mrb, self);
  sprite->bush.y = depth;
  return mrb_fixnum_value((mrb_int)depth);
}

static mrb_value
mrb_sprite_get_bush_opacity(mrb_state *mrb, mrb_value self)
{
  rf_sprite *sprite = mrb_get_sprite(mrb, self);
  return mrb_fixnum_value((mrb_int)(255.0f * sprite->bush.x));
}

static mrb_value
mrb_sprite_set_bush_opacity(mrb_state *mrb, mrb_value self)
{
  mrb_float opacity;
  mrb_get_args(mrb, "f", &opacity);
  rf_sprite *sprite = mrb_get_sprite(mrb, self);
  sprite->bush.x = (float)(rf_clamp(opacity, 0, 255) / 255.0f);
  return mrb_fixnum_value((mrb_int)opacity);
}

static mrb_value
mrb_sprite_get_wave_amp(mrb_state *mrb, mrb_value self)
{
  rf_sprite *sprite = mrb_get_sprite(mrb, self);
  return mrb_float_value(mrb, sprite->wave_amp);
}

static mrb_value
mrb_sprite_get_wave_length(mrb_state *mrb, mrb_value self)
{
  rf_sprite *sprite = mrb_get_sprite(mrb, self);
  return mrb_float_value(mrb, sprite->wave_length);
}

static mrb_value
mrb_sprite_get_wave_speed(mrb_state *mrb, mrb_value self)
{
  rf_sprite *sprite = mrb_get_sprite(mrb, self);
  return mrb_float_value(mrb, sprite->wave_speed);
}

static mrb_value
mrb_sprite_get_wave_phase(mrb_state *mrb, mrb_value self)
{
  rf_sprite *sprite = mrb_get_sprite(mrb, self);
  return mrb_float_value(mrb, sprite->wave_phase);
}

static mrb_value
mrb_sprite_set_wave_amp(mrb_state *mrb, mrb_value self)
{
  mrb_float value;
  mrb_get_args(mrb, "f", &value);
  rf_sprite *sprite = mrb_get_sprite(mrb, self);
  sprite->wave_amp = value;
  return mrb_float_value(mrb, value);
}

static mrb_value
mrb_sprite_set_wave_length(mrb_state *mrb, mrb_value self)
{
  mrb_float value;
  mrb_get_args(mrb, "f", &value);
  rf_sprite *sprite = mrb_get_sprite(mrb, self);
  sprite->wave_length = value;
  return mrb_float_value(mrb, value);
}


static mrb_value
mrb_sprite_set_wave_speed(mrb_state *mrb, mrb_value self)
{
  mrb_float value;
  mrb_get_args(mrb, "f", &value);
  rf_sprite *sprite = mrb_get_sprite(mrb, self);
  sprite->wave_speed = value;
  return mrb_float_value(mrb, value);
}

static mrb_value
mrb_sprite_set_wave_phase(mrb_state *mrb, mrb_value self)
{
  mrb_float value;
  mrb_get_args(mrb, "f", &value);
  rf_sprite *sprite = mrb_get_sprite(mrb, self);
  sprite->wave_phase = value;
  return mrb_float_value(mrb, value);
}

void
mrb_init_ogss_sprite(mrb_state *mrb)
{
  struct RClass *sprite = mrb_define_class(mrb, "Sprite", mrb->object_class);
  MRB_SET_INSTANCE_TT(sprite, MRB_TT_DATA);

  mrb_define_method(mrb, sprite, "initialize", mrb_sprite_initialize, MRB_ARGS_OPT(1));

  mrb_define_method(mrb, sprite, "disposed?", mrb_sprite_disposedQ, MRB_ARGS_NONE());
  mrb_define_method(mrb, sprite, "dispose", mrb_sprite_dispose, MRB_ARGS_NONE());

  mrb_define_method(mrb, sprite, "flash", mrb_sprite_flash, MRB_ARGS_REQ(2));
  mrb_define_method(mrb, sprite, "update", mrb_sprite_update, MRB_ARGS_NONE());

  mrb_define_method(mrb, sprite, "z", mrb_sprite_get_z, MRB_ARGS_NONE());
  mrb_define_method(mrb, sprite, "z=", mrb_sprite_set_z, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, sprite, "viewport", mrb_sprite_get_viewport, MRB_ARGS_NONE());
  mrb_define_method(mrb, sprite, "viewport=", mrb_sprite_set_viewport, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, sprite, "bitmap", mrb_sprite_get_bitmap, MRB_ARGS_NONE());
  mrb_define_method(mrb, sprite, "bitmap=", mrb_sprite_set_bitmap, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, sprite, "visible", mrb_sprite_get_visible, MRB_ARGS_NONE());
  mrb_define_method(mrb, sprite, "visible=", mrb_sprite_set_visible, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, sprite, "src_rect", mrb_sprite_get_src_rect, MRB_ARGS_NONE());
  mrb_define_method(mrb, sprite, "position", mrb_sprite_get_position, MRB_ARGS_NONE());
  mrb_define_method(mrb, sprite, "anchor", mrb_sprite_get_anchor, MRB_ARGS_NONE());
  mrb_define_method(mrb, sprite, "scale", mrb_sprite_get_scale, MRB_ARGS_NONE());
  mrb_define_method(mrb, sprite, "zoom", mrb_sprite_get_scale, MRB_ARGS_NONE());
  mrb_define_method(mrb, sprite, "color", mrb_sprite_get_color, MRB_ARGS_NONE());
  mrb_define_method(mrb, sprite, "tone", mrb_sprite_get_tone, MRB_ARGS_NONE());

  mrb_define_method(mrb, sprite, "blend_mode", mrb_sprite_get_blend_mode, MRB_ARGS_NONE());
  mrb_define_method(mrb, sprite, "blend_mode=", mrb_sprite_set_blend_mode, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, sprite, "bush_depth", mrb_sprite_get_bush_depth, MRB_ARGS_NONE());
  mrb_define_method(mrb, sprite, "bush_depth=", mrb_sprite_set_bush_depth, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, sprite, "bush_opacity", mrb_sprite_get_bush_opacity, MRB_ARGS_NONE());
  mrb_define_method(mrb, sprite, "bush_opacity=", mrb_sprite_set_bush_opacity, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, sprite, "rotation", mrb_sprite_get_rotation, MRB_ARGS_NONE());
  mrb_define_method(mrb, sprite, "rotation=", mrb_sprite_set_rotation, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, sprite, "angle", mrb_sprite_get_rotation, MRB_ARGS_NONE());
  mrb_define_method(mrb, sprite, "angle=", mrb_sprite_set_rotation, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, sprite, "wave_amp", mrb_sprite_get_wave_amp, MRB_ARGS_NONE());
  mrb_define_method(mrb, sprite, "wave_amp=", mrb_sprite_set_wave_amp, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, sprite, "wave_length", mrb_sprite_get_wave_length, MRB_ARGS_NONE());
  mrb_define_method(mrb, sprite, "wave_length=", mrb_sprite_set_wave_length, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, sprite, "wave_speed", mrb_sprite_get_wave_speed, MRB_ARGS_NONE());
  mrb_define_method(mrb, sprite, "wave_speed=", mrb_sprite_set_wave_speed, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, sprite, "wave_phase", mrb_sprite_get_wave_phase, MRB_ARGS_NONE());
  mrb_define_method(mrb, sprite, "wave_phase=", mrb_sprite_set_wave_phase, MRB_ARGS_REQ(1));
}
