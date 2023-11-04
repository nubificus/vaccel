## Writing tests

To write basic tests:

In order to write a test for a ```test.c``` using the cpp framework catch2. ```test.cpp``` will be the file we write our tests in.

- include the header file which you want to test alongside the catch2 header file as well as the relavent dependencies.

```cpp
#include <catch2/catch_test_macros.hpp>

extern "C"{
#include "test_header.h"
#include ...
}
```

> Note: some files use the ```<stdatomic>``` library/types and you may get an error with ```atomic_uint refcount``` etc. Workaround:

```cpp
#include <catch2/catch_test_macros.hpp>

#include <atomic>'

using atomic_int = std::atomic<int>;
using atomic_uint = std::atomic<unsigned int>;

extern "C"{
#include "test_header.h"
#include ...
}
```

### Basic tests

Writing a test is straight forward. The basic test can be written using a ```TEST_CASE``` and ```REQUIRE``` and ```CHECK``` being the only tests assertions you need. ```REQUIRE``` check aborts the test case when it fails whereas in ```CHECK``` continues the test even if fails
```cpp
TEST_CASE(<name of test>)
{
   output = some_function(); /// expect 5
   CHECK(output == 4); /// does not abort the test
   REQUIRE(output == 5);
}
```


So far in our test we also use ```SECTIONS``` for nesting test cases within a test. This is equivalent to test fixtures, eventhough catch2 do provide a more traditional approach to test fixtures if needed.

```cpp
#include <catch2/catch_test_macros.hpp>

#include <atomic>'

using atomic_int = std::atomic<int>;
using atomic_uint = std::atomic<unsigned int>;

extern "C"{
#include "test_header.h"
#include ...
}

TEST_CASE("basic_test") {
    
    int result;
    result = do_something();
    REQUIRE(result == expected);

    SECTION("A")
    {
        result = do_something_else_A();
        REQUIRE(result == not_expected); /// this will fail
    }

    SECTION("B")
    {
        result = do_something_else_B();
        REQUIRE(result == expceted); /// this will pass
    }
}
```
When we run these tests, we will run this test two times, one which runs the initial code at the start and followed by section A, whereas the other one is followed by section B. From our test runner results we would get an ouput of 1 pass and 1 fail.

#### Mock functions

For basic mock functions we use ```fff.h``` (fake function framework).
As an example, in ```test_misc.cpp``` a function is mocked by:


```cpp
#include <catch2/catch_test_macros.hpp>
#include <fff.h>
DEFINE_FFF_GLOBALS;

extern "C"{
#include "misc.h"
FAKE_VALUE_FUNC(int, get_available_plugins, enum vaccel_op_type);
}

TEST_CASE("vaccel_get_plugins", "[vaccel_get_plugins]")
{

    struct vaccel_session session;
    session.session_id = 123;

    SECTION("return correct implementation")
    {
        get_available_plugins_fake.return_val = 15;
        int result = vaccel_get_plugins(&session, VACCEL_NO_OP);
        REQUIRE(result == 15);
    }

    ...

}
```

Add the following at the top: 
```cpp
#include <fff.h>
DEFINE_FFF_GLOBALS;
```
To declare a mocked function, use ```FAKE_VALUE_FUNC(return type, name of function, arguments)``` - this may change depending on the function you use.

```cpp
get_available_plugins_fake.return_val = 15;
```
when ```get_available_plugins()``` is called in our test, this will return 15 which is self explanatory here.

### Adding tests to the test runner

We do not need to include a main function in our tests if we link our tests with ```Catch2::Catch2WithMain```.

To include tests, modify the ```CMakeLists.txt``` file in the ```test``` directory.

Basic template to include tests:


```
add_executable(NAME_OF_TEST ${PROJECT_SOURCE_DIR}/test/TEST.cpp)
target_include_directories(NAME_OF_TEST
    PRIVATE
        ${INCLUDE_LOCATIONS}
)
target_compile_options(NAME_OF_TEST PRIVATE -Wall -Wextra -g -ggdb --coverage -lgcov)
target_link_libraries(NAME_OF_TEST PRIVATE Catch2::Catch2WithMain gcov --coverage)
```

To make sure the tests run, add the following lines below ```enable_testing()```

```
add_test(NAME OUTPUT_NAME COMMAND NAME_OF_TEST)
```

If you would like to add some environment properties, i.e setting ```VACCEL_DEBUG_LEVEL``` etc. You can do so by:

```
set_tests_properties(
    OUTPUT_NAME
    PROPERTIES
    ENVIRONMENT "VACCEL_DEBUG_LEVEL=4"
    ARGS=--order rand --warn NoAssertions
)
```


