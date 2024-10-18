cmake_minimum_required(VERSION 3.10)

find_package(GTest REQUIRED)

set(test_cmds)
file(GLOB_RECURSE test_files CONFIGURE_DEPENDS unit/*.cc)
foreach(test_file IN LISTS test_files)
    get_filename_component(file_name ${test_file} NAME_WE)

    add_executable(${file_name} ${test_file})
    target_link_libraries(${file_name} loquat GTest::GTest GTest::Main)

    list(APPEND test_cmds ${file_name})
endforeach()
add_test(NAME unittest COMMAND ${test_cmds})

set(test_cmds)
file(GLOB_RECURSE test_files CONFIGURE_DEPENDS integration/*.cc)
foreach(test_file IN LISTS test_files)
    get_filename_component(file_name ${test_file} NAME_WE)

    add_executable(${file_name} ${test_file})
    target_link_libraries(${file_name} loquat GTest::GTest GTest::Main)

    list(APPEND test_cmds ${file_name})
endforeach()
add_test(NAME integration COMMAND ${test_cmds})
