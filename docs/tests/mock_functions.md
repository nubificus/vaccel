# Mock functions

Mock functions are dummy functions mimicking the behaviour of real ones. They
can be used to emulate functionality when the actual implementation cannot be
controlled (ie. calls to external libraries). Function mocking is useful in unit
testing for creating isolated, self-contained tests.

For basic mock functions we use ```fff.h``` (fake function framework). A mock
implementation and its' use can be found in ```test/mock_virtio.cpp``` and
```test/core/test_session.cpp``` respectively.

A deducted example is provided below:
```cpp
// test/mock_virtio.cpp

#include "vaccel.h"

static struct vaccel_plugin plugin;
static struct vaccel_plugin_info plugin_info;

...

auto mock_virtio_plugin_virtio() -> struct vaccel_plugin *
{
    plugin_info.name = "fake_virtio";
...
    // set rest of the struct members
...
    plugin.info = &plugin_info;

    return &plugin;
}
```

```cpp
// test/core/test_session.cpp

#include "vaccel.h"
#include <catch.hpp>
#include <fff.h>
...

#include <mock_virtio.hpp>

DEFINE_FFF_GLOBALS;

extern "C" {
FAKE_VALUE_FUNC(struct vaccel_plugin *, plugin_virtio);
}
...

TEST_CASE("session_virtio", "[core][session]")
{
    int ret;
...
    RESET_FAKE(plugin_virtio);

    plugin_virtio_fake.custom_fake = mock_virtio_plugin_virtio;
...
    // call functions using plugin_virtio()
...
    REQUIRE(plugin_virtio_fake.call_count == <nr of function calls>);
}
```

Let us walk this through step by step.

We first create a mock function that does the necessary argument modifications
and returns the expected result - here `mock_virtio_plugin_virtio()`.

Then, to actually use our function we need to include and initialize fff:
```cpp
...
#include <fff.h>
...
DEFINE_FFF_GLOBALS;
```

To declare a mock function, we use
```FAKE_VALUE_FUNC(<return type>, <name of function>, <arguments>)```.
In this example we want to alter the functionality of ```plugin_virtio()()```:
```cpp
FAKE_VALUE_FUNC(struct vaccel_plugin *, plugin_virtio);
```

After declaring the function, we need to reset its' state for each
```TEST_CASE```:
```cpp
RESET_FAKE(plugin_virtio);
```

and set the respective variable so it will substitute the actual function:
```cpp
plugin_virtio_fake.custom_fake = mock_virtio_plugin_virtio;
```

After doing any calls using the mock function, we can verify its' use:
```cpp
REQUIRE(plugin_virtio_fake.call_count == <nr of function calls>);
```

Although this example implements a new function to mock the actual one, it is
possible to only define the function return value instead by using
```mock_virtio_plugin_virtio.return_val = <mock value>``` and skipping the
implementation of a mock ```plugin_virtio()``` entirely.
