file(GLOB_RECURSE SOURCES "*.cpp")

add_library(MarbleSema STATIC ${SOURCES})

target_include_directories(MarbleSema 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/Include>
        $<INSTALL_INTERFACE:Include>
)