## FAQ
---
#### Backend not set properly / operation error
If you keep getting error to do with operation not found: Check if the backend is set for the test accordingly. Most likely to be in the ```set_tests_properties```

---
#### Allowing access to static data members
If you want to test a file with a struct inside of it (to keep track of something) or with many static functions - either include it within the test itself as:

```#include "test_file.c"```.

This gives us the access to all the static data members within that file - then we can test the file as normal. Or you can try and include it within the CMake file itself by setting it as a second source.

```add_executable(test_name ${PROJECT_SOURCE_DIR}/unit/test_file.cpp ${source1})```

---
#### Running tests individually
It is possible to run the tests individually as well:

```
.../build$ test/test_name

Randomness seeded to: 985339580
===============================================================================
All tests passed (100 assertions in 5 test cases)
```

---
#### Coverage build locally
If you want, you can run a coverage run locally by:

```
cd ./src/
patch <../test/coverage.patch
```
This adds the --coverage tag to the src files in order for them to show up on the report

and then:

```
cd build
cmake ../ -DBUILD_PLUGIN_NOOP=ON -DENABLE_TESTS=ON -DENABLE_COVERAGE=ON
make && make test
```
to generate a coverage report (let's say as a html):

```
cd src
```
```
gcovr --cobertura --gcov-executable gcov-12 -o ./test/docs/coverage.xml --print-summary -e "build/_deps/*" --html-details --html=./
```

----
#### Running valgrind locally
From root directory:
```
valgrind ./build/test/test_name
``````
If the test requires a backend, remember to set it as a environment property by:
```
env VACCEL_BACKENDS=./build/plugins/noop/libvaccel-noop.so valgrind build/test/test_name
```

