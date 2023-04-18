cmake_minimum_required(VERSION 3.5)

project(simple_video_player LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

#ui文件转换
set(CMAKE_AUTOUIC ON)

#信号与槽实现
set(CMAKE_AUTOMOC ON)

#资源文件转换
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets Multimedia LinguistTools REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets Multimedia LinguistTools REQUIRED)

# set(TS_FILES  simple_video_player_zh_CN.ts)


#ffmpeg
include_directories(
${CMAKE_CURRENT_SOURCE_DIR}/include
)

link_directories(
${CMAKE_CURRENT_SOURCE_DIR}/lib
)

add_executable(simple_video_player
main.cpp
widget.cpp
widget.h
widget.ui
#${TS_FILES}   #加了这句会报错
)


target_link_libraries(simple_video_player
PRIVATE
Qt${QT_VERSION_MAJOR}::Widgets
Qt${QT_VERSION_MAJOR}::Multimedia
)

#关于ffmpeg
target_link_libraries(simple_video_player
PRIVATE
avcodec
avfilter
avformat
avutil
avdevice
swscale
swresample
pthread
)

#qt5_create_translation/(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
