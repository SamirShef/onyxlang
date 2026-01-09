cmake_minimum_required(VERSION 3.10)
project(onyxc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(include)

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_VERSION}")
message(STATUS "LLVM includes: ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM libraries: ${LLVM_LIBRARY_DIRS}")

include_directories(include)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_subdirectory(src/basic)
add_subdirectory(src/lexer)
add_subdirectory(src/ast)
add_subdirectory(src/parser)
add_subdirectory(src/sema)
add_subdirectory(src/codegen)

add_executable(${PROJECT_NAME} src/Main.cpp)

target_include_directories(${PROJECT_NAME} 
    PRIVATE 
        ${LLVM_INCLUDE_DIRS}
        include
)
target_compile_definitions(${PROJECT_NAME} PRIVATE ${LLVM_DEFINITIONS})

if (TARGET LLVM)
    target_link_libraries(${PROJECT_NAME} PRIVATE LLVM)
else()
    llvm_map_components_to_libnames(LLVM_LIBS all)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${LLVM_LIBS})
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE 
                                              -Wl,--start-group
                                              OnyxBasic
                                              OnyxLexer
                                              OnyxAST
                                              OnyxParser
                                              OnyxSema
                                              OnyxCodeGen
                                              -Wl,--end-group
)