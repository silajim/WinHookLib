cmake_minimum_required(VERSION 3.14)

project(WinHook LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core REQUIRED)

add_library(WinHookLib STATIC  "windowswindows.h"  "windowswindows.cpp" )

target_link_libraries(WinHookLib  Qt${QT_VERSION_MAJOR}::Core psapi dwmapi)

target_include_directories(WinHookLib PUBLIC ${CMAKE_CURRENT_LIST_DIR})

#add_executable(WinHook main.cpp)
#target_link_libraries(WinHook Qt${QT_VERSION_MAJOR}::Core psapi)
