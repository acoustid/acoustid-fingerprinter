cmake_minimum_required(VERSION 2.6)

project(fpsubmit)
set(fpsubmit_VERSION 0.5)

set(LIB_SUFFIX "" CACHE STRING "Define suffix of directory name (32/64)")
set(EXEC_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} CACHE PATH "Installation prefix for executables and object code libraries" FORCE)
set(BIN_INSTALL_DIR ${EXEC_INSTALL_PREFIX}/bin CACHE PATH "Installation prefix for user executables" FORCE)
set(LIB_INSTALL_DIR ${EXEC_INSTALL_PREFIX}/lib${LIB_SUFFIX} CACHE PATH  "Installation prefix for object code libraries" FORCE)
set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/include CACHE PATH "Installation prefix for C header files" FORCE)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

find_package(Qt4 COMPONENTS QtCore QtGui QtNetwork REQUIRED)
find_package(FFmpeg REQUIRED)
find_package(Taglib REQUIRED)
find_package(Chromaprint REQUIRED)

include(${QT_USE_FILE})

if(CMAKE_BUILD_TYPE STREQUAL Release OR CMAKE_BUILD_TYPE STREQUAL MinSizeRel OR CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo)
	add_definitions(-DQT_NO_DEBUG_OUTPUT)
endif(CMAKE_BUILD_TYPE STREQUAL Release OR CMAKE_BUILD_TYPE STREQUAL MinSizeRel OR CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo)

set(fpsubmit_HEADERS
	checkabledirmodel.h
	progressdialog.h
	fingerprinter.h
	mainwindow.h
	loadfilelisttask.h
	analyzefiletask.h
)
set(fpsubmit_SOURCES
	checkabledirmodel.cpp
	progressdialog.cpp
	fingerprinter.cpp
	fingerprintcalculator.cpp
	tagreader.cpp
	mainwindow.cpp
	decoder.cpp
	main.cpp
	loadfilelisttask.cpp
	analyzefiletask.cpp
	updatelogfiletask.cpp
	crc.c
	gzip.cpp
)
#set(fpsubmit_UIS fpsubmit.ui)
set(fpsubmit_RESOURCES fingerprinter.qrc)

qt4_wrap_cpp(fpsubmit_MOC ${fpsubmit_HEADERS})
qt4_wrap_ui(fpsubmit_UIS_H ${fpsubmit_UIS})
qt4_add_resources(fpsubmit_RESOURCES_CPP ${fpsubmit_RESOURCES})

if(WIN32)
	set(fpsubmit_SOURCES ${fpsubmit_SOURCES} fingerprinter.rc)
endif()

add_definitions(
	-D__STDC_LIMIT_MACROS
	-D__STDC_CONSTANT_MACROS
	-DVERSION="${fpsubmit_VERSION}"
)

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${FFMPEG_LIBAVCODEC_INCLUDE_DIRS}
	${FFMPEG_LIBAVFORMAT_INCLUDE_DIRS}
	${FFMPEG_LIBAVUTIL_INCLUDE_DIRS}
	${TAGLIB_INCLUDES}
	${CHROMAPRINT_INCLUDE_DIR}
)

if(WIN32)
set(GUI_TYPE WIN32)
endif(WIN32)

if(APPLE)
    set(GUI_TYPE MACOSX_BUNDLE)
    set(fpsubmit_ICON_FILE ${CMAKE_CURRENT_SOURCE_DIR}/images/acoustid-fp.icns)
    set_source_files_properties(${fpsubmit_ICON_FILE}
        PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources)
	set(fpsubmit_SOURCES ${fpsubmit_SOURCES} ${fpsubmit_ICON_FILE})
endif()

add_executable(fpsubmit ${GUI_TYPE}
    ${fpsubmit_SOURCES}
    ${fpsubmit_MOC}
    ${fpsubmit_UIS_H}
    ${fpsubmit_RESOURCES_CPP}
)

set_target_properties(fpsubmit PROPERTIES
	OUTPUT_NAME acoustid-fingerprinter
    MACOSX_BUNDLE_ICON_FILE acoustid-fp.icns
	MACOSX_BUNDLE_INFO_STRING "Acoustid Fingerprinter ${fpsubmit_VERSION}"
	MACOSX_BUNDLE_BUNDLE_NAME "Acoustid Fingerprinter"
)

target_link_libraries(fpsubmit
	${QT_LIBRARIES}
	${FFMPEG_LIBAVFORMAT_LIBRARIES}
	${FFMPEG_LIBAVCODEC_LIBRARIES}
	${FFMPEG_LIBAVUTIL_LIBRARIES}
	${TAGLIB_LIBRARIES}
	${CHROMAPRINT_LIBRARIES}
)

if(APPLE)
	target_link_libraries(fpsubmit "-framework Accelerate -lz")
endif()

if(UNIX)
	install(FILES acoustid-fingerprinter.desktop DESTINATION share/applications)
endif()

install(TARGETS fpsubmit DESTINATION ${BIN_INSTALL_DIR})
