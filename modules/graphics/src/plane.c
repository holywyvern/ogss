#include <mruby.h>
#include <mruby/data.h>
#include <mruby/class.h>
#include <mruby/variable.h>

#include <rayfork.h>

#include <ogss/graphics.h>
#include <ogss/drawable.h>
#include <ogss/viewport.h>
#include <ogss/bitmap.h>
#include <ogss/plane.h>
#include <ogss/point.h>
#include <ogss/color.h>
#include <math.h>

#define BITMAP mrb_intern_cstr(mrb, "#bitmap")
#define OFFSET mrb_intern_cstr(mrb, "#offset")
#define SCALE mrb_intern_cstr(mrb, "#scale")
#define COLOR mrb_intern_cstr(mrb, "#color")
#define TONE mrb_intern_cstr(mrb, "#tone")
#define VIEWPORT mrb_intern_cstr(mrb, "#viewport")

static struct
{
  int tone;
} shader_locations;

static rf_shader plane_shader;
static mrb_bool shader_ready = FALSE;

static void
free_plane(mrb_state *mrb, void *p)
{
  if (p)
  {
    rf_plane *plane = p;
    mrb_container_remove_child(mrb, plane->base.container, p);
    mrb_free(mrb, plane);
  }
}

const struct mrb_data_type mrb_plane_data_type = { "Plane", free_plane };

