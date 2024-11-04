# Mock functions

Mock functions are dummy functions mimicking the behaviour of real ones. They
can be used to emulate functionality when the actual implementation cannot be
controlled (ie. calls to external libraries). Function mocking is useful in unit
testing for creating isolated, self-contained tests.

For basic mock functions we use ```fff.h``` (fake function framework). An
example implementation can be found at
```test/core/test_misc_mock.cpp```:
```cpp
#include <catch.hpp>
#include <fff.h>
#include <utils.hpp>

DEFINE_FFF_GLOBALS;

#include <vaccel.h>

extern "C" {
FAKE_VALUE_FUNC(int, get_available_plugins, enum vaccel_op_type);
}

TEST_CASE("get_plugins_mock", "[core_misc]")
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

Let us walk this through step by step.

We first include and initialize fff:
```cpp
...
#include <fff.h>
...
DEFINE_FFF_GLOBALS;
```

To declare a mock function, we use
```FAKE_VALUE_FUNC(<return type>, <name of function>, <arguments>)```.
In this example we want to alter the functionality of
```get_available_plugins()```:
```cpp
FAKE_VALUE_FUNC(int, get_available_plugins, enum vaccel_op_type);
```

Then inside ```TEST_CASE``` we can control function arguments/return:
```cpp
get_available_plugins_fake.return_val = 15;
```

Now when ```get_available_plugins()``` is called in our test, it will return
```15```, which is self explanatory here.
