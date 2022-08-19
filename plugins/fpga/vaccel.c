#include <stdio.h>
#include <plugin.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <math.h>

#include "sample_common.h" // Implements udmabuf memory and relevants structs and functions: https://github.com/ikwzm/udmabuf

// TODO: Seperate each part of the FPGA functions into their own header files to tidy things up

typedef short data_t;

/* ----------------------------------------------------------------------------------------------------------------------------- */
/* If using SDSoc, the following implementations would be beneficial for memory usage but moreso the #pragma HLS commands */ 
/* For usage in these implementations below check: https://github.com/jl2022s/fpga_samples */

#ifdef __SDSCC__
#include "sds_lib.h"
#else 
#define sds_alloc(x)(malloc(x))
#define sds_free(x)(free(x))
#endif

/* ----------------------------------------------------------------------------------------------------------------------------- */

int uio_irq_on(int uio_fd)
{
    unsigned int  irq_on = 1;
    write(uio_fd, &irq_on, sizeof(irq_on));
}

int uio_wait_irq(int uio_fd)
{
    unsigned int  count = 0;
    return read(uio_fd, &count,  sizeof(count));
}

/* ----------------------------------------------------------------------------------------------------------------------------- */

#pragma SDS data zero_copy(a, b)

void array_zero_copy(struct udmabuf a, struct udmabuf b, size_t N)
{
     for(int i = 0; i < N; i++) {
          ((int*)(b.buf))[i] = ((int*)(a.buf))[i];
     }
}

static int v_arraycopy(struct vaccel_session *session, int *a, int *b, size_t c)
{   
	fprintf(stdout, "Calling Array Copy function (FPGA) %u\n", session->session_id);

    int            uio_fd;
    void*          regs;
    struct udmabuf A_array;
    struct udmabuf B_array;
    
    if ((uio_fd = uio_open("pump-uio")) == -1) {
        printf("Can not open pump-uio\n");
    }

    regs = mmap(NULL, c, PROT_READ|PROT_WRITE, MAP_SHARED, uio_fd, 0);

    if (udmabuf_open(&A_array, "udmabuf4") == -1)
        exit(1);
    
    if (udmabuf_open(&B_array, "udmabuf5") == -1)
        exit(1);

    for (int j = 0; j < c; j++) {
        ((int*)(A_array.buf))[j]  = a[j];
        ((int*)(B_array.buf))[j] = 0;
    }

    array_zero_copy(A_array, B_array, c);

    for (int j = 0; j < c; j++) {
        b[j] = ((int*)(B_array.buf))[j]; 
    }

    udmabuf_close(&A_array);
    udmabuf_close(&B_array);
    close(uio_fd);

	return VACCEL_OK;
}


/* ----------------------------------------------------------------------------------------------------------------------------- */


static int v_vectoradd(struct vaccel_session *session, float *a, float *b, float *c, size_t len_a, size_t len_b)
{
    fprintf(stdout, "Calling Vector Add function (FPGA) %u\n", session->session_id);

    int            uio_fd;
    void*          regs;
    struct udmabuf A_array;
    struct udmabuf B_array;
    struct udmabuf C_array;

    if ((uio_fd = uio_open("pump-uio")) == -1) {
        fprintf(stdout, "Can not open pump-uio\n");
        exit(1);
    }

    if (len_a != len_b){
        fprintf(stdout, "Incorrect inputs");
        exit(2);
    }

    regs = mmap(NULL, 0x1000, PROT_READ|PROT_WRITE, MAP_SHARED, uio_fd, 0);

    
    

    if (udmabuf_open(&A_array, "udmabuf5") == -1)
        exit(1);
    
    if (udmabuf_open(&B_array, "udmabuf3") == -1)
        exit(1);
   
    if (udmabuf_open(&C_array, "udmabuf4") == -1)
        exit(1);


    for (int j = 0; j < len_a; j++) {
        ((float *)(A_array.buf))[j]  = a[j];
        ((float *)(B_array.buf))[j] =  b[j];
    }

    for(int i = 0; i < len_a; i++) {
        #pragma HLS PIPELINE
        #pragma HLS LOOP_TRIPCOUNT min=c_dim max=c_dim
        ((float *)(C_array.buf))[i] = ((float *)(A_array.buf))[i] + ((float *)(B_array.buf))[i];
    }

    


    for (int j = 0; j < len_a; j++) {
        c[j] = ((float*)(C_array.buf))[j];
    }

    

    udmabuf_close(&A_array);
    udmabuf_close(&B_array);
    udmabuf_close(&C_array);


    return VACCEL_OK;
}


