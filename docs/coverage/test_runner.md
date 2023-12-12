## Adding tests to the test runner

To include tests, modify the ```CMakeLists.txt``` file in the ```test``` directory.

Quickest way is to look at which ```set``` of ```include``` files you need, either ```core``` or ```api``` and add the ```test_name``` into the set.

```API``` enables a plugin backend when running the tests which is useful if you are testing operations implemented by a plugin.

```
set(TESTS_CORE
    test_example
    test_plugin
    test_session
    test_misc
    test_resource
    test_id_pool
    test_vaccel
    test_log
    .
    .
    test_name
)
```

Alongside this:

**Add to one of the ```set_tests_properties``` functions to enable the test to be ran.**

#### For manual tests/more control over what you would like:

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

We do not need to include a main function in our tests if we link our tests with ```Catch2::Catch2WithMain```. If you want to include a main function for your tests - refer to the catch documentation for more help.


To make sure the tests run, add the following lines below ```enable_testing()```

```
add_test(NAME OUTPUT_NAME COMMAND NAME_OF_TEST)
```

### Adding additional environment properties

If you would like to add some environment properties, i.e setting ```VACCEL_DEBUG_LEVEL``` etc. You can do so by:

```
set_tests_properties(
    OUTPUT_NAME
    PROPERTIES
    ENVIRONMENT "VACCEL_DEBUG_LEVEL=4"
    ARGS=--order rand --warn NoAssertions
)
```
