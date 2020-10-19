# Core runtime sources
set(runtime_src backend.c vaccel.c session.c)

# core runtime headers
set(headers backend.h vaccel.h vaccel_ops.h)

add_library(vaccel SHARED ${headers} ${runtime_src})
target_compile_options(vaccel PUBLIC -Wall -Wextra -Werror)

add_subdirectory(plugins)
