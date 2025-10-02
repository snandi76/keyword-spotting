cmake_minimum_required(VERSION 3.13.1)
project(KeywordSpottingTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set build type to Release for better performance
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Add compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

# Define paths
set(EDGE_IMPULSE_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../keyword-spotting-cpp-mcu-v5/edge-impulse-sdk")
set(MODEL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../keyword-spotting-cpp-mcu-v5")

# Source files - only include what we need
set(SOURCES
    main.cpp
    audio_utils.cpp
    ${MODEL_PATH}/tflite-model/tflite_learn_792318_5_compiled.cpp
)

# Create executable
add_executable(keyword_spotting_test ${SOURCES})

# Include directories - use absolute paths
target_include_directories(keyword_spotting_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${EDGE_IMPULSE_SDK_PATH}
    ${EDGE_IMPULSE_SDK_PATH}/classifier
    ${EDGE_IMPULSE_SDK_PATH}/dsp
    ${EDGE_IMPULSE_SDK_PATH}/tensorflow/lite/c
    ${EDGE_IMPULSE_SDK_PATH}/tensorflow/lite/kernels
    ${EDGE_IMPULSE_SDK_PATH}/tensorflow/lite/kernels/internal
    ${EDGE_IMPULSE_SDK_PATH}/tensorflow/lite/micro
    ${EDGE_IMPULSE_SDK_PATH}/tensorflow/lite/micro/kernels
    ${EDGE_IMPULSE_SDK_PATH}/tensorflow/lite/micro/memory_planner
    ${EDGE_IMPULSE_SDK_PATH}/tensorflow/lite/schema
    ${EDGE_IMPULSE_SDK_PATH}/third_party/flatbuffers/include
    ${EDGE_IMPULSE_SDK_PATH}/third_party/gemmlowp
    ${EDGE_IMPULSE_SDK_PATH}/third_party/ruy
    ${MODEL_PATH}/model-parameters
    ${MODEL_PATH}/tflite-model
)

# Add specific Edge Impulse SDK source files that we need
set(EDGE_IMPULSE_SOURCES
    ${EDGE_IMPULSE_SDK_PATH}/classifier/ei_run_classifier_c.cpp
    ${EDGE_IMPULSE_SDK_PATH}/porting/posix/debug_log.cpp
    ${EDGE_IMPULSE_SDK_PATH}/porting/posix/ei_classifier_porting.cpp
    ${EDGE_IMPULSE_SDK_PATH}/dsp/memory.cpp
    ${EDGE_IMPULSE_SDK_PATH}/dsp/kissfft/kiss_fft.cpp
    ${EDGE_IMPULSE_SDK_PATH}/dsp/kissfft/kiss_fftr.cpp
    ${EDGE_IMPULSE_SDK_PATH}/dsp/dct/fast-dct-fft.cpp
    ${EDGE_IMPULSE_SDK_PATH}/tensorflow/lite/c/common.c
)

target_sources(keyword_spotting_test PRIVATE ${EDGE_IMPULSE_SOURCES})

# Link libraries
target_link_libraries(keyword_spotting_test
    pthread
    m
)

# Compiler definitions
target_compile_definitions(keyword_spotting_test PRIVATE
    EI_CLASSIFIER_USE_FULL_TFLITE=1
    EI_CLASSIFIER_USE_TFLITE_MICRO=0
)

# Set properties
set_target_properties(keyword_spotting_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Print build information
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "Edge Impulse SDK path: ${EDGE_IMPULSE_SDK_PATH}")