set(CMAKE_OSX_ARCHITECTURES arm64;x86_64)

# add source files to library
add_library(JSONRPC2 JSONRPC2Client.cpp JSONRPC2Server.cpp JSONParser.cpp)

# add top-level directory for IRPC header
include_directories(../ ../../Common)

# interface library for targets
target_include_directories(JSONRPC2 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

