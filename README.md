# vAccelRT

[![Build and upload](https://github.com/cloudkernels/vaccelrt/actions/workflows/main-build-and-upload.yml/badge.svg)](https://github.com/cloudkernels/vaccelrt/actions/workflows/main-build-and-upload.yml)

vAccelRT is a runtime library for hardware acceleration. vAccelRT provides an
API with a set of functions that the library is able to offload to hardware
acceleration devices. The design of the runtime library is modular, it consists
of a front-end library which exposes the API to the user application and a set
of backend plugins that are responsible to offload computations to the
accelerator.

This design decouples the user application from the actual accelerator specific
code. The advantage of this choice is that the application can make use of
different hardware accelerators without extra development cost or re-compiling.

This repo includes the core runtime library, and a backend plugin for the
`EXEC` operation. For debugging and demonstration purposes we include a `NOOP`
plugin which just prints out debug parameters (input and output) for each API
call. You can also find a `MBENCH` micro-benchmark plugin that emulates a
cpu-intensive workload to measure vAccel overhead.

There is a [splash page](https://vaccel.org) for vAccel, along with more
[elaborate documentation](https://docs.vaccel.org).

For step-by-step tutorials, you can have a look at our
[lab](https://github.com/nubificus/vaccel-tutorials) repo.


## Build & Installation

### 1. Install dependencies and clone repo

```bash
apt-get install build-essential ninja-build pkg-config python3-pip 
pip install meson
git clone https://github.com/cloudkernels/vaccelrt --recursive
```

### 2. Configure, build and install the core runtime library
```bash
cd vaccelrt

# Configure the build directory with the default options and set build
# type to 'release'.
meson setup --buildtype=release build

# Compile the project
meson compile -C build

# Install the project
meson install -C build
```

### 3. Build the plugins

Building the plugins is disabled by default. You can enable building one
or more plugins at configuration time by setting the corresponding
options.

For example, replacing:

```bash
meson setup --buildtype=release build
```

with:

```bash
meson setup --buildtype=release -Dplugin-noop=enabled build
```

in the previous [code snippet](#2-configure-build-and-install-the-core-runtime-library),
will build and install both the core library and the noop backend
plugin.

To view all available options and values, you can use:

```bash
meson setup --buildtype=release build
meson configure build
```

vAccelRT specific options can be found in the `Project Options`
section.

You can find more meson command examples in
[Configuring and building vAccel with meson](docs/meson_build.md).

## Building a vAccel application

We will use an image classification example which can be found under the
[examples](examples)
folder of this project.

To enable build of this and all other examples, enable the `examples`
option at configuration time:
```bash
meson setup --reconfigure -Dexamples=enabled build
```

Alternatively, to build an application manually you can use the
provided pkg-config specification - make sure vAccel is installed
globally or set the `PKG_CONFIG_PATH` environment variable.

For example:

```bash
cd ../examples
gcc classify.c -o classify -Wall -Wextra $(pkg-config --cflags --libs vaccel)
```

## Running a vAccel application

Having built our `classify` example, we need to prepare the vAccel
environment for it to run:

1. Define the path to `libvaccel.so` (if not in the default search path):

   ```bash
   export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
   ```

2. Define the backend plugin to use for our application.

   In this example, we will use the noop plugin:

   ```bash
   export VACCEL_BACKENDS=/usr/local/lib/libvaccel-noop.so
   ```

3. Finally, run the application:

   ```bash
   ./classify /usr/local/share/vaccel/images/example.jpg 1
   ```

   Or, alternatively, from the build directory:

   ```bash
   ./build/examples/classify examples/images/example.jpg 1
   ```

   which should dump the following output:

   ```bash
   Initialized session with id: 1
   Image size: 79281B
   [noop] Calling Image classification for session 1
   [noop] Dumping arguments for Image classification:
   [noop] len_img: 79281
   [noop] will return a dummy result
   classification tags: This is a dummy classification tag!
   ```

You can get verbose output from the vAcceRT library by setting the
`VACCEL_DEBUG_LEVEL` environment variable.

For example, to use debug level logging:
```bash
export VACCEL_DEBUG_LEVEL=4
   ```

## License

[Apache License 2.0](LICENSE)
