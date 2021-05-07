#ifndef __VACCEL_H__
#define __VACCEL_H__

#include <stdint.h>
#include <stddef.h>

#include "error.h"

#include "ops/blas.h"
#include "ops/exec.h"
#include "ops/genop.h"
#include "ops/image.h"
#include "ops/noop.h"
#include "ops/vaccel_ops.h"

const char *vaccel_rundir(void);

#endif /* __VACCEL_H__ */
