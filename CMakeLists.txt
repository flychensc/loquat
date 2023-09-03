cmake_minimum_required(VERSION 3.22.1)

project(loquat)

include_directories(${PROJECT_SOURCE_DIR}/include/)

file(GLOB_RECURSE libs CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/*.cc)
add_library(loquat STATIC ${libs})

file(GLOB_RECURSE mains CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/examples/*.cc)
foreach(mainfile IN LISTS mains)

    get_filename_component(mainname ${mainfile} NAME_WE)
    add_executable(${mainname} ${mainfile})

    target_link_libraries(${mainname} loquat)
endforeach(mainfile IN LISTS mains)
