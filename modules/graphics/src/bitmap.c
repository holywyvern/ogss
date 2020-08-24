#include <mruby.h>
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/variable.h>

#include <rayfork.h>

#include <orgf/alloc.h>
#include <orgf/color.h>
#include <orgf/font.h>
#include <orgf/bitmap.h>
#include <orgf/file.h>
#include <orgf/rect.h>

#define FONT mrb_intern_lit(mrb, "#font")

static const char *IMAGE_EXTENSIONS[] = {
  "",
  ".png",
  ".bmp",
  ".jpg",
  ".jpeg",
  NULL
};

static void
free_bitmap(mrb_state *mrb, void *ptr)
{
  if (ptr)
  {
    rf_bitmap *bmp = ptr;
    rf_unload_texture(bmp->texture);
    rf_unload_image(bmp->image, mrb_get_allocator(mrb));
    mrb_free(mrb, bmp);
  }
}

const struct mrb_data_type mrb_bitmap_data_type = {
  "Bitmap", free_bitmap
};

static mrb_value
mrb_bitmap_initialize(mrb_state *mrb, mrb_value self)
{
  rf_image img;
  mrb_int argc = mrb_get_argc(mrb);
  rf_allocator alloc = mrb_get_allocator(mrb);
  switch (argc)
  {
    case 0:
    {
      img = rf_gen_image_color(1, 1, (rf_color){0, 0, 0, 0}, alloc);
      break;
    }
    case 1:
    {
      int arena = mrb_gc_arena_save(mrb);
      const char *filename;
      rf_io_callbacks io = mrb_get_io_callbacks_for_extensions(mrb, IMAGE_EXTENSIONS);
      mrb_get_args(mrb, "z", &filename);
      const char *new_filename = mrb_filesystem_join(mrb, "Graphics", filename);
      img = rf_load_image_from_file(new_filename, alloc, alloc, io);
      mrb_gc_arena_restore(mrb, arena);
      break;
    }
    case 2:
    {
      mrb_int w, h;
      mrb_get_args(mrb, "ii", &w, &h);
      if (w < 1) mrb_raise(mrb, E_ARGUMENT_ERROR, "Width must be positive");
      if (h < 1) mrb_raise(mrb, E_ARGUMENT_ERROR, "Height must be positive");
      img = rf_gen_image_color((int)w, (int)h, (rf_color){0, 0, 0, 0}, alloc);
      break;
    }
    default: {
      mrb_raisef(mrb, E_ARGUMENT_ERROR, "Wrong number of arguments expected 0, 1 or 2 but %i where given", argc);
    }
  }
  rf_bitmap *bmp = mrb_malloc(mrb, sizeof *bmp);
  bmp->image = img;
  bmp->texture = rf_load_texture_from_image(img);
  bmp->dirty = FALSE;
  mrb_value font = mrb_new_default_font(mrb);
  mrb_iv_set(mrb, self, FONT, font);
  bmp->font = mrb_get_font(mrb, font);
  DATA_TYPE(self) = &mrb_bitmap_data_type;
  DATA_PTR(self) = bmp;
  return mrb_nil_value();
}

static mrb_value
mrb_bitmap_initialize_copy(mrb_state *mrb, mrb_value self)
{
  rf_bitmap *original;
  mrb_get_args(mrb, "d", &original, &mrb_bitmap_data_type);
  rf_image img = rf_image_copy(original->image, mrb_get_allocator(mrb));
  rf_bitmap *bmp = mrb_malloc(mrb, sizeof *bmp);
  bmp->image = img;
  bmp->texture = rf_load_texture_from_image(img);
  bmp->dirty = FALSE;
  return mrb_nil_value();
}

static mrb_value
mrb_bitmap_rect(mrb_state *mrb, mrb_value self)
{
  rf_bitmap *bmp = mrb_get_bitmap(mrb, self);
  return mrb_rect_new(mrb, 0, 0, bmp->image.width, bmp->image.height);
}

static mrb_value
mrb_bitmap_get_width(mrb_state *mrb, mrb_value self)
{
  rf_bitmap *bmp = mrb_get_bitmap(mrb, self);
  return mrb_fixnum_value(bmp->image.width);
}

static mrb_value
mrb_bitmap_get_height(mrb_state *mrb, mrb_value self)
{
  rf_bitmap *bmp = mrb_get_bitmap(mrb, self);
  return mrb_fixnum_value(bmp->image.height);
}

static mrb_value
new_bitmap(mrb_state *mrb)
{
  mrb_value argv[] = {
    mrb_fixnum_value(1),
    mrb_fixnum_value(1)
  };
  struct RClass *bmp = mrb_class_get(mrb, "Bitmap");
  return mrb_obj_new(mrb, bmp, 2, argv);
}