/* ----------------------------------------------------------------------------------------------------------------------------- */

// Doesn't do much without HLS here but just to get the basic idea of things:

#define TEST_DATA_SIZE 4096

void vadd_accel(
                struct udmabuf in1,   // Read-Only Vector 1
                struct udmabuf in2,   // Read-Only Vector 2
                struct udmabuf out,         // Output vector
                int dim           // Size of one dimension of the vector
               )
{
    // Performs vector addition over in1 and in2, and
    // writes the result to output
    vadd_write_out: for(int j = 0; j < dim; j++) {
    #pragma HLS PIPELINE
    #pragma HLS LOOP_TRIPCOUNT min=c_size_min max=c_size_max
        ((int*)(out.buf))[j] = ((int*)(in1.buf))[j] + ((int*)(in2.buf))[j];
    }    
}

void vmul_accel(
                 struct udmabuf in1,     // Read-Only Vector 1
                 struct udmabuf in2,     // Read-Only Vector 2
                 struct udmabuf out,           // Output Result
                 int dim             // Size of one dimension of the vector
                )
{
    // Performs vector multiplication over in1 and in2, and
    // writes the result to output
    vmul_write_out: for(int j = 0; j < dim; j++) {
    #pragma HLS PIPELINE
    #pragma HLS LOOP_TRIPCOUNT min=c_size_min max=c_size_max
        ((int*)(out.buf))[j] = ((int*)(in1.buf))[j] * ((int*)(in2.buf))[j];
    }    

}

static int v_parallel(struct vaccel_session *session, float *a, float  *b, float *add_out, float *mult_out, size_t len_a)
{
    fprintf(stdout, "Calling add and multiply operations in parallel %u\n", session -> session_id);

    size_t vector_size_bytes = sizeof(int) * TEST_DATA_SIZE;
    int uio_fd;
    void* regs;

    if ((uio_fd = uio_open("pump-uio")) == -1) {
        fprintf(stdout, "Can not open pump-uio\n");
        exit(1);
    }

    

    regs = mmap(NULL, 0x1000, PROT_READ|PROT_WRITE, MAP_SHARED, uio_fd, 0);

    struct udmabuf source_in1, source_in2, source_vadd_hw_results, source_vmul_hw_results;

    
    
    if (udmabuf_open(&source_in1, "udmabuf3") == -1)
        exit(1);
    if (udmabuf_open(&source_in2, "udmabuf4") == -1)
        exit(1);
    if (udmabuf_open(&source_vadd_hw_results, "udmabuf5") == -1)
        exit(1);
    if (udmabuf_open(&source_vmul_hw_results, "udmabuf2") == -1)
        exit(1);
    
    

    for (int j = 0; j < len_a; j++) {
        ((float *)(source_in1.buf))[j]  = a[j];
        ((float *)(source_in2.buf))[j] =  b[j];
    }

    #pragma SDS async(1)
    vadd_accel(source_in1, source_in2, source_vadd_hw_results, len_a);
    #pragma SDS async(2)
    vmul_accel(source_in1, source_in2, source_vmul_hw_results, len_a);

    // for(int itr = 0; itr < 4; itr++)
    // {
    //    	#pragma SDS wait(1)
	// 	#pragma SDS async(1)
    //    	vadd_accel(source_in1, source_in2, source_vadd_hw_results, size);
	// 	#pragma SDS wait(2)
	// 	#pragma SDS async(2)
    //   	vmul_accel(source_in1, source_in2, source_vmul_hw_results, size);
    // }
    
    #pragma SDS wait(1)
    #pragma SDS wait(2)


    udmabuf_close(&source_in1);
    udmabuf_close(&source_in2);
    udmabuf_close(&source_vadd_hw_results);
    udmabuf_close(&source_vmul_hw_results);

    for (int j = 0; j < len_a; j++) {
        add_out[j] = ((float*)(source_vadd_hw_results.buf))[j];
        mult_out[j] = ((float*)(source_vmul_hw_results.buf))[j];
    }   
    return VACCEL_OK;
}

