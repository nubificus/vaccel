# How to use vAccel Arg

vAccel operations based on `GenOp` (like `Exec`) and `GenOp` itself expect
arguments in a format like:

```c
ret = vaccel_op( ..., <read args>, <num_read>, <write args>, <num_write>);
```

where `<read args>`/`<write args>` are arrays of vAccel Args.

The vAccel Arg structure (`struct vaccel_arg`) essentially provides a way to
"serialize" arguments into a common format, that can be used in  a generic
interface for defining operations:

```c
struct vaccel_arg {
    /* arg data */
    void *buf;

    /* size of the arg data */
    size_t size;

    /* type of the arg data */
    vaccel_arg_type_t type;

    /* ID of custom type, if type is VACCEL_ARG_CUSTOM */
    uint32_t custom_type_id;

    /* true if arg data is allocated by the API */
    bool owned;
};
```

vAccel Args can be created directly from other variables using the
`vaccel_arg_init*()`/`vaccel_arg_new*()` set of calls. For example:

```c
int32_t myarg = 10;

struct vaccel_arg varg;
ret = vaccel_arg_init_from_buf(&varg, &myarg, sizeof(int32_t), VACCEL_ARG_INT32,
                               0);
```

The code above creates the `varg` vAccel Arg that points to `myarg` (without
owning its memory).

## vAccel Arg Array

To simplify the process of packing/unpacking vAccel Args to/from C arrays -
since operations commonly expect arrays of Args - the vAccel Arg Array wrapper
can be used:

```c
struct vaccel_arg_array {
    /* arg array */
    struct vaccel_arg *args;

    /* count of array args; used when adding args */
    size_t count;

    /* total capacity of the array */
    size_t capacity;

    /* sequential array position; used when getting args */
    size_t position;

    /* true if arg array is allocated by the API */
    bool owned;
};
```

The vAccel Arg Array API provides convenience functions to create and iterate
over arrays of vAccel Args, by wrapping the actual C array data type.

For example, a vAccel Arg Array with the `myarg` variable above is created
with:

```c
int32_t myarg = 10;

struct vaccel_arg_array vargs;
ret = vaccel_arg_array_init(&vargs, 1);
ret = vaccel_arg_array_add_int32(&vargs, &myarg);
```

Array arguments can then be retrieved iteratively with:

```c
int32_t curarg;
ret = vaccel_arg_array_get_int32(&vargs, &curarg);
```

This API enables size/type validation while the underlying C array remains
accessible through `vargs.args`.

Variables for all common data types can be added/retrieved to/from a vAccel Arg
Array using the `vaccel_arg_array_add_*()`/`vaccel_arg_array_get_*()` family of
functions.

### Custom vAccel Arg types

In cases where arguments have custom types, ie. `enum myenum`, the
`vaccel_arg_array_add_custom()`/`vaccel_arg_array_get_custom()` functions can be
used to specify a custom validation function. For example:

```c
bool my_validate_func(const void *buf, size_t size, uint32_t custom_id)
{
    // Custom validation logic
    // ...

    return true;
}

typedef enum {
    ENUM_ITEM
} myenum_t;
myenum_t myarg;

ret = vaccel_arg_array_add_custom(
    vargs, MY_CUSTOM_DTYPE,
    &myarg, sizeof(myarg),
    my_validate_func);

myenum_t prevarg;
size_t prevarg_size;
ret = vaccel_arg_array_get_custom(
    vargs, MY_CUSTOM_DTYPE,
    (void **)&prevarg, &prevarg_size,
    my_validate_func);
```

### Custom vAccel Arg serializers/deserializers

Sometimes, it might be desirable to serialize a complex argument, ie.
`struct mystruct`, into a buffer using a custom binary format. This can be
achieved using
`vaccel_arg_array_add_serialized()`/`vaccel_arg_array_get_serialized()`. For
example:

```c
int my_serializer_func(const void *data, size_t data_size, uint32_t custom_id,
                      void **buf, size_t *size)
{
    // Custom serialization logic
    // ...

    uint32_t *ser_buf = malloc(ser_size);
    if (!ser_buf)
        return VACCEL_ENOMEM;

    *buf = ser_buf;
    *size = ser_size;
    return VACCEL_OK;
}

int my_deserializer_func(const void *buf, size_t size, uint32_t custom_id,
                        size_t data_size, void *out_data)
{
    uint32_t *ser_buf = (uint32_t *)buf;
    struct mydata *deser_buf = (struct mydata *)out_data;

    // Custom deserialization logic
    // ...

    return VACCEL_OK;
}

struct mystruct myarg;
ret = vaccel_arg_array_add_serialized(
    &vargs, VACCEL_ARG_CUSTOM,
    MY_CUSTOM_DTYPE, &myarg, sizeof(myarg),
    my_serializer_func);

struct mystruct prevarg;
ret = vaccel_arg_array_get_serialized(
    &vargs, VACCEL_ARG_CUSTOM,
    MY_CUSTOM_DTYPE, &prevarg, sizeof(prevarg),
    my_deserializer_func);
```

A full example implementation can be found in the
[exec_serialized](../examples/exec_serialized.c) example

For all the available vAccel Arg types, functions and helpers you can look at
the [vAccel Arg header](../src/include/vaccel/arg.h).
