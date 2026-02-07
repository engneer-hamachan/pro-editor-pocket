#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#if defined(PICORB_VM_MRUBY)
#include <mruby.h>
void mrb_picoruby_sdcard_gem_init(mrb_state *mrb);
#elif defined(PICORB_VM_MRUBYC)
#include <mrubyc.h>
void mrbc_sdcard_init(mrbc_vm *vm);
#endif

#ifdef __cplusplus
}
#endif
