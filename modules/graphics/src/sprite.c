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
  if (!sprite->visible) return;
  if (!sprite->bitmap) return;

  mrb_refresh_bitmap(sprite->bitmap);
  rf_texture2d texture = sprite->bitmap->texture;
  if (texture.id <= 0) return;
  mrb_bool flip_x = FALSE, flip_y = FALSE;

  float sx = sprite->scale->x, sy = sprite->scale->y;

  if (!sx || !sy) return;

  if (sx < 0) { flip_x = true; sx *= -1; }
  if (sy < 0) { flip_y = true; sy *= -1; }

  rf_rec src = *(sprite->src_rect);
  float width  = (float)texture.width;
  float height = (float)texture.height;
  rf_rec dst = (rf_rec){
    sprite->position->x, sprite->position->y, width * sx, height * sy
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
    rf_gfx_begin(RF_QUADS);
      rf_gfx_color4f(color.r, color.g, color.b, color.a);
      rf_gfx_normal3f(0.0f, 0.0f, 1.0f);
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
    rf_end_blend_mode();
  rf_gfx_pop_matrix();
  rf_gfx_disable_texture();
}

static mrb_value
sprite_initialize(mrb_state *mrb, mrb_value self)
{
  rf_viewport *viewport;
  DATA_TYPE(self) = &mrb_sprite_data_type;
  rf_sprite *sprite = mrb_malloc(mrb, sizeof *sprite);
  DATA_PTR(self) = sprite;
  mrb_int argc = mrb_get_args(mrb, "|d", &viewport, &mrb_viewport_data_type);
  rf_container *parent;
  sprite->base.container = NULL;
  sprite->base.z = 0;
  sprite->base.draw = (rf_drawable_draw_callback)rf_draw_sprite;
  sprite->base.update = NULL;
  sprite->bitmap = NULL;
  sprite->visible = FALSE;
  sprite->rotation = 0;
  sprite->blend_mode = RF_BLEND_ALPHA;
  mrb_value position = mrb_point_new(mrb, 0, 0);
  mrb_value anchor = mrb_point_new(mrb, 0, 0);
  mrb_value scale = mrb_point_new(mrb, 1 , 1);
  mrb_value color = mrb_color_white(mrb);
  sprite->position = mrb_get_point(mrb, position);
  sprite->anchor = mrb_get_point(mrb, anchor);
  sprite->scale = mrb_get_point(mrb, scale);
  sprite->color = mrb_get_color(mrb, color);
  sprite->visible = TRUE;
  mrb_iv_set(mrb, self, POSITION, position);
  mrb_iv_set(mrb, self, ANCHOR, anchor);
  mrb_iv_set(mrb, self, SCALE, scale);
  mrb_iv_set(mrb, self, COLOR, color);
  mrb_iv_set(mrb, self, BITMAP, mrb_nil_value());
  if (argc)
  {
    parent = &(viewport->base);
  }
  else
  {
    parent = mrb_get_graphics_container(mrb);
  }
  mrb_container_add_child(mrb, parent, &(sprite->base));
  return self;
}

static mrb_value
sprite_disposedQ(mrb_state *mrb, mrb_value self)
{
  return mrb_bool_value(DATA_PTR(self) ? 0 : 1);
}

static mrb_value
sprite_dispose(mrb_state *mrb, mrb_value self)
{
  rf_sprite *sprite = mrb_get_sprite(mrb, self);
  free_sprite(mrb, sprite);
  DATA_PTR(self) = NULL;
  return mrb_nil_value();
}

static mrb_value
sprite_get_z(mrb_state *mrb, mrb_value self)
{
  rf_sprite *sprite = mrb_get_sprite(mrb, self);
  return mrb_fixnum_value(sprite->base.z);
}

static mrb_value
sprite_set_z(mrb_state *mrb, mrb_value self)
{
  mrb_int value;
  rf_sprite *sprite = mrb_get_sprite(mrb, self);
  mrb_get_args(mrb, "i", &value); 
  sprite->base.z = value;
  mrb_container_invalidate(mrb, sprite->base.container);
  return mrb_fixnum_value(value);
}

static mrb_value
sprite_get_visible(mrb_state *mrb, mrb_value self)
{
  rf_sprite *sprite = mrb_get_sprite(mrb, self);
  return mrb_bool_value(sprite->visible);
}

static mrb_value
sprite_set_visible(mrb_state *mrb, mrb_value self)
{
  mrb_bool value;
  rf_sprite *sprite = mrb_get_sprite(mrb, self);
  mrb_get_args(mrb, "b", &value); 
  sprite->visible = value;
  return mrb_bool_value(value);
}

