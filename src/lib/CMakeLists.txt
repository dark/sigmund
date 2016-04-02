# compile protobuf definition for data definition
PROTOBUF_GENERATE_CPP(FREUD_DATA_CC FREUD_DATA_H freud-data.proto freud_pb_src)
add_library(freud_pb ${FREUD_DATA_CC})

# UDP server
add_library(udp_srv threaded_udp_srv.cc)
target_link_libraries(udp_srv pthread)

# DB interface
add_library(db_ifc db_interface.cc)
target_link_libraries(db_ifc sqlite3)

# dispatcher
add_library(dispatcher dispatcher.cc)