#define COLOR_PARAM(v) &v, &mrb_color_data_type

static mrb_value
mrb_bitmap_s_from_color(mrb_state *mrb, mrb_value self)
{
  mrb_int w, h;
  rf_color *color;
  rf_allocator alloc = mrb_get_allocator(mrb);
  mrb_get_args(mrb, "iid", &w, &h, COLOR_PARAM(color));
  mrb_value result = new_bitmap(mrb);
  rf_bitmap *bmp = mrb_get_bitmap(mrb, result);
  rf_unload_texture(bmp->texture);
  rf_unload_image(bmp->image, alloc);
  bmp->image = rf_gen_image_color((int)w, (int)h, *color, alloc);
  bmp->texture = rf_load_texture_from_image(bmp->image);
  return result;
}

static mrb_value
mrb_bitmap_s_cellular(mrb_state *mrb, mrb_value self)
{
  mrb_int w, h, ts;
  rf_allocator alloc = mrb_get_allocator(mrb);
  mrb_get_args(mrb, "iii", &w, &h, &ts);
  mrb_value result = new_bitmap(mrb);
  rf_bitmap *bmp = mrb_get_bitmap(mrb, result);
  rf_unload_texture(bmp->texture);
  rf_unload_image(bmp->image, alloc);
  bmp->image = rf_gen_image_cellular((int)w, (int)h, (int)ts, RF_DEFAULT_RAND_PROC, alloc);
  bmp->texture = rf_load_texture_from_image(bmp->image);
  return result;
}

static mrb_value
mrb_bitmap_s_checked(mrb_state *mrb, mrb_value self)
{
  mrb_int w, h, cx, cy;
  rf_color *c1, *c2;
  rf_allocator alloc = mrb_get_allocator(mrb);
  mrb_get_args(mrb, "iiiidd", &w, &h, &cx, &cy, COLOR_PARAM(c1), COLOR_PARAM(c2));
  mrb_value result = new_bitmap(mrb);
  rf_bitmap *bmp = mrb_get_bitmap(mrb, result);
  rf_unload_texture(bmp->texture);
  rf_unload_image(bmp->image, alloc);
  bmp->image = rf_gen_image_checked((int)w, (int)h, (int)cx, (int)cy, *c1, *c2, alloc);
  bmp->texture = rf_load_texture_from_image(bmp->image);
  return result;
}

static mrb_value
mrb_bitmap_s_gradient(mrb_state *mrb, mrb_value self)
{
  mrb_int w, h;
  rf_color *c1, *c2;
  mrb_bool v = FALSE;
  rf_allocator alloc = mrb_get_allocator(mrb);
  mrb_get_args(mrb, "iidd|b", &w, &h, COLOR_PARAM(c1), COLOR_PARAM(c2), &v);
  mrb_value result = new_bitmap(mrb);
  rf_bitmap *bmp = mrb_get_bitmap(mrb, result);
  rf_unload_texture(bmp->texture);
  rf_unload_image(bmp->image, alloc);
  if (v)
  {
    bmp->image = rf_gen_image_gradient_v((int)w, (int)h, *c1, *c2, alloc);
    
  }
  else
  {
    bmp->image = rf_gen_image_gradient_h((int)w, (int)h, *c1, *c2, alloc);
  }
  bmp->texture = rf_load_texture_from_image(bmp->image);  
  return result;
}

static mrb_value
mrb_bitmap_s_radial_gradient(mrb_state *mrb, mrb_value self)
{
  mrb_int w, h;
  mrb_float d;
  rf_color *c1, *c2;
  mrb_get_args(mrb, "iifdd", &w, &h, &d, COLOR_PARAM(c1), COLOR_PARAM(c2));
  rf_allocator alloc = mrb_get_allocator(mrb);
  mrb_value result = new_bitmap(mrb);
  rf_bitmap *bmp = mrb_get_bitmap(mrb, result);
  rf_unload_texture(bmp->texture);
  rf_unload_image(bmp->image, alloc);
  bmp->image = rf_gen_image_gradient_radial((int)w, (int)h, (float)d, *c1, *c2, alloc);
  bmp->texture = rf_load_texture_from_image(bmp->image);  
  return result;
}

static mrb_value
mrb_bitmap_s_perlin_noise(mrb_state *mrb, mrb_value self)
{
  mrb_int w, h, ox, oy;
  mrb_float s;
  rf_allocator alloc = mrb_get_allocator(mrb);
  mrb_get_args(mrb, "iiiif", &w, &h, &ox, &oy, &s);
  mrb_value result = new_bitmap(mrb);
  rf_bitmap *bmp = mrb_get_bitmap(mrb, result);
  rf_unload_texture(bmp->texture);
  rf_unload_image(bmp->image, alloc);
  bmp->image = rf_gen_image_perlin_noise((int)w, (int)h, (int)ox, (int)oy, (float)s, alloc);
  bmp->texture = rf_load_texture_from_image(bmp->image);
  return result;
}

