# vaccel Runtime System

vaccel is a runtime system for hardware acceleration. It provides an API with a set of functions that the runtime is able to offload to hardware acceleration
devices. The design of the runtime is modular, it consists of a front-end library which exposes the API to the user application and a set of back-end plugins
that are responsible to offload computations to the accelerator.

This design decouples the user application from the actual accelerator specific code. The advantage of this choice is that the application can make use of
different hardware accelerators without extra development cost or re-compiling.

This repo includes the core runtime system, and back-end plugins for VirtIO and the Jetson Inference framework.

## VirtIO backend
TODO: put link to VirtIO README.md

## Jetson Inference backend
TODO: put link to Jetson README.md

## Build & Installation

### 1. Cloning and preparing the build directory

```zsh
~ » git clone https://github.com/cloudkernels/vaccelrt.git

Cloning into 'vaccelrt'...
remote: Enumerating objects: 215, done.
remote: Counting objects: 100% (215/215), done.
remote: Compressing objects: 100% (130/130), done.
remote: Total 215 (delta 115), reused 173 (delta 82), pack-reused 0
Receiving objects: 100% (215/215), 101.37 KiB | 804.00 KiB/s, done.
Resolving deltas: 100% (115/115), done.
~ » cd vaccelrt
~/vaccelrt(master) » mkdir build
~/vaccelrt(master) » cd build
~/vaccelrt/build(master) »                                                                                   
```

### 2. Building the core runtime system
```zsh
# This sets the installation path to ${HOME}/.local
~/vaccelrt/build(master) » cmake -DCMAKE_INSTALL_PREFIX=~/.local ..
~/vaccelrt/build(master) » mak
~/vaccelrt/build(master) » mak install
```

### 3. Building the plugins

Building the plugins is disabled, by default. You can enable building one or
more plugins at configuration time of CMake by setting the corresponding
variable of the following table to `ON`

Backend Plugin | Variable | Default
-------------- | -------- | -------
virtio | BUILD\_PLUGIN\_VIRTIO | `OFF`
jetson | BUILD\_PLUGIN\_JETSON | `OFF`

For example:

```sh
cmake -DBUILD_PLUGIN_VIRTIO=ON ..
```

will enable building the virtio backend plugin.

#### VirtIO plugin building options

Variable | Values | Default
-------- | ------ | -------
VIRTIO\_ACCEL\_ROOT | Path to virtio module installation | `/usr/local/include`


The VirtIO plugin uses the [virtio-accel](https://github.com/cloudkernels/virtio-accel)
kernel module to offload requests to the host. When building we need to point
CMake to the location of virtio-accel installation prefix using the
`VIRTIO_ACCEL_ROOT` variable.

#### Jetson plugin building options

The jetson inference backends depends on the Jetson inference framework, a
corresponding CUDA installation and the STB library.

Variable | Values | Default
-------- | ------ | -------
CUDA\_DIR | Path to CUDA installation | `/usr/local/cuda/targets/x86_64-linux`
JETSON\_DIR | Path to Jetson installation | `/usr/local`
STB\_DIR | Path to STB installation | `/usr/local`

## Building a vaccel application

We will use an example of image classification which can be found under the
[examples](https://github.com/cloudkernels/vaccelrt/tree/master/examples) folder of this project.

You can build the example using the CMake of our project:
```
$ mkdir build
$ cd build
$ cmake -DBUILD_EXAMPLES=ON ..
$ make
$ ls examples
classify  CMakeFiles  cmake_install.cmake  Makefile
```

If, instead, you want to build by hand you need to define the include and library paths (if they are not
in your respective default search paths) and also link with `dl`:

```
$ cd ../examples
$ gcc -Wall -Wextra -I${HOME}/.local/include -L${HOME}/.local/lib classification.c -o classify -lvaccel -ldl
$ ls
classification.c  classify  CMakeLists.txt  images
```

## Running a vaccel application

Having built our `classify` example, we need to prepare the vaccel environment for it to run:

1. Define the path to `libvaccel.so` (if not in the default search path):

```
export LD_LIBRARY_PATH=${HOME}/.local/lib
```

2. Define the backend plugin to use for our application.

In this example, we will use the jetson plugin which implements the image classification operation using the Jetson Inference
framework which uses TensorRT.

```
export VACCEL_BACKENDS=${HOME}/.local/lib/libvaccel-jetson.so
```

Finally, the classification application needs the imagent models in the current working path.
(TODO: Find link to download those). Once you have these, you can do:

```
$ ls 
classify  images  networks
$ VACCEL_IMAGENET_NETWORKS=$(pwd) ./classify images/banana_0.jpg 1
Initialized session with id: 1
Image size: 79281B
classification tags: 99.902% banana
```
