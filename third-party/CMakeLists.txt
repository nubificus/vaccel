# Force -fPIC to static third-party libraries
add_compile_options(-fPIC)

# Simple logger library
add_subdirectory(slog)

# Install slog header files
install(
	FILES
	${CMAKE_SOURCE_DIR}/third-party/slog/src/slog.h
	DESTINATION
	${CMAKE_INSTALL_INCLUDEDIR}
)
