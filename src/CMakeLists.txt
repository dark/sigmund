# the current directory is the root of the source code
include_directories(.)

add_subdirectory(lib)

add_executable(sigmund sigmund.cc)
target_link_libraries(sigmund udp_srv db_ifc)
