/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
	printf("I got nr_in: %ld, nr_out: %ld\n", nr_in, nr_out);
	printf("I got this input: %d\n", a);
	sprintf(output[0].buf, "I got this input: %d\n", a);
	output[0].len = strlen(output[0].buf);

	return 0;
}
