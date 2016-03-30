# compile protobuf definition for data definition
PROTOBUF_GENERATE_CPP(FREUD_DATA_CC FREUD_DATA_H freud-data.proto freud_data_pb)

# UDP server
add_library(udp_srv threaded_udp_srv.cc)
target_link_libraries(udp_srv pthread)
