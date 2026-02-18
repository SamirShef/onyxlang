cmake_minimum_required(VERSION 3.10)
project(marblec)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_options(-w)

include_directories(Include)

message(STATUS "Found LLVM ${LLVM_VERSION}")
message(STATUS "LLVM includes: ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM libraries: ${LLVM_LIBRARY_DIRS}")

include_directories(Include)

find_program(LLVM_CONFIG llvm-config REQUIRED)

execute_process(
    COMMAND ${LLVM_CONFIG} --cxxflags
    OUTPUT_VARIABLE LLVM_CXXFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)                                                                          
execute_process(
    COMMAND ${LLVM_CONFIG} --ldflags
    OUTPUT_VARIABLE LLVM_LDFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND ${LLVM_CONFIG} --libs
    OUTPUT_VARIABLE LLVM_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND ${LLVM_CONFIG} --includedir
    OUTPUT_VARIABLE LLVM_INCLUDE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

include_directories(${LLVM_INCLUDE_DIR})

add_subdirectory(Src/Basic)
add_subdirectory(Src/Lexer)
add_subdirectory(Src/AST)
add_subdirectory(Src/Parser)
add_subdirectory(Src/Sema)
add_subdirectory(Src/CodeGen) # TODO: uncomment
add_subdirectory(Src/Compilation)

add_definitions(-w)
add_executable(${PROJECT_NAME} Src/Main.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE ${LLVM_INCLUDE_DIRS}include)
target_compile_definitions(${PROJECT_NAME} PRIVATE ${LLVM_DEFINITIONS})
separate_arguments(LLVM_LIBS)
separate_arguments(LLVM_LDFLAGS)
separate_arguments(LLVM_CXXFLAGS)
target_compile_options(${PROJECT_NAME} PRIVATE ${LLVM_CXXFLAGS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${LLVM_LIBS} ${LLVM_LDFLAGS})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    -Wl,--start-group
    MarbleBasic
    MarbleLexer
    MarbleAST
    MarbleParser
    MarbleSema
    MarbleCodeGen             # TODO: uncomment
    MarbleCompilation         # TODO: uncomment
    -Wl,--end-group
)
