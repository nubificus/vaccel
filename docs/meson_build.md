# Building and testing vAccel with meson
## Requirements
[Meson](https://mesonbuild.com/) is a python project depending on `ninja` and
`pkg-config`. To install meson and dependencies on a Debian-based distribution
run:
```bash
apt-get install build-essential ninja-build pkg-config python3-pip 
pip install meson
```

## Configuration
To configure vAccel with the default options:
```bash
meson setup <build_dir>
```

To then view the available options and their values:
```bash
meson configure <build_dir>
```

Setting a project option can be done with:
```bash
meson setup --reconfigure -D<option>=<value> <build_dir>
```
or
```bash
meson configure -D<option>=<value> <build_dir>
```

Setting other useful configuration options:
```bash
# set subproject build option
meson setup --reconfigure -D<subproject>:<option>=<value> <build_dir>

# install to custom path
meson setup --reconfigure --prefix=<absolute path> <build_dir>

# use installation from non-standard path
meson setup --reconfigure \
  --pkg-config-path=<prefix>/lib/<x86_64-linux-gnu>/pkgconfig <build_dir>

# set library type (dynamic | static | both)
meson setup --reconfigure -Ddefault_library=<type> <build_dir>

# generate static + dynamic libs and use static when linking
meson setup --reconfigure -Ddefault_library=both --prefer-static <build_dir>

# use custom build toolchain
meson setup --native-file musl.ini -Ddefault_library=static <build_dir>
> # musl.ini (need to add bin to PATH and libs to LD_LIBRARY_PATH)
>
> [constants]
> arch = 'x86_64-linux-musl'
>
> [binaries]
> c = arch + '-gcc'
> cpp = arch + '-g++'
> ar = arch + '-gcc-ar'

# wipe and reconfigure with old settings
meson setup --wipe <build_dir>
```

## Compilation
With a configured <build_dir> vAccel can be built with:
```bash
meson compile -C <build_dir>
```

All sections bellow assume that vAccel is compiled after the <build_dir> is
configured and before running any other meson commands.

## Running the tests
To run the tests make sure that the `tests` option is enabled for the
<build_dir>:
```bash
meson setup --reconfigure -Dtests=enabled <build_dir>
```

If generating a coverage report is desirable, a clean <build_dir> should be
created with at least:
```bash
meson setup -Dtests=enabled -Db_coverage=true <build_dir>
```
After following the printed suggestion for creating a `gcovr.cfg`, a coverage
report can be generated with:
```bash
meson test -C <build_dir>
ninja coverage-text -C <build_dir>
```

Running all vAccel unit tests with meson is done with:
```bash
meson test -C <build_dir>
```

To additionally use valgrind:
```bash
meson test --wrap='valgrind' -C <build_dir>
```

To run a specific test use:
```bash
meson test <test_name> -C <build_dir>
```

## Running the examples
To run all the examples, the `examples`, `plugin-noop` and `plugin-exec` options
must be enabled for the <build_dir>:
```bash
meson setup --reconfigure -Dexamples=enabled -Dplugin-noop=enabled \
  -Dplugin-exec=enabled <build_dir>
```

To run the examples included with vAccel use the `run-examples` target:
```bash
ninja run-examples -C <build_dir>
```

## Formatting code and running static analysis
vAccel includes configurations for `clang-format` and `clang-tidy` to format
and run static analysis on its' code.

If these tools are installed *before* configuring a meson <build_dir>, meson
will generate the relevant targets. We suggest that the official
[llvm](https://apt.llvm.org/) packages are used to get consistent results with
our CI.

Running the tools from a configured <build_dir> can de done with:
```bash
# format all files in place
ninja clang-format -C <build_dir>

# analyze code
ninja clang-tidy -C <build_dir>

# apply fixes where possible
ninja clang-tidy-fix -C <build_dir>
```

## Installation / Generating distribution packages
Finally, to install vAccel built in <build_dir>:
```bash
meson install -C <build_dir>
```

Alternatively, to create a distribution tarball and generate debian packages:
```bash
# Note: uncommitted changes won't be included
meson dist --include-subprojects --allow-dirty -C <build_dir>
```
