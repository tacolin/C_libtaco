cmake_minimum_required( VERSION 2.8.3 )

taco_get_subdirs("./" _sub_directories)

subdirs(${_sub_directories})