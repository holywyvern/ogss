#include <mruby.h>
#include <mruby/class.h>
#include <mruby/data.h>

#include <orgf/tone.h>

const struct mrb_data_type mrb_tone_data_type = { "Tone", mrb_free };

#define clamp(v) ((v) > 255 ? 255 : ((v) < -255 ? -255 : (v)))
#define clampa(v) ((v) > 255 ? 255 : ((v) < 0 ? 0 : (v)))

static void
set_tone(mrb_state *mrb, mrb_int argc, rf_tone *tone)
{
  switch (argc)
  {
    case 0:
    {
      tone->r = tone->g = tone->b = tone->a = 0;
      break;
    }
    case 1:
    {
      rf_tone *original;
      mrb_get_args(mrb, "d", &original, &mrb_tone_data_type);
      *tone = *original;
      break;
    }
    case 3:
    {
      mrb_int r, g, b;
      mrb_get_args(mrb, "iii", &r, &g, &b);
      tone->r = (int16_t)clamp(r);
      tone->g = (int16_t)clamp(g);
      tone->b = (int16_t)clamp(b);
      break;
    }
    case 4:
    {
      mrb_int r, g, b, a;
      mrb_get_args(mrb, "iiii", &r, &g, &b, &a);
      tone->r = (int16_t)clamp(r);
      tone->g = (int16_t)clamp(g);
      tone->b = (int16_t)clamp(b);
      tone->a = (unsigned char)clampa(a);
      break;
    }
  }
}

static mrb_value
mrb_tone_initialize(mrb_state *mrb, mrb_value self)
{
  mrb_int argc = mrb_get_argc(mrb);
  switch (argc)
  {
    case 0: case 1: case 3: case 4: {
      rf_tone *tone = mrb_malloc(mrb, sizeof *tone);
      tone->a = 0;
      DATA_TYPE(self) = &mrb_tone_data_type;
      DATA_PTR(self) = tone;
      set_tone(mrb, argc, tone);
      break;
    }
    default: {
      mrb_raisef(mrb, E_ARGUMENT_ERROR, "Wrong number of arguments, 0, 1, 3, or 4 expected but, %d given.", argc);
      break;
    }
  }
  return self;
}

static mrb_value
mrb_tone_set(mrb_state *mrb, mrb_value self)
{
  mrb_int argc = mrb_get_argc(mrb);
  switch (argc)
  {
    case 1: case 3: case 4: {
      rf_tone *tone = mrb_get_tone(mrb, self);
      set_tone(mrb, argc, tone);
      break;
    }
    default: {
      mrb_raisef(mrb, E_ARGUMENT_ERROR, "Wrong number of arguments 1, 3, or 4 expected but, %d given.", argc);
      break;
    }
  }
  return self;
}

static mrb_value
mrb_tone_get_red(mrb_state *mrb, mrb_value self)
{
  rf_tone *tone = mrb_get_tone(mrb, self);
  return mrb_fixnum_value(tone->r);
}

static mrb_value
mrb_tone_get_green(mrb_state *mrb, mrb_value self)
{
  rf_tone *tone = mrb_get_tone(mrb, self);
  return mrb_fixnum_value(tone->g);
}

static mrb_value
mrb_tone_get_blue(mrb_state *mrb, mrb_value self)
{
  rf_tone *tone = mrb_get_tone(mrb, self);
  return mrb_fixnum_value(tone->b);
}

static mrb_value
mrb_tone_get_gray(mrb_state *mrb, mrb_value self)
{
  rf_tone *tone = mrb_get_tone(mrb, self);
  return mrb_fixnum_value(tone->a);
}

static mrb_value
mrb_tone_set_red(mrb_state *mrb, mrb_value self)
{
  mrb_int value;
  rf_tone *tone = mrb_get_tone(mrb, self);
  mrb_get_args(mrb, "i", &value);
  tone->r = (int16_t)clamp(value);
  return mrb_fixnum_value(value);
}

static mrb_value
mrb_tone_set_green(mrb_state *mrb, mrb_value self)
{
  mrb_int value;
  rf_tone *tone = mrb_get_tone(mrb, self);
  mrb_get_args(mrb, "i", &value);
  tone->g = (int16_t)clamp(value);
  return mrb_fixnum_value(value);
}

static mrb_value
mrb_tone_set_blue(mrb_state *mrb, mrb_value self)
{
  mrb_int value;
  rf_tone *tone = mrb_get_tone(mrb, self);
  mrb_get_args(mrb, "i", &value);
  tone->b = (int16_t)clamp(value);
  return mrb_fixnum_value(value);
}

static mrb_value
mrb_tone_set_gray(mrb_state *mrb, mrb_value self)
{
  mrb_int value;
  rf_tone *tone = mrb_get_tone(mrb, self);
  mrb_get_args(mrb, "i", &value);
  tone->a = (unsigned char)clampa(value);
  return mrb_fixnum_value(value);
}

static mrb_value
mrb_tone_equal(mrb_state *mrb, mrb_value self)
{
  mrb_value other;
  mrb_get_args(mrb, "o", &other);
  if (mrb_tone_p(other))
  {
    rf_tone *a = mrb_get_tone(mrb, self);
    rf_tone *b = mrb_get_tone(mrb, other);
    mrb_bool cmp = (a->r == b->r) && (a->g == b->g) && (a->b == b->b) && (a->a == b->a);
    return mrb_bool_value(cmp);
  }
  return mrb_false_value();
}

void
mrb_init_orgf_tone(mrb_state *mrb)
{
  struct RClass *tone = mrb_define_class(mrb, "Tone", mrb->object_class);
  MRB_SET_INSTANCE_TT(tone, MRB_TT_DATA);

  mrb_define_method(mrb, tone, "initialize", mrb_tone_initialize, MRB_ARGS_OPT(4));
  mrb_define_method(mrb, tone, "initialize_copy", mrb_tone_initialize, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, tone, "r", mrb_tone_get_red, MRB_ARGS_NONE());
  mrb_define_method(mrb, tone, "g", mrb_tone_get_green, MRB_ARGS_NONE());
  mrb_define_method(mrb, tone, "b", mrb_tone_get_blue, MRB_ARGS_NONE());
  mrb_define_method(mrb, tone, "red", mrb_tone_get_red, MRB_ARGS_NONE());
  mrb_define_method(mrb, tone, "green", mrb_tone_get_green, MRB_ARGS_NONE());
  mrb_define_method(mrb, tone, "blue", mrb_tone_get_blue, MRB_ARGS_NONE());
  mrb_define_method(mrb, tone, "gray", mrb_tone_get_gray, MRB_ARGS_NONE());
  mrb_define_method(mrb, tone, "grey", mrb_tone_get_gray, MRB_ARGS_NONE());

  mrb_define_method(mrb, tone, "r=", mrb_tone_set_red, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, tone, "g=", mrb_tone_set_green, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, tone, "b=", mrb_tone_set_blue, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, tone, "red=", mrb_tone_set_red, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, tone, "green=", mrb_tone_set_green, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, tone, "blue=", mrb_tone_set_blue, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, tone, "gray=", mrb_tone_set_gray, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, tone, "grey=", mrb_tone_set_gray, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, tone, "set", mrb_tone_set, MRB_ARGS_OPT(4));

  mrb_define_method(mrb, tone, "==", mrb_tone_equal, MRB_ARGS_REQ(1));
}
