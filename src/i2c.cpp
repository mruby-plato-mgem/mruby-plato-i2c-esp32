#include "mruby.h"
#include "mruby/variable.h"
#include "mruby/class.h"
#include "mruby/string.h"
#include "mruby/array.h"
#include "mruby/data.h"
#include <string.h>

#ifndef NO_DEVICE
  #include "Wire.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mrb_esp32_i2c {
  void *i2c;    /* I2C object */
  mrb_int addr; /* I2C slave address */
} mrb_esp32_i2c;

#ifndef NO_DEVICE
#define WIRE(ptr) ((TwoWire*)(ptr->i2c))
#endif

static void
mrb_esp32_i2c_free(mrb_state *mrb, void *ptr)
{
  mrb_esp32_i2c *i2c = (mrb_esp32_i2c*)ptr;
#ifndef NO_DEVICE
  if (i2c->i2c) {
    i2c->i2c = NULL;
  }
#endif
  mrb_free(mrb, ptr);
}

static const struct mrb_data_type mrb_i2c_type = { "mbedI2C", mrb_esp32_i2c_free };

static mrb_value
mrb_i2c_init(mrb_state *mrb, mrb_value self)
{
  mrb_int addr;
  mrb_esp32_i2c *i2c;

  mrb_get_args(mrb, "i", &addr);

  i2c = (mrb_esp32_i2c*)mrb_malloc(mrb, sizeof(mrb_esp32_i2c));
  /* initialize I2C object */
#ifdef NO_DEVICE
  i2c->i2c = NULL;
#else /* ESP32 */
  i2c->i2c = &Wire;
#endif
  i2c->addr = addr;
  mrb_data_init(self, i2c, &mrb_i2c_type);
  return self;
}

// I2C#read(len) #=> String
static mrb_value
mrb_i2c_read(mrb_state *mrb, mrb_value self)
{
  mrb_int len, i;
  mrb_value v;
  mrb_esp32_i2c *i2c = (mrb_esp32_i2c*)DATA_PTR(self);

#ifndef NO_DEVICE
  if (!i2c->i2c) mrb_raise(mrb, E_RUNTIME_ERROR, "I2C device is already closed.");
#endif

  mrb_get_args(mrb, "i", &len);

  v = mrb_str_buf_new(mrb, len);
  memset(RSTRING_PTR(v), 0, len);

  WIRE(i2c)->requestFrom(i2c->addr, len);
  for (i=0; i<len; i++) {
    RSTRING_PTR(v)[i] = (char)WIRE(i2c)->read();
  }

  return v;
}

// I2C#write(data) #=> self
static mrb_value
mrb_i2c_write(mrb_state *mrb, mrb_value self)
{
  mrb_value data, v;
  uint8_t *buf;
  size_t len, i;
  mrb_esp32_i2c *i2c = (mrb_esp32_i2c*)DATA_PTR(self);

#ifndef NO_DEVICE
  if (!i2c->i2c) mrb_raise(mrb, E_RUNTIME_ERROR, "I2C device is already closed.");
#endif

  mrb_get_args(mrb, "o", &data);

  if (mrb_string_p(data)) {
    len = RSTRING_LEN(data);
    buf = (uint8_t*)mrb_malloc(mrb, len);
    memcpy(buf, RSTRING_PTR(data), len);
  }
  else if (mrb_array_p(data)) {
    len = RARRAY_LEN(data);
    buf = (uint8_t*)mrb_malloc(mrb, len);
    for (i=0; i<len; i++) {
      buf[i] = (uint8_t)mrb_fixnum(mrb_ary_ref(mrb, data, i));
    }
  }
  else if (mrb_fixnum_p(data)) {
    len = 1;
    buf = (uint8_t*)mrb_malloc(mrb, len);
    buf[0] = (uint8_t)mrb_fixnum(data);
  }
  else {
    v = mrb_obj_as_string(mrb, data);
    len = RSTRING_LEN(v);
    buf = (uint8_t*)mrb_malloc(mrb, len);
    memcpy(buf, RSTRING_PTR(v), len);
  }

#ifndef NO_DEVICE
  WIRE(i2c)->beginTransmission(i2c->addr);
  /* write data to I2C */
  for (i=0; i<len; i++) {
    WIRE(i2c)->write(buf[i]);
  }
  WIRE(i2c)->endTransmission();
#endif

  mrb_free(mrb, buf);

  return self;
}

void
mrb_mruby_plato_i2c_esp32_gem_init(mrb_state *mrb)
{
  struct RClass *mod  = mrb_define_module(mrb, "PlatoESP32");
  struct RClass *i2c  = mrb_define_class_under(mrb, mod, "I2C", mrb->object_class);
  MRB_SET_INSTANCE_TT(i2c, MRB_TT_DATA);

  Wire.begin();

  mrb_define_method(mrb, i2c, "initialize", mrb_i2c_init,     MRB_ARGS_REQ(1));
  mrb_define_method(mrb, i2c, "_read",      mrb_i2c_read,     MRB_ARGS_ARG(1, 1));
  mrb_define_method(mrb, i2c, "write",      mrb_i2c_write,    MRB_ARGS_REQ(1));
}

void
mrb_mruby_plato_i2c_esp32_gem_final(mrb_state *mrb)
{
}

#ifdef __cplusplus
}
#endif
