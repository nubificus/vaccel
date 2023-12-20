## Writing tests

To write basic tests:

In order to write a test for a ```test.c``` using the cpp framework catch2. ```test.cpp``` will be the file we write our tests in.

- include the header file which you want to test alongside the catch2 header file as well as the relevant dependencies.

```cpp
#include "../catch2/catch.hpp"

extern "C"{
#include "test_header.h"
#include ...
}
```

> Note: some files use the ```<stdatomic>``` library/types and you may get an error with ```atomic_uint refcount``` etc. Workaround:

```cpp
#include "../catch2/catch.hpp"

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


So far in our test we also use ```SECTIONS``` for nesting test cases within a test. This is equivalent to test fixtures, even though catch2 do provide a more traditional approach to test fixtures if needed.

```cpp
#include "../catch2/catch.hpp"

#include <atomic>

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
        REQUIRE(result == expected); /// this will pass
    }
}
```
When we run these tests, we will run this test two times, one which runs the initial code at the start and followed by section A, whereas the other one is followed by section B. From our test runner results we would get an output of 1 pass and 1 fail.
