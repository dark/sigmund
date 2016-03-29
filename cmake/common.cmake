#### compiler ####
SET(CMAKE_C_COMPILER /usr/bin/clang)
SET(CMAKE_CXX_COMPILER /usr/bin/clang++)

#### C++ flags ####
set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wextra -Wsign-compare -Wtype-limits -O2 -g -pipe")

#### protobuf ####
find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIRS})

# Run c++ protocol buffer compiler on a proto file, generating source
# and header files, plus a target name you can depend on.
function(PROTOBUF_GENERATE_CPP SRC HDR PROTO_FILE TARGET_NAME)
  set(${SRC})
  set(${HDR})

  get_filename_component(FILE_ABSOLUTE ${PROTO_FILE} ABSOLUTE)
  get_filename_component(FILE_PATH ${FILE_ABSOLUTE} PATH)
  get_filename_component(FILE_NOEXTENSION ${PROTO_FILE} NAME_WE)

  list(APPEND ${SRC} "${CMAKE_CURRENT_BINARY_DIR}/${FILE_NOEXTENSION}.pb.cc")
  list(APPEND ${HDR} "${CMAKE_CURRENT_BINARY_DIR}/${FILE_NOEXTENSION}.pb.h")
  add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${FILE_NOEXTENSION}.pb.cc"
    "${CMAKE_CURRENT_BINARY_DIR}/${FILE_NOEXTENSION}.pb.h"
    COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
    ARGS --cpp_out  ${CMAKE_CURRENT_BINARY_DIR} "${FILE_NOEXTENSION}.proto"
    DEPENDS ${FILE_ABSOLUTE} ${FILE_NOEXTENSION}.proto
    WORKING_DIRECTORY ${FILE_PATH}
    COMMENT "Running C++ protoc on ${FILE_NOEXTENSION}.proto"
    VERBATIM
    )

  # create a dependable target
  add_custom_target(${TARGET_NAME} DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${FILE_NOEXTENSION}.pb.cc")

  set_source_files_properties(${${SRC}} ${${HDR}} PROPERTIES GENERATED TRUE)
  set(${SRC} ${${SRC}} PARENT_SCOPE)
  set(${HDR} ${${HDR}} PARENT_SCOPE)
endfunction()
