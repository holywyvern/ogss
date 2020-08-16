#include <mruby.h>
#include <mruby/data.h>
#include <mruby/variable.h>

#include <rayfork.h>

#include <ogss/drawable.h>
#include <ogss/point.h>
#include <ogss/rect.h>
#include <ogss/color.h>
#include <ogss/bitmap.h>
#include <ogss/sprite.h>

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
draw_sprite(mrb_state *mrb, rf_sprite *sprite)
{
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
  rf_gfx_pop_matrix();
  rf_gfx_disable_texture();
}

void
mrb_init_ogss_sprite(mrb_state *mrb)
{
  struct RClass *sprite = mrb_define_class(mrb, "Sprite", mrb->object_class);
  SET_INSTANCE_TT(sprite, MRB_TT_DATA);
}