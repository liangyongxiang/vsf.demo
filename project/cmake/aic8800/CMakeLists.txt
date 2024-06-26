cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0076 NEW)

#if(NOT DEFINED ENV{VSF_PATH})
    get_filename_component(__VSF_PATH ../../../vsf ABSOLUTE)
    set(ENV{VSF_PATH} ${__VSF_PATH})
#endif()
message(STATUS "VSF_PATH: $ENV{VSF_PATH}")

# board and application settings
set(BOARD_PATH ../../../board/aic8800m_uikit)
set(APPLICATION_PATH ../../../application/kernel/helloworld)

# compiler configurations
# arm-none-eabi-gcc will not pass compiler checker of cmake, so include before project
set(CMAKE_C_FLAGS 
    "-Ofast"
    CACHE INTERNAL "C compiler common flags"
)
set(CMAKE_CXX_FLAGS 
    "-Ofast"
    CACHE INTERNAL "C++ compiler common flags"
)
include($ENV{VSF_PATH}/script/cmake/compilers/gnuarmemb.cmake)

project(vsf_demo C CXX ASM)
set(VSF_TARGET AIC8800M)
set(VSF_TARGET_DEFINITIONS
#   hw related definitions for AIC8800M required by sdk
#   hw_platform: pxp(0), fpga(1), asic(2)
    "CFG_HW_PLATFORM=2"
#   rf_mode: null(0), bt_only(1), bt_combo(2), btwifi_combo(3)
    "CFG_RF_MODE=0"
#   rom_ver
    "CFG_ROM_VER=255"
#   dac: left(0), mute(1), right(2), lprd2(3), lmrd2(4)
    "CFG_DACL_MIXER_MODE=1"
    "CFG_DACR_MIXER_MODE=1"
#   pmic
    "CFG_PMIC"
    "CFG_PMIC_VER_LITE"
    "CFG_PMIC_ITF_ASDMA"
#   wifi
    "CFG_WIFI_STACK"
    "CFG_RSSI_DATAPKT"
    "CONFIG_RWNX"
    "CONFIG_RWNX_LWIP"
#   wifi 5G
#    "CFG_BAND5G"
#   misc
    "CFG_DBG"
    "CFG_IPERF"
#   mic matrix
    "CFG_AIC1000_MIC_MATRIX=0"
)

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

# linker script
target_link_options(${CMAKE_PROJECT_NAME} PRIVATE
    LINKER:-T${CMAKE_CURRENT_SOURCE_DIR}/linker.ld
    LINKER:-Map=output.map
)

# add user application source code/include directories
add_subdirectory(${BOARD_PATH} ${CMAKE_CURRENT_BINARY_DIR}/board_bin)
add_subdirectory(${APPLICATION_PATH} ${CMAKE_CURRENT_BINARY_DIR}/application_bin)

# type conversion and show size
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
    SUFFIX ".elf"
)
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex ${CMAKE_PROJECT_NAME}.elf ${CMAKE_PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary ${CMAKE_PROJECT_NAME}.elf ${CMAKE_PROJECT_NAME}.bin
)
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    USES_TERMINAL
)