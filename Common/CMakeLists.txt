# find zlib library
find_package(ZLIB)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        set(CMAKE_OSX_ARCHITECTURES arm64;x86_64)
endif()

# add source files to library
add_library(CommonLibrariesCommon ConcurrentCommunicationEndpoint.cpp CRC32.cpp IniFile.cpp
        IniIterator.cpp IniParser.cpp misc.cpp Serial.cpp cserial.c SimpleSockets.cpp
        StringHelpers.cpp utf8.cpp Threading.cpp timing.cpp ZSocket.cpp)

# interface library for targets
target_include_directories(CommonLibrariesCommon INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(CommonLibrariesCommon ZLIB::ZLIB)