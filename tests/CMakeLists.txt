set(COMMUNICAKE_TESTS
    test_geometry.cpp
    test_fingertips.cpp
    test_stabilizer.cpp
)

foreach(test_source ${COMMUNICAKE_TESTS})
    get_filename_component(test_name ${test_source} NAME_WE)
    add_executable(${test_name} ${test_source})
    target_link_libraries(${test_name} PRIVATE communicake_core)
    target_include_directories(${test_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
