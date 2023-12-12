## Running tests

To run tests - ensure you have the following:

 ***The CXX version of at least 12.3.0***


We follow a similar set of instructions to building vAccel:

Create a build directory:
```
mkdir build
cd build
```
Run CMake with test support:
```
cmake ../  -DENABLE_TESTS=ON
``` 
Build the project and execute the tests
```
make
make test
```

For tests requiring a plugin implementation of some operations, use the following command:

```
cmake ../ -DBUILD_PLUGIN_NOOP=ON -DENABLE_TESTS=ON -DBUILD_EXAMPLES=ON
```

After running the tests, you should see an output similar to:
```
    Start  1: test_example
 1/17 Test  #1: test_example .....................   Passed    0.01 sec
 .
 .
 .

    Start 17: test_blas
17/17 Test #17: test_blas ........................   Passed    0.01 sec

100% tests passed, 0 tests failed out of 17
```
### Test failure

Suppose one of our tests fail, our output will look similar to this:

```
94% tests passed, 1 tests failed out of 17

Total Test time (real) =   0.20 sec

The following tests FAILED:
         17 - test_blas (Failed)
Errors while running CTest
Output from these tests are in: /.../feat_vaccel_testing/build/Testing/Temporary/LastTest.log

```

In the log file, we can see where the test failed and what failed:


```
.../test/test_blas.cpp:43
...............................................................................

.../test/test_blas.cpp:66: FAILED:
  REQUIRE( 1 == 0 )

===============================================================================
test cases: 1 | 1 failed
assertions: 4 | 3 passed | 1 failed

<end of output>
```
Quite handy :)



### Verbose tests

It is possible to also do the tests individually as well:

```
.../build$ test/test_session

Randomness seeded to: 985339580
===============================================================================
All tests passed (29 assertions in 5 test cases)
```

To run tests verbosely: - build with ```make test ARGS="-V"```.
NOTE: Do keep in mind, if your test needs a backend, you need to set that accordingly (see FAQ).


and get output similar to this:

```
Start  8: test_log

8: Test command: /path/to/test_log
8: Working Directory: /path/to/build
8: Test timeout computed to be: 10000000
8: Randomness seeded to: 3675778273
8: ===============================================================================
8: All tests passed (3 assertions in 1 test case)
8:
8: 2023.12.11-01:07:05.80 - <debug> Shutting down vAccel
8: 2023.12.11-01:07:05.80 - <debug> Cleaning up plugins
 8/17 Test  #8: test_log .........................   Passed    0.00 sec
```