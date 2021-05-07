# include the CMake modules for Rust
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

# Enable Rust support
enable_language(Rust)
include(CMakeCargo)

add_subdirectory(rust-vaccel)
