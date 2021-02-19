#include "resources.h"
#include "list.h"
#include "id_pool.h"
#include "log.h"
#include "common.h"

#include <stdlib.h>

#define MAX_VACCEL_RESOURCES 1024*1024UL

static struct {
	id_pool_t *ids;
} state;

int vaccel_resources_bootstrap(void)
{
	state.ids = id_pool_new(MAX_VACCEL_RESOURCES);
	if (!state.ids)
		return VACCEL_ENOMEM;

	return VACCEL_OK;
}

int vaccel_resource_new(struct vaccel_resource *resource,
		enum vaccel_resource_type type, void *owner)
{
	if (type >= VACCEL_RESOURCE_TYPE_MAX)
		return VACCEL_EINVAL;

	if (!owner)
		return VACCEL_EINVAL;

	resource->id = get_new_id(state.ids);
	resource->type = type;
	list_init(&resource->entry);
	resource->owner = owner;

	return VACCEL_OK;
}

void vaccel_resource_destroy(struct vaccel_resource *resource)
{
	if (entry_linked(&resource->entry))
		vaccel_warn("Destroying resource %u which is still linked",
				resource->id);

	release_id(state.ids, resource->id);
}
