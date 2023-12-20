#### Mock functions

For basic mock functions we use ```fff.h``` (fake function framework).
As an example, in ```test_misc.cpp``` a function is mocked by:


```cpp
#include "../catch2/catch.hpp"
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


Let us work through this step by step:



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

And now we can use this mocked function for our testing. Useful for unit testing files in isolation without any dependence on other functions working correctly.