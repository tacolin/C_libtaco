cmake_minimum_required( VERSION 2.8.3 )

project(test_ringbuf)

taco_get_header_dirs(${CMAKE_CURRENT_LIST_DIR} _hdr_dirs)
include_directories(${_hdr_dirs})

include_directories("${ROOT_DIR}/include")

add_definitions(-g)
add_definitions(-Werror)
# add_definitions(-pthread)
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-pthread")

taco_get_src_dirs(${CMAKE_CURRENT_LIST_DIR} _src_dirs)

foreach(_dir ${_src_dirs})
    aux_source_directory( ${_dir} _src_files )
    taco_get_obj_files(${_dir} objs )
    set(_obj_files ${objs} ${_obj_files})
endforeach()

add_executable( ${PROJECT_NAME} ${_src_files} ${_obj_files})
set_target_properties( ${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE C )

add_dependencies( ${PROJECT_NAME} taco )

target_link_libraries( ${PROJECT_NAME} taco )

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "${APP_INSTALL_DIR}")

add_custom_target("run-${PROJECT_NAME}"
                  DEPENDS ${PROJECT_NAME} taco)

add_custom_command(TARGET "run-${PROJECT_NAME}"
                   COMMAND valgrind ./${PROJECT_NAME}
                   WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}/${APP_INSTALL_DIR}"
                   COMMENT "[TACO] Run ${PROJECT_NAME}")

