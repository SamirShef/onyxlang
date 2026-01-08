file(GLOB_RECURSE SOURCES "*.cpp")

add_library(OnyxLexer STATIC ${SOURCES})

target_include_directories(OnyxLexer 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)