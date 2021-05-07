#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#include "include/plugin.h"
#include "ops/vaccel_ops.h"

#include <stdint.h>
#include "vaccel.h"

void *get_plugin_op(enum vaccel_op_type op_type);
struct vaccel_plugin *get_virtio_plugin(void);
int plugins_bootstrap(void);
int plugins_shutdown(void);

#endif /* __PLUGIN_H__ */
