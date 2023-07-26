cmake_minimum_required(VERSION 3.5)

if(WIN32)
    set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE INTERNAL "" FORCE)
endif()

project(WebRTCStreamer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ON CACHE INTERNAL "" FORCE)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(LibDataChannel CONFIG REQUIRED)

add_executable(${PROJECT_NAME}
    src/client.cpp src/client.h
    src/client_manager.cpp src/client_manager.h
    src/dispatch_queue.cpp src/dispatch_queue.h
    src/stream.cpp src/stream.h
    src/file_parser.cpp src/file_parser.h
    src/h264_file_parser.cpp src/h264_file_parser.h
    src/opus_file_parser.cpp src/opus_file_parser.h
    src/main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE LibDataChannel::LibDataChannel)
