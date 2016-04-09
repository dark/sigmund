# the current src/build directories are the root of the source code
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(lib)

# C++ wrapper for versioning
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/version.cc.in" "${CMAKE_CURRENT_BINARY_DIR}/version.cc" @ONLY)
add_library(version "${CMAKE_CURRENT_BINARY_DIR}/version.cc")
# Bash wrapper for versioning
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/version.sh.in" "${CMAKE_CURRENT_BINARY_DIR}/version.sh" @ONLY)

add_executable(sigmund sigmund.cc)
target_link_libraries(sigmund db_ifc es_ifc dispatcher udp_srv version config)
