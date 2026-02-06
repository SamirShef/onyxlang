file(GLOB_RECURSE SOURCES "*.cpp")

add_library(MarbleLexer STATIC ${SOURCES})

target_include_directories(MarbleLexer 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/Include>
        $<INSTALL_INTERFACE:Include>
)