static inline float
plane_mod(float a, float b)
{
  mrb_int ia = (mrb_int)roundf(a);
  mrb_int ib = (mrb_int)roundf(b);
  return (float)((ia % ib + ib) % ib);
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

static void
init_shader(mrb_state *mrb)
{
  rf_get_default_shader();
  plane_shader = rf_gfx_load_shader(NULL, frag);
  shader_locations.tone = rf_gfx_get_shader_location(plane_shader, "tone");
  shader_ready = TRUE;
}

static inline void
bind_shader(rf_plane *plane)
{
  float tone[] = {
    (float)plane->tone->r / 255.f,
    (float)plane->tone->g / 255.f,
    (float)plane->tone->b / 255.f,
    (float)plane->tone->a / 255.f
  };
  rf_gfx_set_shader_value(plane_shader, shader_locations.tone, tone, RF_UNIFORM_VEC4);
}

static void
rf_draw_plane(mrb_state *mrb, rf_plane *plane)
{
  if (!plane->bitmap) return;

  mrb_refresh_bitmap(plane->bitmap);
  rf_texture2d texture = plane->bitmap->texture;
  if (texture.id <= 0) return;
  if (!texture.valid) return;

  mrb_bool flip_x = FALSE, flip_y = FALSE;

  float sx = plane->scale->x, sy = plane->scale->y;

  if (!sx || !sy) return;

  if (sx < 0) { flip_x = true; sx *= -1; }
  if (sy < 0) { flip_y = true; sy *= -1; }

  rf_rec dst = (rf_rec){
    0, 0, texture.width * sx, texture.height * sy
  }; 

  float vx = 0, vy = 0, vw = 0, vh = 0;

  if (plane->viewport)
  {
    vx = plane->viewport->offset->x; 
    vy = plane->viewport->offset->y;
    vw = plane->viewport->rect->width;
    vh = plane->viewport->rect->height;
  }
  else
  {
    rf_sizef size = mrb_get_graphics_size(mrb);
    vw = size.width;
    vh = size.height;
  }
  if (!vw || !vh) return;

  float ox = vx + plane_mod(plane->offset->x + vx, dst.width);
  float oy = vy + plane_mod(plane->offset->y + vy, dst.height);

  float corners[4][2] = {
    // Bottom-left corner for texture and quad
    { 0, 0 },
    // Bottom-right corner for texture and quad
    { 0, 1 },
    // Top-right corner for texture and quad
    { 1, 1 },
    // Top-left corner for texture and quad
    { 1, 0 }         
  };

  if (flip_x)
  {
    swap_point(corners[0], corners[3]);
    swap_point(corners[1], corners[2]);
  }
  if (flip_y)
  {
    swap_point(corners[0], corners[1]);
    swap_point(corners[2], corners[3]);
  }
  mrb_int tx = 2 + vw / dst.width;
  mrb_int ty = 2 + vh / dst.height;

  rf_color color = *(plane->color);

  rf_gfx_enable_texture(texture.id);
  rf_begin_blend_mode(plane->blend_mode);
  rf_begin_shader(plane_shader);
  bind_shader(plane);
  for (mrb_int j = -2; j < ty; ++j)
  {
    for (mrb_int i = -2; i < tx; ++i)
    {
      float x = (float)(i * dst.width - ox);
      float y = (float)(j * dst.height - oy);
      rf_gfx_push_matrix();
      rf_gfx_translatef(x, y, 0);
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
      rf_gfx_pop_matrix();
    }
  }
  rf_end_shader();
  rf_end_blend_mode();
  rf_gfx_disable_texture();
}

static mrb_value
plane_initialize(mrb_state *mrb, mrb_value self)
{
  if (!shader_ready)
  {
    init_shader(mrb);
  }
  DATA_TYPE(self) = &mrb_plane_data_type;
  rf_plane *plane = mrb_malloc(mrb, sizeof *plane);
  DATA_PTR(self) = plane;
  rf_container *parent;
  plane->base.container = NULL;
  plane->base.z = 0;
  plane->base.draw = (rf_drawable_draw_callback)rf_draw_plane;
  plane->base.update = NULL;
  plane->base.visible = FALSE;
  plane->bitmap = NULL;
  plane->blend_mode = RF_BLEND_ALPHA;
  mrb_value offset = mrb_point_new(mrb, 0, 0);
  mrb_value scale = mrb_point_new(mrb, 1 , 1);
  mrb_value color = mrb_color_white(mrb);
  mrb_value tone  = mrb_tone_neutral(mrb);
  plane->offset = mrb_get_point(mrb, offset);
  plane->scale = mrb_get_point(mrb, scale);
  plane->color = mrb_get_color(mrb, color);
  plane->tone = mrb_get_tone(mrb, tone);
  plane->base.visible = TRUE;
  plane->viewport = NULL;
  mrb_iv_set(mrb, self, SCALE, scale);
  mrb_iv_set(mrb, self, COLOR, color);
  mrb_iv_set(mrb, self, TONE, tone);
  mrb_iv_set(mrb, self, OFFSET, offset);
  mrb_iv_set(mrb, self, BITMAP, mrb_nil_value());
  mrb_value parent_value = mrb_nil_value();
  mrb_int argc = mrb_get_args(mrb, "|o", &parent_value);
  if (argc && !mrb_nil_p(parent_value))
  {
    if (!mrb_viewport_p(parent_value)) mrb_raise(mrb, E_ARGUMENT_ERROR, "parent must be a Viewport");
    rf_viewport *viewport = mrb_get_viewport(mrb, parent_value);
    parent = &(viewport->base);
    plane->viewport = viewport;
    mrb_iv_set(mrb, self, VIEWPORT, parent_value);
  }
  else
  {
    parent = mrb_get_graphics_container(mrb);
    mrb_iv_set(mrb, self, VIEWPORT, mrb_nil_value());
  }
  mrb_container_add_child(mrb, parent, &(plane->base));
  return self;
}

static mrb_value
plane_disposedQ(mrb_state *mrb, mrb_value self)
{
  return mrb_bool_value(DATA_PTR(self) ? 0 : 1);
}

static mrb_value
plane_dispose(mrb_state *mrb, mrb_value self)
{
  rf_plane *plane = mrb_get_plane(mrb, self);
  free_plane(mrb, plane);
  DATA_PTR(self) = NULL;
  return mrb_nil_value();
}

static mrb_value
plane_get_z(mrb_state *mrb, mrb_value self)
{
  rf_plane *plane = mrb_get_plane(mrb, self);
  return mrb_fixnum_value(plane->base.z);
}

static mrb_value
plane_set_z(mrb_state *mrb, mrb_value self)
{
  mrb_int value;
  rf_plane *plane = mrb_get_plane(mrb, self);
  mrb_get_args(mrb, "i", &value);
  if (plane->base.z != value)
  {
    plane->base.z = value;
    mrb_container_invalidate(mrb, plane->base.container);
  }
  return mrb_fixnum_value(value);
}

static mrb_value
plane_get_viewport(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, VIEWPORT);
}

static mrb_value
plane_set_viewport(mrb_state *mrb, mrb_value self)
{
  mrb_value parent_value;
  mrb_get_args(mrb, "o", &parent_value);
  rf_plane *plane = mrb_get_plane(mrb, self);
  if (mrb_nil_p(parent_value))
  {
    mrb_container_add_child(mrb, mrb_get_graphics_container(mrb), &(plane->base));
    mrb_iv_set(mrb, self, VIEWPORT, parent_value);
    plane->viewport = NULL;
    return parent_value;
  }
  if (!mrb_viewport_p(parent_value))
  {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "value is not a Viewport");
  }
  rf_viewport *view = mrb_get_viewport(mrb, parent_value);
  mrb_container_add_child(mrb, &(view->base), &(plane->base));
  mrb_iv_set(mrb, self, VIEWPORT, parent_value);
  plane->viewport = view;
  return parent_value;
}

