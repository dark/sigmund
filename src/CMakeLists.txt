# the current src/build directories are the root of the source code
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(lib)

add_executable(sigmund sigmund.cc)
target_link_libraries(sigmund db_ifc es_ifc dispatcher udp_srv)
