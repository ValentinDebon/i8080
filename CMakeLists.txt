cmake_minimum_required(VERSION 3.14)

project(I8080 LANGUAGES C)

#########
# Build #
#########

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

include_directories(include)

file(GLOB_RECURSE I8080_SOURCES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/i8080/*.c)
add_executable(i8080 ${I8080_SOURCES})

file(GLOB_RECURSE LIBI8080_SOURCES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/libi8080/*.c)
add_library(libi8080 ${LIBI8080_SOURCES})

target_link_libraries(i8080 PUBLIC libi8080)

set_target_properties(libi8080 PROPERTIES
	OUTPUT_NAME i8080
	PUBLIC_HEADER include/i8080/cpu.h
)

########
# Test #
########

enable_testing()

function(add_test_i8080 name)
	add_test(NAME "${name}" COMMAND i8080 -- "${CMAKE_CURRENT_SOURCE_DIR}/test/${name}.COM")
endfunction()

add_test_i8080(8080EXM)
add_test_i8080(8080PRE)
add_test_i8080(CPUTEST)
add_test_i8080(TST8080)

