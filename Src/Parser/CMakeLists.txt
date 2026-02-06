file(GLOB_RECURSE SOURCES "*.cpp")

add_library(MarbleParser STATIC ${SOURCES})

target_include_directories(MarbleParser 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/Include>
        $<INSTALL_INTERFACE:Include>
)