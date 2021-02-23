#include "model.h"
#include "session.h"
#include "vaccel.h"
#include "resources.h"
#include "common.h"
#include "log.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int vaccel_ml_model_init(struct vaccel_ml_model *model, uint32_t flags)
{
	if (!model)
		return VACCEL_EINVAL;

	model->flags = flags;

	return VACCEL_OK;
}