static mrb_value
sprite_get_rotation(mrb_state *mrb, mrb_value self)
{
  rf_sprite *sprite = mrb_get_sprite(mrb, self);
  return mrb_float_value(mrb, sprite->rotation);
}

static mrb_value
sprite_set_rotation(mrb_state *mrb, mrb_value self)
{
  mrb_float value;
  rf_sprite *sprite = mrb_get_sprite(mrb, self);
  mrb_get_args(mrb, "f", &value); 
  sprite->rotation = (float)value;
  return mrb_float_value(mrb, value);
}

static mrb_value
sprite_get_blend_mode(mrb_state *mrb, mrb_value self)
{
  rf_sprite *sprite = mrb_get_sprite(mrb, self);
  return mrb_fixnum_value(sprite->blend_mode);
}

static mrb_value
sprite_set_blend_mode(mrb_state *mrb, mrb_value self)
{
  mrb_int value;
  rf_sprite *sprite = mrb_get_sprite(mrb, self);
  mrb_get_args(mrb, "i", &value); 
  sprite->blend_mode = (rf_blend_mode)value;
  return mrb_fixnum_value(value);
}

static mrb_value
sprite_get_bitmap(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, BITMAP);
}

static mrb_value
sprite_set_bitmap(mrb_state *mrb, mrb_value self)
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
    if (!mrb_bitmap_p(value)) mrb_raise(mrb, E_ARGUMENT_ERROR, "Value is not a Bitmap");
    sprite->bitmap = mrb_get_bitmap(mrb, value);
    mrb_value src_rect = mrb_funcall(mrb, value, "rect", 0);
    mrb_funcall(mrb, mrb_iv_get(mrb, self, SRC_RECT), "set", 1, src_rect);
  }
  mrb_iv_set(mrb, self, BITMAP, value);
  return value;
}

static mrb_value
sprite_get_src_rect(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, SRC_RECT);
}

static mrb_value
sprite_get_position(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, POSITION);
}

static mrb_value
sprite_get_anchor(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, ANCHOR);
}

static mrb_value
sprite_get_scale(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, SCALE);
}

void
mrb_init_ogss_sprite(mrb_state *mrb)
{
  struct RClass *sprite = mrb_define_class(mrb, "Sprite", mrb->object_class);
  MRB_SET_INSTANCE_TT(sprite, MRB_TT_DATA);

  mrb_define_method(mrb, sprite, "initialize", sprite_initialize, MRB_ARGS_OPT(1));

  mrb_define_method(mrb, sprite, "disposed?", sprite_disposedQ, MRB_ARGS_NONE());
  mrb_define_method(mrb, sprite, "dispose", sprite_dispose, MRB_ARGS_NONE());

  mrb_define_method(mrb, sprite, "z", sprite_get_z, MRB_ARGS_NONE());
  mrb_define_method(mrb, sprite, "z=", sprite_set_z, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, sprite, "bitmap", sprite_get_bitmap, MRB_ARGS_NONE());
  mrb_define_method(mrb, sprite, "bitmap=", sprite_set_bitmap, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, sprite, "visible", sprite_get_visible, MRB_ARGS_NONE());
  mrb_define_method(mrb, sprite, "visible=", sprite_set_visible, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, sprite, "src_rect", sprite_get_src_rect, MRB_ARGS_NONE());
  mrb_define_method(mrb, sprite, "position", sprite_get_position, MRB_ARGS_NONE());
  mrb_define_method(mrb, sprite, "anchor", sprite_get_anchor, MRB_ARGS_NONE());
  mrb_define_method(mrb, sprite, "scale", sprite_get_scale, MRB_ARGS_NONE());
  mrb_define_method(mrb, sprite, "zoom", sprite_get_scale, MRB_ARGS_NONE());

  mrb_define_method(mrb, sprite, "blend_mode", sprite_get_blend_mode, MRB_ARGS_NONE());
  mrb_define_method(mrb, sprite, "blend_mode=", sprite_set_blend_mode, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, sprite, "rotation", sprite_get_rotation, MRB_ARGS_NONE());
  mrb_define_method(mrb, sprite, "rotation=", sprite_set_rotation, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, sprite, "angle", sprite_get_rotation, MRB_ARGS_NONE());
  mrb_define_method(mrb, sprite, "angle=", sprite_set_rotation, MRB_ARGS_REQ(1));
}
