#include <stdlib.h>
#include <stdio.h>

#include <vaccel.h>

int main()
{
	int ret;
	struct vaccel_session sess;
	
    float a[5] = {5.0,1.0,2.1,1.2,5.2};
    float b[5] = {-2.2,1.1,6.4,2.3,6.1};
    float c[5] = {0,0,0,0,0};
    size_t len_a = sizeof(a)/sizeof(a[0]);
    size_t len_b = sizeof(b)/sizeof(b[1]); 

    

	ret = vaccel_sess_init(&sess, 0);
	if (ret != VACCEL_OK) {
		fprintf(stderr, "Could not initialize session\n");
		return 1;
	}




	printf("Initialized session with id: %u\n", sess.session_id);


	ret = vaccel_fpga_vadd(&sess, a,b,c,len_a,len_b);
	if (ret) {
		fprintf(stderr, "Could not run op: %d\n", ret);
		goto close_session;
	}
    
    for (int i=0; i < 5 ;i++) {
    printf("%f\n",c[i]);
    }


    // works    



    

close_session:
	if (vaccel_sess_free(&sess) != VACCEL_OK) {
		fprintf(stderr, "Could not clear session\n");
		return 1;
	}


	return ret;
}