# Configuring and building vAccel with meson
## Configuration
To configure vAccel with the default options:
```python
meson setup <build_dir>
```

To view the available options and their values:
```python
meson configure <build_dir>
```

Setting a project option can be then done with:
```python
meson setup --reconfigure -D<option>=<value> <build_dir>
```
or
```python
meson configure -D<option>=<value> <build_dir>
```

Setting other useful configuration options:
```python
# set subproject build option
meson setup --reconfigure -D<subproject>:<option>=<value> <build_dir>

# install to custom path
meson setup --reconfigure --prefix=<absolute path> <build_dir>

# use installation from non-standard path
meson setup --reconfigure --pkg-config-path=<prefix>/lib/<x86_64-linux-gnu>/pkgconfig <build_dir>

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
With a configured build directory vAccel can be built with:
```python
meson compile -C <build_dir>
```

## Running the tests
Running vAccel unit tests with meson is done with:
```python
meson test -C <build_dir>
```

Some commands for more specific use cases:
```python
# w/ specific test 
meson test <test_name> -C <build_dir>

# w/ valgrind
meson test --wrap='valgrind' -C <build_dir>

# w/ coverage (will probably need to clear the build dir if already configured)
meson setup -Db_coverage=true <build_dir>
meson test -C <build_dir>
ninja coverage-text -C <build_dir>
```

## Installation / Generating distribution packages
Finally, to install vAccel:
```python
meson install -C <build_dir>
```

Alternatively, to create a distribution tarball and generate debian packages:
```python
# Note: uncommitted changes won't be included
meson dist --include-subprojects --allow-dirty -C <build_dir>
```