static mrb_value
mrb_bitmap_s_white_noise(mrb_state *mrb, mrb_value self)
{
  mrb_int w, h;
  mrb_float f;
  rf_allocator alloc = mrb_get_allocator(mrb);
  mrb_get_args(mrb, "iif", &w, &h, &f);
  mrb_value result = new_bitmap(mrb);
  rf_bitmap *bmp = mrb_get_bitmap(mrb, result);
  rf_unload_texture(bmp->texture);
  rf_unload_image(bmp->image, alloc);
  bmp->image = rf_gen_image_white_noise((int)w, (int)h, (float)f, RF_DEFAULT_RAND_PROC, alloc);
  bmp->texture = rf_load_texture_from_image(bmp->image);
  return result;
}

static mrb_value
mrb_bitmap_disposedQ(mrb_state *mrb, mrb_value self)
{
  return mrb_bool_value(DATA_PTR(self) ? 0 : 1);
}

static mrb_value
mrb_bitmap_dispose(mrb_state *mrb, mrb_value self)
{
  rf_bitmap *bmp = mrb_get_bitmap(mrb, self);
  free_bitmap(mrb, bmp);
  DATA_PTR(self) = NULL;
  return mrb_nil_value();
}

static mrb_value
mrb_bitmap_blt(mrb_state *mrb, mrb_value self)
{
  rf_bitmap *bmp = mrb_get_bitmap(mrb, self);
  return mrb_nil_value();
}

static mrb_value
mrb_bitmap_stretch_blt(mrb_state *mrb, mrb_value self)
{
  rf_bitmap *bmp = mrb_get_bitmap(mrb, self);
  return mrb_nil_value();
}

static mrb_value
mrb_bitmap_fill_rect(mrb_state *mrb, mrb_value self)
{
  rf_bitmap *bmp = mrb_get_bitmap(mrb, self);
  return mrb_nil_value();
}

static mrb_value
mrb_bitmap_gradient_fill_rect(mrb_state *mrb, mrb_value self)
{
  rf_bitmap *bmp = mrb_get_bitmap(mrb, self);
  return mrb_nil_value();
}

static mrb_value
mrb_bitmap_clear(mrb_state *mrb, mrb_value self)
{
  rf_bitmap *bmp = mrb_get_bitmap(mrb, self);
  return mrb_nil_value();
}

static mrb_value
mrb_bitmap_clear_rect(mrb_state *mrb, mrb_value self)
{
  rf_bitmap *bmp = mrb_get_bitmap(mrb, self);
  return mrb_nil_value();
}

static mrb_value
mrb_bitmap_get_pixel(mrb_state *mrb, mrb_value self)
{
  rf_bitmap *bmp = mrb_get_bitmap(mrb, self);
  return mrb_nil_value();
}

static mrb_value
mrb_bitmap_set_pixel(mrb_state *mrb, mrb_value self)
{
  rf_bitmap *bmp = mrb_get_bitmap(mrb, self);
  return mrb_nil_value();
}

static mrb_value
mrb_bitmap_hue_change(mrb_state *mrb, mrb_value self)
{
  rf_bitmap *bmp = mrb_get_bitmap(mrb, self);
  return mrb_nil_value();
}

static mrb_value
mrb_bitmap_blur(mrb_state *mrb, mrb_value self)
{
  rf_bitmap *bmp = mrb_get_bitmap(mrb, self);
  return mrb_nil_value();
}

static mrb_value
mrb_bitmap_radial_blur(mrb_state *mrb, mrb_value self)
{
  rf_bitmap *bmp = mrb_get_bitmap(mrb, self);
  return mrb_nil_value();
}

static mrb_value
mrb_bitmap_draw_text(mrb_state *mrb, mrb_value self)
{
  rf_bitmap *bmp = mrb_get_bitmap(mrb, self);
  return mrb_nil_value();
}

static mrb_value
mrb_bitmap_text_size(mrb_state *mrb, mrb_value self)
{
  rf_bitmap *bmp = mrb_get_bitmap(mrb, self);
  return mrb_nil_value();
}

static mrb_value
mrb_bitmap_get_font(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, FONT);
}

static mrb_value
mrb_bitmap_set_font(mrb_state *mrb, mrb_value self)
{
  rf_bitmap *bmp = mrb_get_bitmap(mrb, self);
  mrb_value font;
  mrb_get_args(mrb, "o", &font);
  if (!mrb_font_p(font))
  {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "value is not a Font.");
  }
  mrb_iv_set(mrb, self, FONT, font);
  bmp->font = mrb_get_font(mrb, font);
  return mrb_nil_value();
}

