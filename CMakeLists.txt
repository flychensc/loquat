cmake_minimum_required(VERSION 3.22.1)

project(loquat)

option(SAMPLE "Enable the sample feature for demonstration purposes." ON)
option(DEBUG "Enable debug mode for detailed logging and error checking." OFF)
option(TEST "Enable building and running unit tests using Google Test." ON)

if (DEBUG)
    set(CMAKE_BUILD_TYPE Debug)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
endif()

if (TEST)
    enable_testing()
    add_subdirectory(tests)
endif()

find_package(spdlog REQUIRED)

file(GLOB_RECURSE libs CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/*.cc)
add_library(loquat STATIC ${libs})
target_include_directories(loquat PUBLIC ${PROJECT_SOURCE_DIR}/include/)
target_link_libraries(loquat PRIVATE spdlog::spdlog)

if (SAMPLE)
    file(GLOB_RECURSE mains CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/examples/*.cc)
    foreach(mainfile IN LISTS mains)
        get_filename_component(mainname ${mainfile} NAME_WE)

        add_executable(${mainname} ${mainfile})
        target_link_libraries(${mainname} loquat)
    endforeach(mainfile IN LISTS mains)
endif()
