
#include <stdlib.h>
#include <stdio.h>

#include <vaccel.h>

int main()
{
	int ret;
	struct vaccel_session sess;

	int a[6] = { 12, 15, 12, 15, 12, 11 };
	int b[6];
	//size_t len_a = sizeof(a) / sizeof(a[0]);
	//size_t len_b = sizeof(b)/sizeof(b[1]); 

	ret = vaccel_sess_init(&sess, 0);
	if (ret != VACCEL_OK) {
		fprintf(stderr, "Could not initialize session\n");
		return 1;
	}

	printf("Initialized session with id: %u\n", sess.session_id);

	enum vaccel_op_type op_type = VACCEL_F_ARRAYCOPY;
	struct vaccel_arg read[2] = {
		{ .size = sizeof(enum vaccel_op_type), .buf = &op_type},
		{ .size = sizeof(a), .buf = (char*)a}
	};
	struct vaccel_arg write[2] = {
		{ .size = sizeof(b), .buf = b},
	};

	//for (int i = 0; i < atoi(argv[2]); ++i) {
		ret = vaccel_genop(&sess, read, 2, write, 1);
		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto close_session;
		}

#if 0
	ret = vaccel_fpga_arraycopy(&sess, a, b, len_a);
	if (ret) {
		fprintf(stderr, "Could not run op: %d\n", ret);
		goto close_session;
	}
#endif

	for (int i = 0; i < 6; i++) {
		printf("%i\n", b[i]);
	}

 close_session:
	if (vaccel_sess_free(&sess) != VACCEL_OK) {
		fprintf(stderr, "Could not clear session\n");
		return 1;
	}

	return ret;
}
