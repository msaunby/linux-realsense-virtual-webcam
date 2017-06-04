#
# This cmake file based on the one included in librealsense-master/examples
# Something simpler should be possible.  I know next to nothing about cmake.
# It works for me.  Michael Saunby. 3 June 2017
#
cmake_minimum_required(VERSION 2.8.3)

project(MyProject)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
# View the makefile commands during build
#set(CMAKE_VERBOSE_MAKEFILE on)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

find_package(OpenGL REQUIRED)
set(DEPENDENCIES realsense ${OPENGL_LIBRARIES})

find_package(PkgConfig)

pkg_check_modules(GSTREAMER gstreamer-1.0)

link_directories(
   ${GSTREAMER_LIBRARY_DIRS}
)
include_directories(
   ${GSTREAMER_INCLUDE_DIRS}
)

if(WIN32)
    add_subdirectory(third_party/glfw)
    list(APPEND DEPENDENCIES glfw3)
else()
    # Find glfw header
    find_path(GLFW_INCLUDE_DIR NAMES GLFW/glfw3.h
        PATHS /usr/X11R6/include
              /usr/include/X11
              /opt/graphics/OpenGL/include
              /opt/graphics/OpenGL/contrib/libglfw
              /usr/local/include
              /usr/include/GL
              /usr/include
    )
    # Find glfw library
    find_library(GLFW_LIBRARIES NAMES glfw glfw3
            PATHS /usr/lib64
                  /usr/lib
                  /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}
                  /usr/local/lib64
                  /usr/local/lib
                  /usr/local/lib/${CMAKE_LIBRARY_ARCHITECTURE}
                  /usr/X11R6/lib
    )
    list(APPEND DEPENDENCIES m  ${GLFW_LIBRARIES} ${LIBUSB1_LIBRARIES})
    # Find gstapp-1.0 library
    find_library(GSTAPP_LIBRARY NAMES gstapp-1.0
            PATHS /usr/lib/x86_64-linux-gnu
                  /usr/lib64
                  /usr/lib
                  /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}
                  /usr/local/lib64
                  /usr/local/lib
                  /usr/local/lib/${CMAKE_LIBRARY_ARCHITECTURE}
                  /usr/X11R6/lib
    )
    # gstreamer-1.0 gobject-2.0 glib-2.0
    find_library(GST_LIBRARY NAMES gstreamer-1.0
    )
    find_library(GOBJ_LIBRARY NAMES gobject-2.0
    )
    find_library(GLIB_LIBRARY NAMES glib-2.0
    )
    list(APPEND DEPENDENCIES m ${GST_LIBRARY} ${GOBJ_LIBRARY} ${GLIB_LIBRARY} ${GSTAPP_LIBRARY} )
    include_directories(${GLFW_INCLUDE_DIR})
endif()

# Projects

add_executable(colorcam colorcam.c)
target_link_libraries(colorcam ${DEPENDENCIES})

add_executable(pointcloudcam pointcloudcam.c)
#target_link_libraries(pointcloudcam ${DEPENDENCIES})
target_link_libraries(pointcloudcam  ${DEPENDENCIES} ${GLFW_LIBRARIES} ${LIBUSB1_LIBRARIES} ${GST_LIBRARIES} ${GSTAPP_LIBRARY} )

install(
    TARGETS
    colorcam
    pointcloudcam

    RUNTIME DESTINATION
    ${CMAKE_INSTALL_BINDIR}
)