static mrb_value
plane_get_bitmap(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, BITMAP);
}

static mrb_value
plane_set_bitmap(mrb_state *mrb, mrb_value self)
{
  mrb_value value;
  rf_plane *plane = mrb_get_plane(mrb, self);
  mrb_get_args(mrb, "o", &value);
  if (mrb_nil_p(value))
  {
    plane->bitmap = NULL;
  }
  else
  {
    if (!mrb_bitmap_p(value)) mrb_raise(mrb, E_ARGUMENT_ERROR, "value is not a Bitmap");
    plane->bitmap = mrb_get_bitmap(mrb, value);
  }
  mrb_iv_set(mrb, self, BITMAP, value);
  return value;
}

static mrb_value
plane_get_visible(mrb_state *mrb, mrb_value self)
{
  rf_plane *plane = mrb_get_plane(mrb, self);
  return mrb_bool_value(plane->base.visible);
}

static mrb_value
plane_set_visible(mrb_state *mrb, mrb_value self)
{
  mrb_bool value;
  rf_plane *plane = mrb_get_plane(mrb, self);
  mrb_get_args(mrb, "b", &value); 
  plane->base.visible = value;
  return mrb_bool_value(value);
}

static mrb_value
plane_get_blend_mode(mrb_state *mrb, mrb_value self)
{
  rf_plane *plane = mrb_get_plane(mrb, self);
  return mrb_fixnum_value(plane->blend_mode);
}

static mrb_value
plane_set_blend_mode(mrb_state *mrb, mrb_value self)
{
  mrb_int value;
  rf_plane *plane = mrb_get_plane(mrb, self);
  mrb_get_args(mrb, "i", &value); 
  plane->blend_mode = (rf_blend_mode)value;
  return mrb_fixnum_value(value);
}

static mrb_value
plane_get_offset(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, OFFSET);
}

static mrb_value
plane_get_scale(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, SCALE);
}

static mrb_value
plane_get_color(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, COLOR);
}

static mrb_value
plane_get_tone(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, TONE);
}

void
mrb_init_ogss_plane(mrb_state *mrb)
{
  struct RClass *plane = mrb_define_class(mrb, "Plane", mrb->object_class);
  MRB_SET_INSTANCE_TT(plane, MRB_TT_DATA);

  mrb_define_method(mrb, plane, "initialize", plane_initialize, MRB_ARGS_OPT(1));

  mrb_define_method(mrb, plane, "disposed?", plane_disposedQ, MRB_ARGS_NONE());
  mrb_define_method(mrb, plane, "dispose", plane_dispose, MRB_ARGS_NONE());

  mrb_define_method(mrb, plane, "z", plane_get_z, MRB_ARGS_NONE());
  mrb_define_method(mrb, plane, "z=", plane_set_z, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, plane, "viewport", plane_get_viewport, MRB_ARGS_NONE());
  mrb_define_method(mrb, plane, "viewport=", plane_set_viewport, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, plane, "bitmap", plane_get_bitmap, MRB_ARGS_NONE());
  mrb_define_method(mrb, plane, "bitmap=", plane_set_bitmap, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, plane, "visible", plane_get_visible, MRB_ARGS_NONE());
  mrb_define_method(mrb, plane, "visible=", plane_set_visible, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, plane, "blend_mode", plane_get_blend_mode, MRB_ARGS_NONE());
  mrb_define_method(mrb, plane, "blend_mode=", plane_set_blend_mode, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, plane, "offset", plane_get_offset, MRB_ARGS_NONE());
  mrb_define_method(mrb, plane, "scroll", plane_get_offset, MRB_ARGS_NONE());
  mrb_define_method(mrb, plane, "scale", plane_get_scale, MRB_ARGS_NONE());
  mrb_define_method(mrb, plane, "zoom", plane_get_scale, MRB_ARGS_NONE());
  mrb_define_method(mrb, plane, "color", plane_get_color, MRB_ARGS_NONE());
  mrb_define_method(mrb, plane, "tone", plane_get_tone, MRB_ARGS_NONE());
}