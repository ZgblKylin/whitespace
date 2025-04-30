project(whitespace)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin)

option(WHITESPACE_DEBUG "Use S/T/L instead of \\s \\t \\n" OFF)
option(WHITESPACE_DISABLE_STATIC_STACK "Disable static stack, use dynamic allocation" OFF)
set(WHITESPACE_STATIC_STACK_SIZE "1024" CACHE STRING "Static stack size")

add_library(whitespace OBJECT)
target_sources(whitespace PRIVATE src/whitespace.h src/whitespace.c)
target_include_directories(whitespace PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src)
if(WHITESPACE_DEBUG)
  target_compile_definitions(whitespace PUBLIC -DWHITESPACE_DEBUG)
endif()
if(WHITESPACE_DISABLE_STATIC_STACK)
  target_compile_definitions(whitespace PUBLIC -DWHITESPACE_DYNAMIC_STACK)
else()
  target_compile_definitions(whitespace PUBLIC -DWHITESPACE_STATIC_STACK_SIZE=${WHITESPACE_STATIC_STACK_SIZE})
endif()
target_compile_definitions(whitespace PUBLIC -D_CRT_SECURE_NO_WARNINGS)

add_executable(1_100_1 test/1_100_1.c)
target_link_libraries(1_100_1 PUBLIC whitespace)

add_executable(run_code test/run_code.c)
target_link_libraries(run_code PUBLIC whitespace)
