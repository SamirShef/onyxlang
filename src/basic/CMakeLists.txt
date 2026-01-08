file(GLOB_RECURSE SOURCES "*.cpp")

add_library(OnyxBasic STATIC ${SOURCES})

target_include_directories(OnyxBasic 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)