/* ----------------------------------------------------------------------------------------------------------------------------- */
void madd(float *A, struct udmabuf B, struct udmabuf C, int N)
{
    int i, j;

    for (i = 0; i < N; i++)
        for (j = 0; j < N; j++)
        #pragma HLS PIPELINE II=1
        ((float *)(C.buf))[i*N+j] = A[i*N+j] + ((float *)(B.buf))[i*N+j];
}

void mmult (struct udmabuf A, struct udmabuf B, float *C, int N)
{
        float Abuf[N][N], Bbuf[N][N];
#pragma HLS array_partition variable=Abuf block factor=16 dim=2
#pragma HLS array_partition variable=Bbuf block factor=16 dim=1
     
     for(int i=0; i<N; i++) {
          for(int j=0; j<N; j++) {
#pragma HLS PIPELINE
               Abuf[i][j] = ((float *)(A.buf))[i * N + j];
               Bbuf[i][j] = ((float *)(B.buf))[i * N + j];
            }
        }
     
     for (int i = 0; i < N; i++) {
          for (int j = 0; j < N; j++) {
#pragma HLS PIPELINE
               float result = 0;
               for (int k = 0; k < N; k++) {
                    float term = Abuf[i][k] * Bbuf[k][j];
                    result += term;
                }
                C[i * N + j] = result;
        }
    }
}

static int v_mmult(struct vaccel_session *session, float *a, float  *b, float *c, float *d_out, size_t len_a)
{
    fprintf(stdout, "Calling matrix multiplication and addition (AB + C) %u\n", session -> session_id);
    size_t vector_size_bytes = sizeof(int) * TEST_DATA_SIZE;
    int uio_fd;
    void* regs;
    int array_size = len_a * sizeof(float);

    struct udmabuf A, B, C, D;

    if ((uio_fd = uio_open("pump-uio")) == -1) {
        printf("Can not open pump-uio\n");
        exit(1);
    }

    regs = mmap(NULL, array_size, PROT_READ|PROT_WRITE, MAP_SHARED, uio_fd, 0);
    
    if (udmabuf_open(&A, "udmabuf0") == -1)
        exit(1);
    if (udmabuf_open(&B, "udmabuf1") == -1)
        exit(1);
    if (udmabuf_open(&C, "udmabuf2") == -1)
        exit(1);
    if (udmabuf_open(&D, "udmabuf3") == -1)
        exit(1);

    float tmp[len_a], tmp1[len_a];
    mmult(A, B, tmp1, sqrt(len_a));
    madd(tmp1, C, D, sqrt(len_a));
    
    return VACCEL_OK;
}

struct vaccel_op ops[] = {
	VACCEL_OP_INIT(ops[0], VACCEL_F_ARRAYCOPY, v_arraycopy),
	VACCEL_OP_INIT(ops[1], VACCEL_F_VECTORADD, v_vectoradd),
	VACCEL_OP_INIT(ops[2], VACCEL_F_PARALLEL, v_parallel),
	VACCEL_OP_INIT(ops[3], VACCEL_F_MMULT, v_mmult),
};

static int init(void)
{
	return register_plugin_functions(ops, sizeof(ops)/ sizeof(ops[0]));
}

static int fini(void)
{
	return VACCEL_OK;
}

VACCEL_MODULE(
	.name = "fpga_functions",
	.version = "0.1",
	.init = init,
	.fini = fini
)