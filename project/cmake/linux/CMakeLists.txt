cmake_minimum_required(VERSION 3.13)

if(NOT DEFINED ENV{VSF_PATH})
    get_filename_component(__VSF_PATH ../../../vsf/ ABSOLUTE)
    set(ENV{VSF_PATH} ${__VSF_PATH})
endif()
message(STATUS "VSF_PATH: $ENV{VSF_PATH}")

# board and application settings
set(BOARD_PATH ../../../board/linux)
set(APPLICATION_PATH ../../../application/kernel/helloworld)

project(vsf_template)
set(VSF_TARGET generic/linux64)

# compiler configurations
# if compiler is not set, cmake will find one

# VSF_USE configurations

# include vsf cmake script
set(VSF_CONFIG_PATH ${APPLICATION_PATH})
list(APPEND VSF_TARGET_INCLUDE_DIRECTORIES
    ${BOARD_PATH}
)
include($ENV{VSF_PATH}/script/cmake/vsf.cmake)

# application configurations
target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC
    __VSF__
    __OOC_CPP__
)

# add user application source code/include directories
add_subdirectory(${BOARD_PATH} ${CMAKE_CURRENT_BINARY_DIR}/board_bin)
add_subdirectory(${APPLICATION_PATH} ${CMAKE_CURRENT_BINARY_DIR}/application_bin)
