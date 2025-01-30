# Adding tests to Meson

vAccel's meson implementation builds and generates tests by iterating the test
sources.

To add a new meson test simply add the file in the relevant ```meson.build```'s
sources list. To ie. add a ```test_mytest``` in core tests, modify
```test_core_sources``` in ```test/core/meson.build``` with the file name:
```meson
tests_core_sources = files([
  ...
  'test_mytest.cpp,
])
```

## Setting test-specific arguments

To set different arguments for a specific test you will need to modify the test
generation loops in ```test/meson.build```. A loop for core tests looks like:
```meson
foreach t : tests_core_sources
  name = fs.stem(t)
  exe = executable(name,
    sources : t,
    ...)

  test(name, exe, args : tests_args, env : tests_env, is_parallel : false)
  if name == 'test_log'
    foreach l : range(1, 4)
      test('@0@+@1@@2@'.format(name, 'level', l), exe,
        args : tests_args,
        env : 'VACCEL_LOG_LEVEL=' + l.to_string(),
        is_parallel : false)
    endforeach
  endif
endforeach
```

In this example the ```test_log``` test executable is compiled with the same
arguments as all other tests but additional test targets are generated with
different arguments. You can use a similar ```if``` condition inside the
respective loops to specify different behaviour for either compilation or
test generation.
