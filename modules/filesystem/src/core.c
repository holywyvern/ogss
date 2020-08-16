#include <ogss/file.h>

#include <mruby.h>
#include <mruby/variable.h>
#include <mruby/hash.h>
#include <mruby/string.h>
#include <mruby/compile.h>
#include <mruby/proc.h>
#include <mruby/error.h>
#include <mruby/dump.h>
#include <mruby/data.h>

#include <rayfork.h>
#include <physfs.h>

#include <setjmp.h>

#define E_FILE_ERROR mrb_exc_get(mrb, "FileError")
#define PHYSFS_ERROR_STR PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode())

#define REQUIRED_FILES mrb_intern_lit(mrb, "#required_files")
#define CURRENT_FILE mrb_intern_lit(mrb, "#current_file")

struct mrb_file
{
  mrb_state         *mrb;
  PHYSFS_File       *file;
  enum mrb_file_mode mode;
};

static struct ogss_file_alloc_ext
{
  mrb_state   *mrb;
  const char **extensions;
} file_allocator;

static mrb_file *
alloc_file(mrb_state *mrb, const char *name, PHYSFS_File *fp)
{
  if (!fp)
  {
    mrb_raisef(mrb, E_FILE_ERROR, "Unable to open file '%s' (%s)", name, PHYSFS_ERROR_STR);
  }
  mrb_file *file = (mrb_file *)mrb_malloc(mrb, sizeof *file);
  file->mrb = mrb;
  file->file = fp;
  return file;
}

static mrb_file *
alloc_file_with_extension(mrb_state *mrb, const char *name, const char *extension)
{
  PHYSFS_File *fp;
  size_t name_len = strlen(name);
  size_t ext_size = strlen(extension);
  size_t size = name_len + ext_size;
  char *name_with_ext = mrb_alloca(mrb, size);
  strncpy_s(name_with_ext, size, name_with_ext, name_len);
  strncpy_s(&(name_with_ext[name_len]), ext_size, extension, ext_size);
  fp = PHYSFS_openRead(name_with_ext);
  if (!fp)
  {
    return NULL;
  }
  mrb_file *file = (mrb_file *)mrb_malloc(mrb, sizeof *file);
  file->file = fp;
  file->mode = MRB_FILE_READ;
  file->mrb = mrb;
  return file;
}

mrb_file *
mrb_file_open_read(mrb_state *mrb, const char *name)
{
  PHYSFS_File *fp;
  fp = PHYSFS_openRead(name);
  mrb_file *file = alloc_file(mrb, name, fp);
  file->mode = MRB_FILE_READ;
  return file;
}

mrb_file *
mrb_file_open_read_with_extensions(mrb_state *mrb, const char *name, const char **extensions)
{
  mrb_file *file;
  while (*extensions)
  {
    int arena = mrb_gc_arena_save(mrb);
    file = alloc_file_with_extension(mrb, name, *extensions);
    if (file)
    {
      mrb_gc_arena_restore(mrb, arena);
      break;
    }
    ++extensions;
    mrb_gc_arena_restore(mrb, arena);
  }
  if (!file)
  {
    mrb_raisef(mrb, E_FILE_ERROR, "Unable to open file '%s' (%s)", name, PHYSFS_ERROR_STR);
  }
  file->mode = MRB_FILE_READ;
  return file;
}

mrb_file *
mrb_file_open_write(mrb_state *mrb, const char *name)
{
  PHYSFS_File *fp;
  fp = PHYSFS_openWrite(name);
  mrb_file *file = alloc_file(mrb, name, fp);
  file->mode = MRB_FILE_WRITE;
  return file;
}

mrb_file *
mrb_file_open_append(mrb_state *mrb, const char *name)
{
  PHYSFS_File *fp;
  fp = PHYSFS_openAppend(name);
  mrb_file *file = alloc_file(mrb, name, fp);
  file->mode = MRB_FILE_WRITE;
  return file;
}

void
mrb_file_close(mrb_file *file)
{
  int code = PHYSFS_close(file->file);
  if (!code)
  {
    mrb_state *mrb = file->mrb;
    mrb_raisef(mrb, E_FILE_ERROR, "Unable to close file (%s)", PHYSFS_ERROR_STR);
  }
  mrb_free(file->mrb, file);
}

