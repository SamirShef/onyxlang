file(GLOB_RECURSE SOURCES "*.cpp")

add_library(OnyxAST STATIC ${SOURCES})

target_include_directories(OnyxAST 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/Include>
        $<INSTALL_INTERFACE:Include>
)