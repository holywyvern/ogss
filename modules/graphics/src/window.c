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

void
mrb_init_ogss_window(mrb_state *mrb)
{
  struct RClass *window = mrb_define_class(mrb, "Window", mrb->object_class);
  MRB_SET_INSTANCE_TT(window, MRB_TT_DATA);
}
