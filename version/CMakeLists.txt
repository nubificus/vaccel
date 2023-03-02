include_directories(${CMAKE_BINARY_DIR})
find_package(Git)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/version/git_version.h
  COMMAND ${CMAKE_COMMAND} -D SRC=${CMAKE_SOURCE_DIR}/version/git_version.h.in
                   -D DST=${CMAKE_BINARY_DIR}/version/git_version.h
                   -D GIT_EXECUTABLE=${GIT_EXECUTABLE}
                   -P ${CMAKE_SOURCE_DIR}/version/generateGitVersionHeader.cmake
  DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/git_version.h.in
  COMMENT "Configuring git_version"
  VERBATIM)

add_custom_target(version ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/version/git_version.h)

