# Mock functions

Mock functions are dummy functions mimicking the behaviour of real ones. They
can be used to emulate functionality when the actual implementation cannot be
controlled (ie. calls to external libraries). Function mocking is useful in unit
testing for creating isolated, self-contained tests.

For basic mock functions we use ```fff.h``` (fake function framework). A mock
implementation and its' use can be found in ```test/utils/test_net_curl.cpp```.

A deducted example is provided below:

```cpp
// test/utils/test_net_curl.cpp

#include "vaccel.h"
#include <catch.hpp>
#include <fff.h>
// ...
DEFINE_FFF_GLOBALS;

extern "C" {
FAKE_VALUE_FUNC(bool, net_nocurl_path_is_url, const char *);
// ...
}

TEST_CASE("net_path_is_url", "[utils][net][curl]")
{
    RESET_FAKE(net_nocurl_path_is_url);
// ...
    const char *url = "http://nubificus.co.uk";
    const char *word = "hello world";

    SECTION("is url")
    {
        net_nocurl_path_is_url_fake.return_val = true;
        REQUIRE(net_path_is_url(url));
#ifdef USE_LIBCURL
        REQUIRE(net_nocurl_path_is_url_fake.call_count == 0);
#else
        // emulate net_curl_path_is_url()
        REQUIRE(net_nocurl_path_is_url_fake.call_count == 1);
#endif
    }
// ...
}
```

Let us walk this through step by step.

To use a mock function we need to first include and initialize fff:

```cpp
...
#include <fff.h>
...
DEFINE_FFF_GLOBALS;
```

To declare a mock function, we use
```FAKE_VALUE_FUNC(<return type>, <name of function>, <arguments>)```.

In this example we want to alter the functionality of
```net_nocurl_path_is_url()```:

```cpp
FAKE_VALUE_FUNC(bool, net_nocurl_path_is_url, const char *);
```

After declaring the function, we need to reset its' state for each
```TEST_CASE```:

```cpp
RESET_FAKE(net_nocurl_path_is_url);
```

and set the respective variable so it will substitute the actual function value:

```cpp
net_nocurl_path_is_url_fake.return_val = true;
```

After doing any calls using the mock function, we can verify its' usage:

```cpp
REQUIRE(net_nocurl_path_is_url_fake.call_count == <nr of function calls>);
```

Although this example defines a fixed return value for the mock function, it is
possible to define a proper fake function instead by using
```mock_virtio_plugin_virtio.custom_fake = <mock function>```.