size_t
mrb_file_read(mrb_file *file, size_t buffer_size, char *buffer)
{
  PHYSFS_sint64 read = PHYSFS_readBytes(file->file, buffer, buffer_size);
  if (read < 0)
  {
    mrb_state *mrb = file->mrb;
    mrb_raisef(mrb, E_FILE_ERROR, "Unable to read file (%s)", PHYSFS_ERROR_STR);
  }
  return (size_t)read;
}

size_t
mrb_file_write(mrb_file *file, size_t buffer_size, char *buffer)
{
  PHYSFS_sint64 write = PHYSFS_writeBytes(file->file, buffer, buffer_size);
  if (write < (PHYSFS_sint64)buffer_size)
  {
    mrb_state *mrb = file->mrb;
    mrb_raisef(mrb, E_FILE_ERROR, "Unable to write file (%s)", PHYSFS_ERROR_STR);
  }
  return buffer_size;
}

size_t
mrb_file_tell(mrb_file *file)
{
  PHYSFS_sint64 pos = PHYSFS_tell(file->file);
  if (pos == -1)
  {
    mrb_state *mrb = file->mrb;
    mrb_raisef(mrb, E_FILE_ERROR, "Unable to get file position (%s)", PHYSFS_ERROR_STR);
  }
  return (size_t)pos;
}

size_t
mrb_file_length(mrb_file *file)
{
  PHYSFS_sint64 size = PHYSFS_fileLength(file->file);
  if (size == -1)
  {
    mrb_state *mrb = file->mrb;
    mrb_raisef(mrb, E_FILE_ERROR, "Unable to get file size (%s)", PHYSFS_ERROR_STR);
  }
  return (size_t)size;
}

mrb_bool
mrb_file_eof(mrb_file *file)
{
  return PHYSFS_eof(file->file);
}

mrb_bool
mrb_file_exists(const char *filename)
{
  return PHYSFS_exists(filename);
}

mrb_bool
mrb_file_exists_with_extensions(mrb_state *mrb, const char *filename, const char **extensions)
{
  mrb_value name = mrb_str_new_cstr(mrb, filename);
  while (*extensions)
  {
    int arena = mrb_gc_arena_save(mrb);
    mrb_value name_with_ext = mrb_str_cat_cstr(mrb, mrb_str_dup(mrb, name), *extensions);
    if (mrb_file_exists(mrb_str_to_cstr(mrb, name_with_ext)))
    {
      mrb_gc_arena_restore(mrb, arena);
      return TRUE;
    }
    ++extensions;
    mrb_gc_arena_restore(mrb, arena);
  }
  return FALSE;
}

void
mrb_file_seek(mrb_file *file, size_t position)
{
  int seek = PHYSFS_seek(file->file, position);
  if (!seek)
  {
    mrb_state *mrb = file->mrb;
    mrb_raisef(mrb, E_FILE_ERROR, "Unable to change file position (%s)", PHYSFS_ERROR_STR);
  }
}

static int
rf_mrb_file_size(void* user_data, const char* filename)
{
  mrb_state *mrb = (mrb_state *)user_data;
  mrb_file *f = mrb_file_open_read(mrb, filename);
  int size = (int)mrb_file_length(f);
  mrb_file_close(f);
  return size;
}

static bool
rf_mrb_file_read(void* user_data, const char* filename, void* dst, int dst_size)
{
  mrb_state *mrb = (mrb_state *)user_data;
  mrb_file *f = mrb_file_open_read(mrb, filename);
  size_t read = mrb_file_read(f, dst_size, (char *)dst);
  mrb_file_close(f);
  return read > 0;
}

static int
rf_mrb_file_size_with_ext(void* user_data, const char* filename)
{
  struct ogss_file_alloc_ext *u = (struct ogss_file_alloc_ext *)user_data;
  mrb_file *f = mrb_file_open_read_with_extensions(u->mrb, filename, u->extensions);
  int size = (int)mrb_file_length(f);
  mrb_file_close(f);
  return size;
}

static bool
rf_mrb_file_read_with_ext(void* user_data, const char* filename, void* dst, int dst_size)
{
  struct ogss_file_alloc_ext *u = (struct ogss_file_alloc_ext *)user_data;
  mrb_file *f = mrb_file_open_read_with_extensions(u->mrb, filename, u->extensions);
  size_t read = mrb_file_read(f, dst_size, (char *)dst);
  mrb_file_close(f);
  return read > 0;
}

