cmake_policy(SET CMP0079 NEW)

# Find Protobuf installation
# Looks for protobuf-config.cmake file installed by Protobuf's cmake installation.
set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${protobuf_VERSION}")

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)

# Find gRPC installation
# Looks for gRPCConfig.cmake file installed by gRPC's cmake installation.
find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")

set(_GRPC_GRPCPP_UNSECURE gRPC::grpc++_unsecure)
find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)

# Proto file
get_filename_component(isound_proto "source.proto" ABSOLUTE)
get_filename_component(isound_proto_path "${isound_proto}" PATH)

# Generated sources
set(isound_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/source.pb.cc")
set(isound_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/source.pb.h")
set(isound_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/source.grpc.pb.cc")
set(isound_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/source.grpc.pb.h")
add_custom_command(
      OUTPUT "${isound_proto_srcs}" "${isound_proto_hdrs}" "${isound_grpc_srcs}" "${isound_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${isound_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${isound_proto}"
      DEPENDS "${isound_proto}")


# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

add_library(grpcsoundconnector gRPCConnector.cpp gRPCConnector.h ${isound_proto_srcs} ${isound_grpc_srcs})
target_link_libraries(grpcsoundconnector ${_GRPC_GRPCPP_UNSECURE} ${_PROTOBUF_LIBPROTOBUF})
target_link_libraries(lib${APP_NAME_LC} PUBLIC grpcsoundconnector)
