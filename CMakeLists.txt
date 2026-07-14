cmake_minimum_required(VERSION 3.16)

project(communicake VERSION 5.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

find_package(OpenCV 4 REQUIRED COMPONENTS core imgproc imgcodecs videoio highgui)

# Vision, gesture, configuration and action logic. Deliberately free of windows,
# cameras and platform shell calls so the tests can drive it directly.
add_library(communicake_core STATIC
    src/geometry.cpp
    src/vision.cpp
    src/gesture.cpp
    src/json.cpp
    src/config.cpp
    src/action.cpp
    src/cli.cpp
    src/frame_source.cpp
    src/profile.cpp
    src/observation_log.cpp
    src/timing.cpp
)

target_include_directories(communicake_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(communicake_core PUBLIC ${OpenCV_LIBS})

add_library(communicake_app STATIC
    src/app.cpp
    src/ui.cpp
)

target_link_libraries(communicake_app PUBLIC communicake_core)

add_executable(communicake src/main.cpp)
target_link_libraries(communicake PRIVATE communicake_app)

enable_testing()
add_subdirectory(tests)