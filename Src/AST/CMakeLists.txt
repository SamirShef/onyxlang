file(GLOB_RECURSE SOURCES "*.cpp")

add_library(MarbleAST STATIC ${SOURCES})

target_include_directories(MarbleAST 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/Include>
        $<INSTALL_INTERFACE:Include>
)