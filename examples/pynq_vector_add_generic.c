#include <stdlib.h>
#include <stdio.h>

#include <vaccel.h>

int main()
{
	int ret;
	struct vaccel_session sess;

	float a[6] = { 5.0, 1.0, 2.1, 1.2, 5.2, 8.15 };
	float b[5] = { -2.2, 1.1, 6.4, 2.3, 6.1 };
	float c[6] = { 0, 0, 0, 0, 0 };
	size_t len_a = sizeof(a) / sizeof(a[0]);
	size_t len_b = sizeof(b) / sizeof(b[1]);
	size_t len_c = (len_a > len_b ? len_a : len_b);

	ret = vaccel_sess_init(&sess, 0);
	if (ret != VACCEL_OK) {
		fprintf(stderr, "Could not initialize session\n");
		return 1;
	}

	printf("Initialized session with id: %u\n", sess.session_id);
	enum vaccel_op_type op_type = VACCEL_F_VECTORADD;
	struct vaccel_arg read[3] = {
		{ .size = sizeof(enum vaccel_op_type), .buf = &op_type},
		{ .size = sizeof(a), .buf = (char*)a},
		{ .size = sizeof(b), .buf = (char*)b}
	};
	struct vaccel_arg write[1] = {
		{ .size = (len_a > len_b ? len_a : len_b), .buf = c},
	};

	//for (int i = 0; i < atoi(argv[2]); ++i) {
	ret = vaccel_genop(&sess, read, 3, write, 1);
	if (ret) {
		fprintf(stderr, "Could not run op: %d\n", ret);
		goto close_session;
	}

	//ret = vaccel_fpga_vadd(&sess, a, b, c, len_a, len_b);

	for (int i = 0; i < (int)len_c; i++) {
		printf("%f\n", c[i]);
	}

	// works    

 close_session:
	if (vaccel_sess_free(&sess) != VACCEL_OK) {
		fprintf(stderr, "Could not clear session\n");
		return 1;
	}

	return ret;
}
