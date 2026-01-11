file(GLOB_RECURSE SOURCES "*.cpp")

add_library(OnyxCompilation STATIC ${SOURCES})

target_include_directories(OnyxCompilation 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/Include>
        $<INSTALL_INTERFACE:Include>
)