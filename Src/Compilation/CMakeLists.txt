file(GLOB_RECURSE SOURCES "*.cpp")

add_library(MarbleCompilation STATIC ${SOURCES})

target_include_directories(MarbleCompilation 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/Include>
        $<INSTALL_INTERFACE:Include>
)