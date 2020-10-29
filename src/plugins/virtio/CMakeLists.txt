if (NOT DEFINED VIRTIO_ACCEL_INCLUDE)
	set(VIRTIO_ACCEL_INCLUDE /usr/local/include)
endif()
message("Using VIRTIO_ACCEL_INCLUDE ${VIRTIO_ACCEL_INCLUDE}")

set(include_dir ${CMAKE_SOURCE_DIR}/src)
set(SOURCES
	vaccel.c
	operations.c
	session.c
	ioctl.c
)

add_library(vaccel-virtio SHARED ${SOURCES})
target_include_directories(vaccel-virtio PRIVATE ${include_dir} ${VIRTIO_ACCEL_INCLUDE})

# Setup make install
install(TARGETS vaccel-virtio DESTINATION "${lib_path}")
