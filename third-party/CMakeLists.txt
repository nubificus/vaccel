# Force -fPIC to static third-party libraries
add_compile_options(-fPIC)

# Simple logger library
add_subdirectory(slog)
