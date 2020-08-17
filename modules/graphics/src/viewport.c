#include <mruby.h>
#include <mruby/data.h>
#include <mruby/class.h>
#include <mruby/variable.h>

#include <rayfork.h>

#include <ogss/point.h>
#include <ogss/color.h>
#include <ogss/drawable.h>
#include <ogss/rect.h>
#include <ogss/viewport.h>
#include <ogss/graphics.h>

#define RECT mrb_intern_lit(mrb, "#rect")
#define COLOR mrb_intern_lit(mrb, "#color")
#define OFFSET mrb_intern_lit(mrb, "#offset")

static void
free_viewport(mrb_state *mrb, void *p)
{
  if (p)
  {
    rf_viewport *vp = p;
    rf_unload_render_texture(vp->render);
    mrb_container_free(mrb, p);
    mrb_free(mrb, p);
  }
}

const struct mrb_data_type mrb_viewport_data_type = { "Viewport", free_viewport };

static void
rf_viewport_update(mrb_state *mrb, rf_viewport *viewport)
{
  int w = (int)viewport->rect->width;
  int h = (int)viewport->rect->height;
  if (!w || !h || !viewport->visible) return;
  if (viewport->render.texture.width != w || viewport->render.texture.height != h)
  {
    rf_unload_render_texture(viewport->render);
    viewport->render = rf_load_render_texture(w, h);
  }
  mrb_container_update(mrb, &(viewport->base));
  rf_begin_render_to_texture(viewport->render);
    rf_clear(RF_BLANK);
    rf_gfx_push_matrix();
      rf_gfx_translatef(-viewport->offset->x, -viewport->offset->y, 0);
      mrb_container_draw_children(&(viewport->base));
    rf_gfx_pop_matrix();
  rf_end_render_to_texture();
}

static void
rf_viewport_draw(rf_viewport *viewport)
{
  int x = (int)viewport->rect->x;
  int y = (int)viewport->rect->y;
  int w = (int)viewport->rect->width;
  int h = (int)viewport->rect->height;
  if (!w || !h || !viewport->visible) return;
  rf_draw_texture(viewport->render.texture, x, y, *(viewport->color));
}

static mrb_value
viewport_initialize(mrb_state *mrb, mrb_value self)
{
  mrb_int argc = mrb_get_argc(mrb);
  rf_viewport *data = mrb_malloc(mrb, sizeof *data);
  DATA_TYPE(self) = &mrb_viewport_data_type;
  DATA_PTR(self) = data;
  mrb_container_init(mrb, &(data->base));
  data->visible = TRUE;
  data->base.base.update = (rf_drawable_update_callback)rf_viewport_update;
  data->base.base.draw   = (rf_drawable_draw_callback)rf_viewport_draw;
  rf_container *c = mrb_get_graphics_container(mrb);
  mrb_value color = mrb_color_white(mrb);
  mrb_value offset = mrb_point_new(mrb, 0, 0);
  data->color = mrb_get_color(mrb, color);
  data->offset = mrb_get_point(mrb, offset);
  mrb_iv_set(mrb, self, COLOR, color);
  mrb_iv_set(mrb, self, OFFSET, offset);
  mrb_container_add_child(mrb, c, &(data->base.base));
  switch (argc)
  {
    case 0:
    { 
      rf_sizef size = mrb_get_graphics_size(mrb);
      mrb_value rect = mrb_rect_new(mrb, 0, 0, size.width, size.height);
      mrb_iv_set(mrb, self, RECT, rect);
      data->render = rf_load_render_texture((int)size.width, (int)size.height);
      data->rect = mrb_get_rect(mrb, rect);
      break;
    }
    case 1:
    {
      mrb_value value;
      mrb_get_args(mrb, "o", &value);
      if (!mrb_rect_p(value)) mrb_raise(mrb, E_ARGUMENT_ERROR, "Argument is not a Rect");
      mrb_iv_set(mrb, self, RECT, value);
      data->rect = mrb_get_rect(mrb, value);
      break;
    }
    case 4:
    {
      mrb_float x, y, w, h;
      mrb_get_args(mrb, "ffff", &x, &y, &w, &h);
      mrb_value rect = mrb_rect_new(mrb, x, y, w, h);
      data->render = rf_load_render_texture((int)w, (int)h);
      mrb_iv_set(mrb, self, RECT, rect);
      data->rect = mrb_get_rect(mrb, rect);
      break;
    }
    default:
    {
      mrb_raisef(mrb, E_ARGUMENT_ERROR, "Wrong number of arguments, expected 0, 1 or 4 but %d where given", argc);
      break;
    }
  }
  return mrb_nil_value();
}

static mrb_value
viewport_disposedQ(mrb_state *mrb, mrb_value self)
{
  return mrb_bool_value(DATA_PTR(self) ? 0 : 1);
}

static mrb_value
viewport_dispose(mrb_state *mrb, mrb_value self)
{
  rf_viewport *view = mrb_get_viewport(mrb, self);
  free_viewport(mrb, view);
  DATA_PTR(self) = NULL;
  return mrb_nil_value();
}

static mrb_value
viewport_color(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, COLOR);
}

static mrb_value
viewport_rect(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, RECT);
}

static mrb_value
viewport_offset(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, OFFSET);
}

static mrb_value
viewport_get_z(mrb_state *mrb, mrb_value self)
{
  rf_viewport *view = mrb_get_viewport(mrb, self);
  return mrb_fixnum_value(view->base.base.z);
}

static mrb_value
viewport_set_z(mrb_state *mrb, mrb_value self)
{
  mrb_int value;
  rf_viewport *view = mrb_get_viewport(mrb, self);
  mrb_get_args(mrb, "i", &value); 
  view->base.base.z = value;
  mrb_container_invalidate(mrb, view->base.base.container);
  return mrb_bool_value(value ? TRUE : FALSE);
}

static mrb_value
viewport_get_visible(mrb_state *mrb, mrb_value self)
{
  rf_viewport *view = mrb_get_viewport(mrb, self);
  return mrb_bool_value(view->visible);
}

static mrb_value
viewport_set_visible(mrb_state *mrb, mrb_value self)
{
  mrb_bool value;
  rf_viewport *view = mrb_get_viewport(mrb, self);
  mrb_get_args(mrb, "b", &value); 
  view->visible = value;
  return mrb_bool_value(value);
}


void
mrb_init_ogss_viewport(mrb_state *mrb)
{
  struct RClass *viewport = mrb_define_class(mrb, "Viewport", mrb->object_class);
  MRB_SET_INSTANCE_TT(viewport, MRB_TT_DATA);

  mrb_define_method(mrb, viewport, "initialize", viewport_initialize, MRB_ARGS_OPT(4));

  mrb_define_method(mrb, viewport, "rect", viewport_rect, MRB_ARGS_NONE());
  mrb_define_method(mrb, viewport, "color", viewport_color, MRB_ARGS_NONE());
  mrb_define_method(mrb, viewport, "offset", viewport_offset, MRB_ARGS_NONE());

  mrb_define_method(mrb, viewport, "disposed?", viewport_disposedQ, MRB_ARGS_NONE());
  mrb_define_method(mrb, viewport, "dispose", viewport_dispose, MRB_ARGS_NONE());

  mrb_define_method(mrb, viewport, "z", viewport_get_z, MRB_ARGS_NONE());
  mrb_define_method(mrb, viewport, "z=", viewport_set_z, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, viewport, "visible", viewport_get_visible, MRB_ARGS_NONE());
  mrb_define_method(mrb, viewport, "visible=", viewport_set_visible, MRB_ARGS_REQ(1));
}