rf_io_callbacks
mrb_get_io_callbacks(mrb_state *mrb)
{
  rf_io_callbacks io;
  io.file_size_proc = rf_mrb_file_size;
  io.read_file_proc = rf_mrb_file_read;
  io.user_data = mrb;
  return io;
}

void
mrb_file_set_write(mrb_state *mrb, const char *org, const char *name)
{
  const char *prefs = PHYSFS_getPrefDir(org, name);
  if (!PHYSFS_setWriteDir(prefs))
  {
    mrb_raisef(mrb, E_FILE_ERROR, "Failed to setup write directory (%s)", PHYSFS_ERROR_STR);
  }
  PHYSFS_mount(prefs, "@saves", 0);
}

enum mrb_file_mode
mrb_file_get_mode(mrb_file *file)
{
  return file->mode;
}

mrb_bool
mrb_file_mkdir(mrb_state *mrb, const char *name)
{
  return (mrb_bool)PHYSFS_mkdir(name);
}

mrb_bool
mrb_file_remove(mrb_state *mrb, const char *name)
{
  return (mrb_bool)PHYSFS_delete(name);
}

void
mrb_file_flush(mrb_file *file)
{
  if (!PHYSFS_flush(file->file))
  {
    mrb_state *mrb = file->mrb;
    mrb_raisef(mrb, E_FILE_ERROR, "Failed to flush file (%s)", PHYSFS_ERROR_STR);
  }
}

static PHYSFS_Allocator physfs_allocator;
static mrb_state *current_physfs_mrb;

void
mrb_init_file(mrb_state *mrb);

void
mrb_init_file_utils(mrb_state *mrb);

static void *
physfs_malloc(PHYSFS_uint64 size)
{
  return mrb_malloc(current_physfs_mrb, (size_t)size);
}

static void *
physfs_realloc(void *ptr, PHYSFS_uint64 size)
{
  return mrb_realloc(current_physfs_mrb, ptr, (size_t)size);
}

static void
physfs_free(void *ptr)
{
  mrb_free(current_physfs_mrb, ptr);
}

const char *RUBY_FILE_EXTENSIONS[] = {
  "",
  ".mrb",
  ".rb",
  NULL
};

mrb_value
mrb_file_detect_extension(mrb_state *mrb, const char *file, char **extensions)
{
  mrb_value real_name = mrb_str_new_cstr(mrb, file);
  while (*extensions)
  {
    int arena = mrb_gc_arena_save(mrb);
    mrb_value name = mrb_str_cat_cstr(mrb, mrb_str_dup(mrb, real_name), *extensions);
    if (mrb_file_exists(mrb_str_to_cstr(mrb, name)))
    {
      mrb_gc_arena_restore(mrb, arena);
      return name;
    }
    mrb_gc_arena_restore(mrb, arena);
    ++extensions;
  }
  mrb_raisef(mrb, E_FILE_ERROR, "Cannot load file '%s' (file not found).", file);
  return mrb_nil_value();
}

const char *
file_extension(const char *str)
{
  return strrchr(str, '.');
}

static void
eval_load_irep(mrb_state *mrb, mrb_irep *irep, mrbc_context *ctx)
{
  int ai;
  struct RProc *proc;

#ifdef USE_MRUBY_OLD_BYTE_CODE
  replace_stop_with_return(mrb, irep);
#endif 
  proc = mrb_proc_new(mrb, irep);
  mrb_irep_decref(mrb, irep);
  MRB_PROC_SET_TARGET_CLASS(proc, mrb->object_class);

  ai = mrb_gc_arena_save(mrb);
  mrb_value k = mrb_obj_value(mrb->kernel_module);
  mrb_value dir = mrb_iv_get(mrb, k, CURRENT_FILE);
  mrb_iv_set(mrb, k, CURRENT_FILE, mrb_str_new_cstr(mrb, ctx->filename));
  mrb_yield_with_class(mrb, mrb_obj_value(proc), 0, NULL, mrb_top_self(mrb), mrb->object_class);
  mrb_iv_set(mrb, k, CURRENT_FILE, dir);
  mrb_gc_arena_restore(mrb, ai);
}

