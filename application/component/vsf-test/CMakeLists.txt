# Generate test parameters header from YAML (run manually or via CMake)
set(TEST_PARAMS_YML ${CMAKE_CURRENT_SOURCE_DIR}/test_params.yml)
set(TEST_PARAMS_H   ${CMAKE_CURRENT_SOURCE_DIR}/test_params_generated.h)
set(VSF_TEST_GLOBAL_PARAMS_DIR $ENV{VSF_PATH}/test/vsf_test/params)

find_program(PYTHON3_CMD python3 REQUIRED)
set(Python3_EXECUTABLE ${PYTHON3_CMD})

# Discover YAML files automatically.  Adding/removing a .yml requires
# re-running CMake (standard for test-params generation anyway).
# Local YAMLs override global YAMLs with the same name.
file(GLOB PERIPH_YMLS   ${CMAKE_CURRENT_SOURCE_DIR}/*.yml)
file(GLOB SCENARIO_YMLS ${CMAKE_CURRENT_SOURCE_DIR}/scenario/*.yml)
file(GLOB GLOBAL_PERIPH_YMLS   ${VSF_TEST_GLOBAL_PARAMS_DIR}/*.yml)
file(GLOB GLOBAL_SCENARIO_YMLS ${VSF_TEST_GLOBAL_PARAMS_DIR}/scenario/*.yml)

add_custom_command(
    OUTPUT ${TEST_PARAMS_H}
    COMMAND ${CMAKE_COMMAND} -E env VSF_TEST_GLOBAL_PARAMS_DIR=${VSF_TEST_GLOBAL_PARAMS_DIR}
        PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/../../../vsf/test
        ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/../../../vsf/test/vsf_bench/gen_test_params.py
        ${TEST_PARAMS_YML} ${TEST_PARAMS_H}
    DEPENDS
        ${TEST_PARAMS_YML}
        ${PERIPH_YMLS}
        ${SCENARIO_YMLS}
        ${GLOBAL_PERIPH_YMLS}
        ${GLOBAL_SCENARIO_YMLS}
    COMMENT "Generating test parameters header"
)

add_custom_target(vsf_test_gen_params DEPENDS ${TEST_PARAMS_H})

vsf_add_include_directories(
    ./
    $ENV{VSF_PATH}
)

# Application entry point only — test framework + suites are pulled in
# automatically via the vsf_test CMake library below.
vsf_add_sources(main.c)

# Pull in the VSF test framework library (framework + all peripheral suites).
add_subdirectory($ENV{VSF_PATH}/test/vsf_test ${CMAKE_CURRENT_BINARY_DIR}/vsf_test_bin)

# Ensure header is generated before the firmware target builds
add_dependencies(${CMAKE_PROJECT_NAME} vsf_test_gen_params)

# Kernel profile selection. Pass via cmake -DVSF_KERNEL_PROFILE=VSF_TEST_KERNEL_PROFILE_<NAME>.
# Defaults to baremetal when undefined (see vsf_usr_cfg.h).
if(DEFINED VSF_KERNEL_PROFILE)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC ${VSF_KERNEL_PROFILE})
endif()
