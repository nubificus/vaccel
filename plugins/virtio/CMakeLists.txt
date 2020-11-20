set(VIRTIO_ACCEL_ROOT "/usr/local/include" CACHE STRING "Path to the virtio driver headers")
message("Using VIRTIO_ACCEL_ROOT ${VIRTIO_ACCEL_ROOT}")

set(include_dir ${CMAKE_SOURCE_DIR}/src)
set(SOURCES
	vaccel.c
	operations.c
	session.c
	ioctl.c
)

add_library(vaccel-virtio SHARED ${SOURCES})
target_include_directories(vaccel-virtio PRIVATE ${include_dir} ${VIRTIO_ACCEL_ROOT})

# Setup make install
install(TARGETS vaccel-virtio DESTINATION "${lib_path}")
