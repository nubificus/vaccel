# Core runtime sources
set(runtime_src backend.c vaccel.c vaccel_ops.c  session.c)

# core runtime headers
set(headers backend.h vaccel.h vaccel_ops.h)

add_library(vaccel SHARED ${headers} ${runtime_src})
target_compile_options(vaccel PUBLIC -Wall -Wextra -Werror)

add_subdirectory(plugins)

# Setup make install
install(TARGETS vaccel DESTINATION "${lib_path}")
install(FILES vaccel.h vaccel_ops.h DESTINATION "${include_path}")
