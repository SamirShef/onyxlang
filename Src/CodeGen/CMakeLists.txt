file(GLOB_RECURSE SOURCES "*.cpp")

add_library(MarbleCodeGen STATIC ${SOURCES})

target_include_directories(MarbleCodeGen 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/Include>
        $<INSTALL_INTERFACE:Include>
)