struct ctx_str_state
{
  mrbc_context *ctx;
  const char *buffer;
  size_t      size;
};

static mrb_value
load_str_ctx(mrb_state *mrb, mrb_value self)
{
  struct ctx_str_state *state;
  state = self.value.p;
  return mrb_load_nstring_cxt(mrb, state->buffer, state->size, state->ctx);
}

static mrb_value
protected_load(mrb_state *mrb, const char *buffer, size_t size, mrbc_context *ctx, mrb_bool *error)
{
  struct ctx_str_state state;
  mrb_value value;
  state.ctx = ctx;
  state.size = size;
  state.buffer = buffer;
  value.value.p = (void *)&state;
  return mrb_protect(mrb, load_str_ctx, value, error);
}

static void
load_file(mrb_state *mrb, const char *file)
{
  int arena = mrb_gc_arena_save(mrb);
  mrb_value name = mrb_file_detect_extension(mrb, file, RUBY_FILE_EXTENSIONS);
  const char *real_name = mrb_str_to_cstr(mrb, name);
  mrb_file *fp = mrb_file_open_read(mrb, real_name);
  size_t size = mrb_file_length(fp);
  char *buffer = mrb_alloca(mrb, size);
  mrb_file_read(fp, size, buffer);
  mrbc_context *ctx = mrbc_context_new(mrb);
  mrbc_filename(mrb, ctx, file);
  if (strcmp(file_extension(real_name), ".mrb") == 0)
  {
    FILE *fp = tmpfile();
    if (!fp)
    {
      mrb_gc_arena_restore(mrb, arena);
      mrbc_context_free(mrb, ctx);
      mrb_raise(mrb, E_RUNTIME_ERROR, "Failed to load temporary file.");
    }
    if (fwrite(buffer, size, 1, fp) < 1)
    {
      mrb_gc_arena_restore(mrb, arena);
      mrbc_context_free(mrb, ctx);
      fclose(fp);
      mrb_raise(mrb, E_RUNTIME_ERROR, "Failed to write to temporary file.");
    };
    if (fflush(fp))
    {
      mrb_gc_arena_restore(mrb, arena);
      mrbc_context_free(mrb, ctx);
      fclose(fp);
      mrb_raise(mrb, E_RUNTIME_ERROR, "Failed to flush temporary file.");
    }
    rewind(fp);
    mrb_irep *irep = mrb_read_irep_file(mrb, fp);
    fclose(fp);
    if (irep) {
      eval_load_irep(mrb, irep, ctx);
    } else if (mrb->exc) {
      mrb_gc_arena_restore(mrb, arena);
      mrbc_context_free(mrb, ctx);
      longjmp(*(jmp_buf*)mrb->jmp, 1);
    } else {
      mrb_gc_arena_restore(mrb, arena);
      mrbc_context_free(mrb, ctx);
      mrb_raisef(mrb, E_FILE_ERROR, "Failed to load file '%s'.", file);
    }
  }
  else
  {
    mrb_value k = mrb_obj_value(mrb->kernel_module);
    mrb_value dir = mrb_iv_get(mrb, k, CURRENT_FILE);
    mrb_iv_set(mrb, k, CURRENT_FILE, mrb_str_new_cstr(mrb, ctx->filename));    
    mrb_bool error;
    mrb_value exc = protected_load(mrb, buffer, size, ctx, &error);
    mrb_iv_set(mrb, k, CURRENT_FILE, dir);
    if (error)
    {
      mrb_exc_raise(mrb, exc);
    }
  }
  mrb_gc_arena_restore(mrb, arena);
  mrbc_context_free(mrb, ctx);
}

static mrb_value
mrb_load(mrb_state *mrb, mrb_value self)
{
  const char *file;
  mrb_get_args(mrb, "z", &file);
  load_file(mrb, file);
  return mrb_nil_value();
}

static mrb_value
mrb_require(mrb_state *mrb, mrb_value self)
{
  mrb_value file;
  mrb_get_args(mrb, "S", &file);
  mrb_value hash = mrb_iv_get(mrb, mrb_obj_value(mrb->kernel_module), REQUIRED_FILES);
  if (mrb_hash_key_p(mrb, hash, file))
  {
    return mrb_false_value();
  }
  load_file(mrb, mrb_str_to_cstr(mrb, file));
  mrb_hash_set(mrb, hash, file, mrb_true_value());
  return mrb_true_value();
}

