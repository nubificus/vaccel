#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

struct vaccel_arg {
	uint32_t len;
	void *buf;
};

/* We know we're getting only one read and only one write argument */

int mytestfunc(struct vaccel_arg *input, size_t nr_in,
	       struct vaccel_arg *output, size_t nr_out)
{
	int a = *(int *)input[0].buf;
	assert(nr_in >= 1);
	assert(nr_out >= 1);
	printf("I got this input: %d\n", a);
	sprintf(output[0].buf, "I got this input: %d\n", a);
	output[0].len = strlen(output[0].buf);

	return 0;
}
