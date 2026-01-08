file(GLOB_RECURSE SOURCES "*.cpp")

add_library(OnyxParser STATIC ${SOURCES})

target_include_directories(OnyxParser 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)