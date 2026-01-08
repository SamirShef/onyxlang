file(GLOB_RECURSE SOURCES "*.cpp")

add_library(OnyxCodeGen STATIC ${SOURCES})

target_include_directories(OnyxCodeGen 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)