static mrb_value
mrb_print(mrb_state *mrb, mrb_value self)
{
  mrb_int argc;
  mrb_value *argv;
  mrb_get_args(mrb, "*", &argv, &argc);
  if (argc < 1)
  {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "Print requires an object");
  }
  for (mrb_int i = 0; i < argc; ++i)
  {
    int arena = mrb_gc_arena_save(mrb);
    mrb_value obj = mrb_to_str(mrb, mrb_funcall(mrb, argv[i], "to_s", 0));
    const char *str = mrb_str_to_cstr(mrb, obj);
    fprintf(stdout, "%s", str);
    mrb_gc_arena_restore(mrb, arena);
  }
  return mrb_nil_value();
}

static mrb_value
mrb_puts(mrb_state *mrb, mrb_value self)
{
  mrb_print(mrb, self);
  fprintf(stdout, "\n");
  return mrb_nil_value();
}

rf_io_callbacks
mrb_get_io_callbacks_for_extensions(mrb_state *mrb, const char **extensions)
{
  rf_io_callbacks io;
  file_allocator.extensions = extensions;
  file_allocator.mrb = mrb;
  io.file_size_proc = rf_mrb_file_size_with_ext;
  io.read_file_proc = rf_mrb_file_read_with_ext;
  io.user_data = &file_allocator;
  return mrb_get_io_callbacks(mrb);
}

mrb_value
mrb_get_current_file(mrb_state *mrb, mrb_value self)
{
  mrb_value k = mrb_obj_value(mrb->kernel_module);
  return mrb_iv_get(mrb, k, CURRENT_FILE);
}

void
mrb_setup_filesystem(mrb_state *mrb)
{
  if (!PHYSFS_isInit())
  {
    int physfs_init = 1;
    mrb_value argv0 = mrb_gv_get(mrb, mrb_intern_lit(mrb, "$0"));
    current_physfs_mrb = mrb;
    physfs_allocator.Malloc = physfs_malloc;
    physfs_allocator.Realloc = physfs_realloc;
    physfs_allocator.Free = physfs_free;
    physfs_allocator.Init = NULL;
    physfs_allocator.Deinit = NULL;
    PHYSFS_setAllocator(&physfs_allocator);
    if (mrb_string_p(argv0))
    {
      const char *str = mrb_str_to_cstr(mrb, argv0);
      physfs_init = PHYSFS_init(str);
      PHYSFS_mount(str, NULL, 1);
    }
    else
    {
      physfs_init = PHYSFS_init(NULL);
    }
    if (!physfs_init)
    {
      mrb_raisef(mrb, E_RUNTIME_ERROR, "Failed to initialize file system (%s)", PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
    }
    PHYSFS_permitSymbolicLinks(1);
    PHYSFS_mount(PHYSFS_getBaseDir(), NULL, 1);
    mrb_file_set_write(mrb, "OGSS Games", "Default");
  }
  mrb_define_module_function(mrb, mrb->kernel_module, "load", mrb_load, MRB_ARGS_REQ(1));
  mrb_define_module_function(mrb, mrb->kernel_module, "require", mrb_require, MRB_ARGS_REQ(1));
  mrb_define_module_function(mrb, mrb->kernel_module, "__exc_file__", mrb_get_current_file, MRB_ARGS_NONE());

  mrb_value k = mrb_obj_value(mrb->kernel_module);
  mrb_iv_set(mrb, k, REQUIRED_FILES, mrb_hash_new(mrb));
  mrb_iv_set(mrb, k, CURRENT_FILE, mrb_str_new_cstr(mrb, ""));

  mrb_define_module_function(mrb, mrb->kernel_module, "print", mrb_print, MRB_ARGS_REQ(1)|MRB_ARGS_REST());
  mrb_define_module_function(mrb, mrb->kernel_module, "puts", mrb_puts, MRB_ARGS_REQ(1)|MRB_ARGS_REST());
}

void
mrb_close_filesystem(mrb_state *mrb)
{
  if (PHYSFS_isInit() && current_physfs_mrb == mrb)
  {
    if (!PHYSFS_deinit())
    {
      mrb_raise(mrb, E_RUNTIME_ERROR, "Failed to deinitialize file system");
    }
  }
}