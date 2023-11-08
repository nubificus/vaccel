#include <catch2/catch_test_macros.hpp>

#include <atomic>

using atomic_int = std::atomic<int>;
using atomic_uint = std::atomic<unsigned int>;

extern "C"{
#include <stdlib.h>
#include <stdio.h>

#include <vaccel.h>
#include <ops/tf.h>

#include "session.h"
#include "tf_saved_model.h" 
}

#define min(a, b) ((a) < (b) ? (a) : (b))

TEST_CASE("base"){
    REQUIRE(1==1);
}

TEST_CASE("main")
{
	struct vaccel_session vsess;
	struct vaccel_tf_saved_model model;
    struct vaccel_tf_status status;
    int ret;

    const char *model_path = "../../test/models/tf/lstm2";

    ret = vaccel_tf_saved_model_set_path(&model, model_path);
    REQUIRE(ret == 0);
    
    ret = vaccel_tf_saved_model_register(&model);
    REQUIRE(ret == 0);

    ret = vaccel_sess_init(&vsess, 0);
    REQUIRE(ret == 0);

    printf("Initialized vAccel session %u\n", vsess.session_id);

    ret = vaccel_sess_register(&vsess, model.resource);
    REQUIRE(ret == 0);

    ret = vaccel_tf_session_load(&vsess, &model, &status);
    REQUIRE(ret == 0);


    struct vaccel_tf_buffer run_options = { NULL, 0 };
    const char* in_node_name = "serving_default_input_1";

    struct vaccel_tf_node in_node = { const_cast<char*>(in_node_name), 0 };

    uint32_t dims[] = {1, 30};
    float data[30];
	for (int i = 0; i < 30; ++i)
		data[i] = 1.00;

    struct vaccel_tf_tensor *in = vaccel_tf_tensor_new(2, dims, VACCEL_TF_FLOAT);
    REQUIRE(in);

    in->data = data;
    in->size = sizeof(float) * 30;

    const char* out_node_name = "StatefulPartitionedCall";
	
    struct vaccel_tf_node out_node = { const_cast<char*>(out_node_name), 0 };
	struct vaccel_tf_tensor *out;

	ret = vaccel_tf_session_run(&vsess, &model, &run_options,
			&in_node, &in, 1,
			&out_node, &out, 1,
			&status);
    
    REQUIRE(ret == 0);

    printf("Success!\n");
	printf("Output tensor => type:%u nr_dims:%u\n", out->data_type, out->nr_dims);
	for (int i = 0; i < out->nr_dims; ++i)
		printf("dim[%d]: %d\n", i, out->dims[i]);
		
	printf("Result Tensor :\n");
	float *offsets = (float *)out->data;
	for (unsigned int i = 0; i < min(10, out->size / sizeof(float)) ; ++i)
		printf("%f\n", offsets[i]);

}   