# Writing tests

vAccel uses the [Catch2](https://github.com/catchorg/Catch2) framework to
implement unit tests. Tests reside in ```test``` directory and are named after
the component they refer to, ie. ```test_session.c``` includes unit tests for
```session.c```.

To implement a new test first include the Catch2 and vaccel headers:
```cpp
#include <catch2/catch_test_macros.hpp>
#include <vaccel.h>
```

Some optional common utilities for tests a can be found at ```test/utils.hpp```.

The basic operations for writing tests are ```TEST_CASE```, ```REQUIRE``` and
```CHECK```. A ```TEST_CASE``` represents a discrete test unit, ```REQUIRE``` is
used to abort a test based on a condition and ```CHECK``` will just evaluate a
condition but will not make the test fail even if it is false:
```cpp
TEST_CASE(<name of test>)
{
   output = some_function(); // expect 5
   CHECK(output == 4); // does not abort the test
   REQUIRE(output == 5);
}
```

Additionally, ```SECTIONS``` can be used to create isolated sub-tests for a
```TEST_CASE```. The code in a ```SECTION``` alters state only inside this
section and not in other sections. In other words, code outside of
```SECTION```s is common between them, but code inside a ```SECTION``` is only
applicable to the section itself:
```cpp
#include <catch2/catch_test_macros.hpp>
#include <vaccel.h>
#include <utils.hpp>

TEST_CASE("basic_test") {
    int result;
    result = do_something();
    REQUIRE(result == expected);

    SECTION("A")
    {
        result = do_something_else_A();
        REQUIRE(result == not_expected); // this will fail
    }

    SECTION("B")
    {
        result = do_something_else_B();
        REQUIRE(result == expected); // this will pass
    }

    // result has the value of do_something() here
}
```

After writing a test you need to add it to the build system so you can use it.
[Adding tests to Meson](adding_tests.md) describes the process of adding tests to
vAccel's meson implementation.
