#ifndef __VACCEL_RESOURCES_H__
#define __VACCEL_RESOURCES_H__

#include <stdint.h>

#include "list.h"

enum vaccel_resource_type {
	VACCEL_ML_MODEL = 0,
	VACCEL_RESOURCE_TYPE_MAX,
};

struct vaccel_resource {
	/* Unique identifier of the resource */
	uint64_t id;

	/* Type of resource */
	enum vaccel_resource_type type;

	/* List entry used to link this resource in a list
	 * of resources */
	list_entry_t entry;

	/* Owner of the resource */
	void *owner;
};

int vaccel_resources_bootstrap(void);
int vaccel_resource_new(struct vaccel_resource *resource,
		enum vaccel_resource_type type, void *owner);
void vaccel_resource_destroy(struct vaccel_resource *resource);

#endif /* __VACCEL_RESOURCES_H__ */
