cmake_minimum_required( VERSION 2.8.3 )

project(taco)

include_directories("${ROOT_DIR}/include")

add_definitions(-g)
add_definitions(-Werror)
add_definitions(-fms-extensions)
# add_definitions(-pthread)
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-pthread")

aux_source_directory( ${CMAKE_CURRENT_LIST_DIR} _src_files )

taco_get_obj_files( ${CMAKE_CURRENT_LIST_DIR} _obj_files )

add_library( ${PROJECT_NAME} SHARED ${_src_files} ${_obj_files} )

set_target_properties( ${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE C )

target_link_libraries( ${PROJECT_NAME} rt )
target_link_libraries( ${PROJECT_NAME} crypto )
target_link_libraries( ${PROJECT_NAME} ssl )

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${LIB_INSTALL_DIR})