void
mrb_init_orgf_bitmap(mrb_state *mrb)
{
  struct RClass *bitmap = mrb_define_class(mrb, "Bitmap", mrb->object_class);
  MRB_SET_INSTANCE_TT(bitmap, MRB_TT_DATA);

  mrb_define_method(mrb, bitmap, "initialize", mrb_bitmap_initialize, MRB_ARGS_OPT(2));
  mrb_define_method(mrb, bitmap, "initialize_copy", mrb_bitmap_initialize_copy, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, bitmap, "rect", mrb_bitmap_rect, MRB_ARGS_NONE());
  mrb_define_method(mrb, bitmap, "width", mrb_bitmap_get_width, MRB_ARGS_NONE());
  mrb_define_method(mrb, bitmap, "height", mrb_bitmap_get_height, MRB_ARGS_NONE());

  mrb_define_method(mrb, bitmap, "font", mrb_bitmap_get_font, MRB_ARGS_NONE());
  mrb_define_method(mrb, bitmap, "font=", mrb_bitmap_set_font, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, bitmap, "disposed?", mrb_bitmap_disposedQ, MRB_ARGS_NONE());
  mrb_define_method(mrb, bitmap, "dispose", mrb_bitmap_dispose, MRB_ARGS_NONE());

  mrb_define_method(mrb, bitmap, "block_transfer", mrb_bitmap_blt, MRB_ARGS_REQ(4)|MRB_ARGS_OPT(1));
  mrb_define_method(mrb, bitmap, "blt", mrb_bitmap_blt, MRB_ARGS_REQ(4)|MRB_ARGS_OPT(1));
  mrb_define_method(mrb, bitmap, "stretch_blt", mrb_bitmap_stretch_blt, MRB_ARGS_REQ(3)|MRB_ARGS_OPT(1));
  mrb_define_method(mrb, bitmap, "fill_rect", mrb_bitmap_fill_rect, MRB_ARGS_REQ(2)|MRB_ARGS_OPT(2));
  mrb_define_method(mrb, bitmap, "gradient_fill_rect", mrb_bitmap_gradient_fill_rect, MRB_ARGS_REQ(3)|MRB_ARGS_OPT(4));
  mrb_define_method(mrb, bitmap, "clear", mrb_bitmap_clear, MRB_ARGS_OPT(1));
  mrb_define_method(mrb, bitmap, "clear_rect", mrb_bitmap_clear_rect, MRB_ARGS_REQ(1)|MRB_ARGS_OPT(3));
  mrb_define_method(mrb, bitmap, "get_pixel", mrb_bitmap_get_pixel, MRB_ARGS_REQ(1)|MRB_ARGS_OPT(1));
  mrb_define_method(mrb, bitmap, "set_pixel", mrb_bitmap_set_pixel, MRB_ARGS_REQ(2)|MRB_ARGS_OPT(1));
  mrb_define_method(mrb, bitmap, "hue_change", mrb_bitmap_hue_change, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, bitmap, "change_hue", mrb_bitmap_hue_change, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, bitmap, "blur", mrb_bitmap_blur, MRB_ARGS_NONE());
  mrb_define_method(mrb, bitmap, "radial_blur", mrb_bitmap_radial_blur, MRB_ARGS_REQ(2));
  mrb_define_method(mrb, bitmap, "draw_text", mrb_bitmap_draw_text, MRB_ARGS_REQ(2)|MRB_ARGS_OPT(4));
  mrb_define_method(mrb, bitmap, "text_size", mrb_bitmap_text_size, MRB_ARGS_REQ(1));

  mrb_define_class_method(mrb, bitmap, "cellular", mrb_bitmap_s_cellular, MRB_ARGS_ANY());
  mrb_define_class_method(mrb, bitmap, "checked", mrb_bitmap_s_checked, MRB_ARGS_ANY());
  mrb_define_class_method(mrb, bitmap, "from_color", mrb_bitmap_s_from_color, MRB_ARGS_ANY());
  mrb_define_class_method(mrb, bitmap, "gradient", mrb_bitmap_s_gradient, MRB_ARGS_ANY());
  mrb_define_class_method(mrb, bitmap, "radial_gradient", mrb_bitmap_s_radial_gradient, MRB_ARGS_ANY());
  mrb_define_class_method(mrb, bitmap, "perlin_noise", mrb_bitmap_s_perlin_noise, MRB_ARGS_ANY());
  mrb_define_class_method(mrb, bitmap, "white_noise", mrb_bitmap_s_white_noise, MRB_ARGS_ANY());
}
