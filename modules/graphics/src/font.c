#include <mruby.h>
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/string.h>
#include <mruby/variable.h>

#include <ogss/alloc.h>
#include <ogss/file.h>
#include <ogss/font.h>

#include <rayfork.h>

#define NAME mrb_intern_lit(mrb, "#name")
#define SIZE mrb_intern_lit(mrb, "#size")

static void
font_free(mrb_state *mrb, void *ptr)
{
  if (ptr)
  {
    rf_font *font = (rf_font *)ptr;
    rf_unload_font(*font, mrb_get_allocator(mrb));
    mrb_free(mrb, font);
  }
}

const struct mrb_data_type mrb_font_data_type = { "Font", font_free };

static const char *FONT_EXTENSIONS[] = {
  "",
  ".ttf",
  NULL,
};

static mrb_value
font_initialize(mrb_state *mrb, mrb_value self)
{
  mrb_bool antialias;
  mrb_int size;
  const char *filename;
  mrb_int argc = mrb_get_args(mrb, "|zib", &filename, &size, &antialias);
  DATA_TYPE(self) = &mrb_font_data_type;
  if (argc < 3) antialias = mrb_get_default_font_antialias(mrb);
  if (argc < 2) size = mrb_get_default_font_size(mrb);
  if (argc < 1) filename = mrb_get_default_font_name(mrb);
  if (!mrb_file_exists(filename))
  {
    mrb_raisef(mrb, E_LOAD_ERROR, "Cannot load font '%s'", filename);
  }
  if (size < 1)
  {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "Font size must be positive");
  }
  rf_allocator alloc = mrb_get_allocator(mrb);
  rf_io_callbacks io = mrb_get_io_callbacks_for_extensions(mrb, FONT_EXTENSIONS);
  rf_font_antialias aa = antialias ? RF_FONT_ANTIALIAS : RF_FONT_NO_ANTIALIAS;
  rf_font font = rf_load_ttf_font_from_file(filename, size, aa, alloc, alloc, io);
  rf_font *data = mrb_malloc(mrb, sizeof *data);
  *data = font;
  DATA_PTR(self) = data;
  mrb_iv_set(mrb, self, NAME, mrb_str_new_cstr(mrb, filename));
  mrb_iv_set(mrb, self, SIZE, mrb_fixnum_value(size));
  return self;
}

static mrb_value
font_get_name(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, NAME);
}

static mrb_value
font_get_size(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, SIZE);
}

static mrb_value
font_measure_text(mrb_state *mrb, mrb_value self)
{
  const char *text;
  mrb_int height;
  rf_font *font = mrb_get_font(mrb, self);
  if (mrb_get_args(mrb, "z|i", &text, &height) < 2)
  {
    height = font->base_size;
  }
  rf_sizef size = rf_measure_text(*font, text, rf_font_height(*font, (float)height), 0);
  return mrb_float_value(mrb, size.width);
}

void
mrb_init_ogss_font(mrb_state *mrb)
{
  struct RClass *font = mrb_define_class(mrb, "Font", mrb->object_class);
  MRB_SET_INSTANCE_TT(font, MRB_TT_DATA);

  mrb_define_method(mrb, font, "initialize", font_initialize, MRB_ARGS_OPT(3));

  mrb_define_method(mrb, font, "name", font_get_name, MRB_ARGS_NONE());
  mrb_define_method(mrb, font, "size", font_get_size, MRB_ARGS_NONE());

  mrb_define_method(mrb, font, "measure_text", font_measure_text, MRB_ARGS_REQ(1));
}

const char *
mrb_get_default_font_name(mrb_state *mrb)
{
  struct RClass *font = mrb_class_get(mrb, "Font");
  mrb_value name = mrb_to_str(mrb, mrb_funcall(mrb, mrb_obj_value(font), "default_name", 0));
  return mrb_str_to_cstr(mrb, name);
}

mrb_int
mrb_get_default_font_size(mrb_state *mrb)
{
  struct RClass *font = mrb_class_get(mrb, "Font");
  return mrb_int(mrb, mrb_funcall(mrb, mrb_obj_value(font), "default_size", 0));
}

mrb_bool
mrb_get_default_font_antialias(mrb_state *mrb)
{
  struct RClass *font = mrb_class_get(mrb, "Font");
  return mrb_bool(mrb_funcall(mrb, mrb_obj_value(font), "default_antialias", 0));
}
