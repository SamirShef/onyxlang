file(GLOB_RECURSE SOURCES "*.cpp")

add_library(MarbleBasic STATIC ${SOURCES})

target_include_directories(MarbleBasic 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/Include>
        $<INSTALL_INTERFACE:Include>
)