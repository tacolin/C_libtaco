#############################################################################
# 檢查 32 bit 或 64 bit
#############################################################################
function(taco_is_32bit_system output)
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(${output} TRUE PARENT_SCOPE)
    else()
        set(${output} FALSE PARENT_SCOPE)
    endif()
endfunction()

#############################################################################
# function name: taco_get_subdirs
# arguments:
#   input: current directories
#   output: list of sub directories
# description: 找出input下的所有子目錄 (但排除隱藏目錄)
#############################################################################
function(taco_get_subdirs input output )
    set(_members "")
    set(_tmp_list "")
    file(GLOB _members "${input}/[^.]*")
    foreach(_mem ${_members})
        if (IS_DIRECTORY ${_mem})
            list(APPEND _tmp_list ${_mem})
        endif()
    endforeach()
    set(${output} ${_tmp_list} PARENT_SCOPE)
endfunction()


#############################################################################
# function name: taco_get_subdirs
# arguments:
#   input: current directories
#   output: list of directories in which the header file exists
# description: 找出input下的所有含有header file的目錄
#############################################################################
function(taco_get_header_dirs input output)
    set(_tmp_file_list "")
    set(_tmp_dir_list "")
    set(_tmp_dir "")

    # file(GLOB_RECURSE _tmp_file_list "*.h" "*.H" "*.hpp" "*.HPP")
    file(GLOB_RECURSE _tmp_file_list "${input}/*.h" "${input}/*.H" "${input}/*.hpp" "${input}/*.HPP")

    foreach(_tmp_file ${_tmp_file_list})
        get_filename_component(_tmp_dir ${_tmp_file} PATH)
        list(APPEND _tmp_dir_list ${_tmp_dir})
    endforeach()

    list(REMOVE_DUPLICATES _tmp_dir_list)
    set(${output} ${_tmp_dir_list} PARENT_SCOPE)
endfunction()

#############################################################################
# function name: taco_get_src_dirs
# arguments:
#   input: current directories
#   output: list of directories in which the source file exists
# description: 找出input下的所有含有source file的目錄
#############################################################################
function(taco_get_src_dirs input output)
    set(_tmp_file_list "")
    set(_tmp_dir_list "")
    set(_tmp_dir "")

    file(GLOB_RECURSE _tmp_file_list "${input}/*.c" "${input}/*.C" "${input}/*.cpp" "${input}/*.CPP")

    foreach(_tmp_file ${_tmp_file_list})
        get_filename_component(_tmp_dir ${_tmp_file} PATH)
        list(APPEND _tmp_dir_list ${_tmp_dir})
    endforeach()

    list(REMOVE_DUPLICATES _tmp_dir_list)
    set(${output} ${_tmp_dir_list} PARENT_SCOPE)
endfunction()

#############################################################################
# function name: taco_get_obj_files
# arguments:
#   input: current directories
#   output: list of object files in the input directory
# description: 找出input下的所有 object file (.o檔)
#############################################################################
function(taco_get_obj_files input output)
    set(_tmp_file_list "")

    file(GLOB_RECURSE _tmp_file_list "${input}/*.o" "${input}/*.O")

    list(REMOVE_DUPLICATES _tmp_file_list)
    set(${output} ${_tmp_file_list} PARENT_SCOPE)
endfunction()

