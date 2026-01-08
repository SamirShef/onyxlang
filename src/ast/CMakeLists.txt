file(GLOB_RECURSE SOURCES "*.cpp")

add_library(OnyxAST STATIC ${SOURCES})
message(STATUS ${CMAKE_SOURCE_DIR})

target_include_directories(OnyxAST 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)