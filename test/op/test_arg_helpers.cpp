#include <catch.hpp>
#include <cstdint>
#include <utils.hpp>

extern "C"
{
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <vaccel.h>
}

#include "vaccel_args.c"

struct mydata
{
	uint32_t size;
	int* array;
};

void* ser(void* buf, uint32_t* bytes){
	struct mydata* non_ser = (struct mydata*)buf;

	uint32_t size = (non_ser->size + 1) * sizeof(int);
	int* ser_buf = (int*)malloc(size);

	memcpy(ser_buf, (int*)(&non_ser->size), sizeof(int));

	for(int i=0; i<(int)non_ser->size; i++)
		memcpy(&ser_buf[i+1], &non_ser->array[i], sizeof(int));


	*bytes = size;
	return ser_buf;
}


void* deser(void* buf, uint32_t bytes){
    printf("bytes: %d\n", (int)bytes);
	int *ser_buf = (int*)buf;
	int size = ser_buf[0];

	struct mydata* new_buf = 
		(struct mydata*)malloc(sizeof(struct mydata));

	new_buf->size = (uint32_t)size;
	new_buf->array = (int*)malloc(new_buf->size * sizeof(int));
	for(int i=0; i<size; i++)
		memcpy(&new_buf->array[i], &ser_buf[i+1], sizeof(int));

	return new_buf;
}

   /*
    *
    * The code below performs Unit Testing to vaccel arguments 
    * helpers. More specifally, the functions that are getting 
    * tested are:
    * 
    * 1) vaccel_args_init()
    * 2) delete_arg_list()
    *   
    *   -which are related to initialization and deletion of the 
    *   -argument list in main program.
    * 
    * 3) vaccel_add_ser_arg()
    * 4) vaccel_add_deser_arg()
    *   
    *   -which are the functions which the user might use to add an
    *   -argument in an arg-list (in main program).
    * 
    * 5) vaccel_expect_ser_arg()
    * 6) vaccel_expect_deser_ser_arg()
    * 
    *   -which are used to define the type of the argument/s that
    *   -you 'expect' to receive by the plugin (used in main program)
    * 
    * 7) vaccel_extract_ser_arg()
    * 8) vaccel_extract_deser_arg()
    * 
    *   -which are used to extract an argument that was packed in an
    *   -arg-list (used both in main program and plugin)
    * 
    * 9)  vaccel_write_ser_arg()
    * 10) vaccel_write_deser_arg() 
    * 
    *   -which are used to write to an already-defined argument
    *   -(used in plugin)
    *  
    */


TEST_CASE("exec_helpers")
{
    int ret;
    struct mydata mydata_instance;
    mydata_instance.size = 5;
	mydata_instance.array = (int*)malloc(5*sizeof(int));
	for(int i=0; i<5; i++)
        mydata_instance.array[i] = i+1;

    uint32_t bytes;
    void* serbuf = ser(&mydata_instance, &bytes);


    // vaccel_args_init() Testing
    struct vaccel_arg_list* read  = vaccel_args_init(1);
    struct vaccel_arg_list* write = vaccel_args_init(1);

    if (!read || !write){
        ret = 1;
    }
    REQUIRE(ret != 1);

    REQUIRE(read != NULL);
    REQUIRE(read->size == 1);
    REQUIRE(read->list != NULL);

    REQUIRE(write != NULL);
    REQUIRE(write->size == 1);
    REQUIRE(write->list != NULL);


    // vaccel_add_ser_arg() Testing
    int input_int = 15;
    ret = vaccel_add_serial_arg(read, &input_int, sizeof(int));
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(read->size == 1);
    REQUIRE(read->list[0].size == sizeof(int));
    REQUIRE(read->list[0].argtype == 0);
    REQUIRE(read->list[0].buf == &input_int);

    int output_int;
    ret = vaccel_expect_serial_arg(write, &output_int, sizeof(output_int));
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(write->size == 1);
    REQUIRE(write->list[0].size == sizeof(int));
    REQUIRE(write->list[0].argtype == 0);
    REQUIRE(write->list[0].buf == &output_int);

    ret = vaccel_delete_args(write);
    REQUIRE(ret == VACCEL_OK);
    ret = vaccel_delete_args(read);
    REQUIRE(ret == VACCEL_OK);
    free(serbuf);
}
