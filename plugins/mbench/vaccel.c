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

#include <stdio.h>
#include <vaccel.h>
#include <signal.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TIME 300000
int the_end;

static void alarm_handler(int signum)
{
	the_end = 1;
}

static int mbench(int time)
{
	int ret, sec = 0, usec = 1;
	struct itimerval timer;
	struct vaccel_arg *args;

	if (time > (int) MAX_TIME || time < 1)
		return VACCEL_EINVAL;

	signal(SIGALRM, alarm_handler);
	the_end = 0;

	if (time > 999) {
		sec = time / 1000;
		usec = time % 1000;
	} else {
		usec = time;
	}

	timer.it_value.tv_sec = sec;
	timer.it_value.tv_usec = usec;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 0;
	if (setitimer(ITIMER_REAL, &timer, NULL) < 0) {
		perror("[mbench] setittimer");
		return VACCEL_EINVAL;
	}

	while (1) {
		if (the_end)
			break;
	}

	vaccel_debug("[mbench] %d ms elapsed", time);

	return VACCEL_OK;
}

static int mbench_unpack(struct vaccel_session *session,
		const char *library, const char *fn_symbol, void *read,
		size_t nr_read, void *write, size_t nr_write)
{
	struct vaccel_arg *read_args = (struct vaccel_arg*)read;
	int time;
	void *buf;

	vaccel_debug("Calling mbench for session %u", session->session_id);

	if (strcmp("mbench", library) != 0 && strcmp("mbench", fn_symbol) != 0)
		return VACCEL_EINVAL;

	if (nr_read < 2)
		return VACCEL_EINVAL;

	time = atoi(read_args[0].buf);
	buf = (void *)read_args[1].buf;

	return mbench(time);
}
struct vaccel_op ops[] = {
	VACCEL_OP_INIT(ops[0], VACCEL_EXEC, mbench_unpack),
};

static int init(void)
{
	return register_plugin_functions(ops, sizeof(ops) / sizeof(ops[0]));
}

static int fini(void)
{
	return VACCEL_OK;
}

VACCEL_MODULE(
	.name = "mbench",
	.version = "0.1",
	.init = init,
	.fini = fini
)
