# How to pass arguments when running a vAccel operation
When running an operation in vAccel, we would probably provide some input and expect some output:
```c
ret = vaccel_op( ..., <read args>, <num_read>, <write args>, <num_write>);
```
Each argument is passed to the operation using the following structure:
```c
struct vaccel_arg {
    uint32_t argtype;
    uint32_t size;
    void *buf;
};
```

If we are not willing to pass each of the arguments by hand, we could use the structure below:

```c
struct vaccel_arg_list;
```
which can be initialized with the function:

```c
vaccel_args_init()
```
For example, in case that we want to pass both input and output arguments, we should initialize two instances of that structure like that:

```c
struct vaccel_arg_list* read   = vaccel_args_init(<NUM_ARGS>);
struct vaccel_arg_list* write  = vaccel_args_init(<NUM_ARGS>);
```

## Main Program
### Add an input argument
Let's say we want to call an operation with two input arguments. One integer and one other kind of structure which is not already serialized (eg a struct that contains pointers)

#### Add already serialized input argument
In the first case, we simply define the address in which our input is located. For example:
```c
int input = ...;
ret = vaccel_add_serial_arg(read, &input, sizeof(input));
if (ret != VACCEL_OK) {
  /* A problem occurred */
}
```
#### Add non-serialized input argument
However, in case our argument is not serialized, we can't use the previous function. For example, we may need to pass as an argument a struct that contains pointers. For instance, the following structure:
 ```c
struct mydata
{
    uint32_t size;
    int* array;
};
 ```

Unfortunately, it's meaningless to transfer such data remotely, since the pointers won't have any sense to a **different address space**. For that reason, we have to serialize that data before it's used. Actually, we can use ```vaccel_add_nonserial_arg()``` and provide a function that will serialize the data. That function must have the following signature:
```c
void* serializer(void*, uint32_t*);
```

And the rationale of its functionality:

```c
void* serializer(void *nonserial_buf, uint32_t* bytes){

  /* Read the structure */
  struct mydata *input = (struct mydata*) nonserial_buf;

  /* Allocate space for the serialized buffer */
  void *serial_mydata = ... ;


 /*
  * Process `input` in order to create a serialized  
  * form of `mydata`, and write it to `serial_mydata`
  */


  /* Provide the size of the serialized data */
  *bytes = ... ;

  /* `serial_mydata` now contains raw data */
  return serial_mydata;
}
```

Afterwards, we can use the above function like that:
```c
struct mydata arg_mydata= init_mydata();
uint32_t type = 0;
ret = vaccel_add_nonserial_arg(read, &arg_mydata, type, serializer);
if (ret != VACCEL_OK) {
  /* A problem occurred */
}
```

### Add an output argument
Now, since we may expect by the operation to return some output, we should provide some space in which they will be written:

#### Add already serialized output argument
In case we expect to get back a float, we simply add the following command:
```c
float out;
ret = vaccel_expect_serial_arg(write, &out, sizeof(out));
if (ret != VACCEL_OK) {
  /* A problem occurred */
}
```
#### Add non-serialized output argument
Similarly, we call the corresponding function for non-serialized arguments. However we don't define any buffer, because the returned data will be meaningless after the operation ends, since it won't have been deserialized yet. Later (after calling the operation), we can ask for the output using their indices, along with a deserializer function. We also provide the size of the expected output as a parameter.

Let's assume that we expect an instance of ```struct mydata```. We just run:
```c
uint32_t expected_size = ... ;
ret = vaccel_expect_nonserial_arg(write, expected_size);
if (ret != VACCEL_OK) {
  /* A problem occurred */
}
```
As you can see we did't define the type of the output, since it will be raw data, prior to its deserialization.

### Run the operation
Since we have prepared our arguments, we can call the operation like that:
```c
ret = vaccel_op( ..., read->list, read->size, write->list, write->size);
```
### Read the output arguments
After the operation returns, we can ask for the output using the "**extract**" functions.

#### Read already serialized output arguments
For serialized data (i.e. the float we expect) we have direct access by the buffer we provided:
```c
float ret_val = out;
```
However, we can also ask for that location by calling:
```c
float *outptr = vaccel_extract_serial_arg(write->list, 0);
/* where 0 is the index of our output */
```

#### Read non-serialized output arguments
On the other hand, for non-serialized output, we cannot just ask for the value, since the returned data has been serialized. Thus, we provide a deserializer function to retrieve the structure. This deserializer must have the following signature:

```c
void* deserializer(void*, uint32_t);
```
And the rationale of its functionality:
```c
void* deserializer(void* serial_buf, uint32_t bytes){
  
  /* Allocate space for the structure */
  struct mydata *new_mydata = ... ;

  
  /* 
   * Process `serial_buf` in order to retrieve  
   * the real data and write it to `new_mydata`
   */
   

  /* new_mydata now contains the retrieved structure */
  return new_mydata;
}
```
And we use it like this:
```c
struct mydata *outbuf;
outbuf = vaccel_extract_nonserial_arg(write->list, 1, deserializer);
/* where 1 is the index on the arguments' list */
/* the memory that `outbuf` points to, must be freed later */
```
**It's up to the user to free the memory that vaccel_extract_nonserial_arg() returns**

### Clean the memory

Finally, we can clean the memory that was allocated by the argument lists by running:
```c
ret = vaccel_delete_args(read);
if (ret != VACCEL_OK) {
  /* A problem occurred */
}

ret = vaccel_delete_args(write);
if (ret != VACCEL_OK) {
  /* A problem occurred */
}
```
## Plugin

Inside the plugin, we want to read the input, process it and return the result back to the main program.

### Read the arguments
The "**extract**" functions that were previously used, will be used again to receive the input. The rationale remains the same.

#### Read already serialized arguments
```c
static int plugin_func(struct vaccel_arg *read,
        size_t nr_read, struct vaccel_arg *write, size_t nr_write){

  /* we expect an integer at index 0 */
  int *input = vaccel_extract_serial_arg(read->list, 0);

  ...
}
```

#### Read non-serialized arguments
```c
static int plugin_func(struct vaccel_arg *read,
        size_t nr_read, struct vaccel_arg *write, size_t nr_write){

  ...

  /* we also expect an instance of "mydata" at index 1 */
  struct mydata *inbuf;
  inbuf = vaccel_extract_nonserial_arg(read->list, 1, deserializer);

  ...
}
```
**The address returned by vaccel_extract_nonserial_arg() contains allocated memory. It must be freed after the data is read.**

### Write to output arguments
Finally, we want to write the response of the plugin to the buffers that were previously defined. For that reason we use the "**write**" functions
#### Write already serialized arguments
Since the user defined that expects to receive a float at index 0, we can provide the value with the following function:
```c
...
float response = ...;
ret = vaccel_write_serial_arg(write->list, 0, &response);
if (ret != VACCEL_OK) {
  /* A problem occurred */
}
...
```
#### Write non-serialized arguments

Similarly, it was also defined that a non-serialized structure will be received at index 1. So:
```c
...
struct mydata out = ...;
ret = vaccel_write_nonserial_arg(write->list, 1, &out, serializer);
if (ret != VACCEL_OK) {
  /* A problem occurred */
}
...
```
