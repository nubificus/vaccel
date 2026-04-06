// SPDX-License-Identifier: Apache-2.0

#include "prof_backend.h"

const struct vaccel_prof_backend *vaccel_prof_backend_get(void)
{
	return vaccel_prof_base_backend_get();
}
