file(GLOB_RECURSE SOURCES "*.cpp")

add_library(OnyxSema STATIC ${SOURCES})

target_include_directories(OnyxSema 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/Include>
        $<INSTALL_INTERFACE:Include>
)