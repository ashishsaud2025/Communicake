cmake_minimum_required(VERSION 3.16)

project(communicake VERSION 5.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

find_package(OpenCV 4 REQUIRED COMPONENTS core imgproc imgcodecs videoio highgui)

add_executable(communicake
    src/main.cpp
    src/hand_tracker.cpp
    src/utilities.cpp
)

target_include_directories(communicake PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(communicake PRIVATE ${OpenCV_LIBS})
