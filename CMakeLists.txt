cmake_minimum_required(VERSION 3.30)

project(GS3108TPro VERSION 0.2.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets Svg)

qt_standard_project_setup()

qt_add_executable(gs3108t-pro
    src/main.cpp
    src/hidmanager.h
    src/hidmanager.cpp
    src/keyboardcontroller.h
    src/keyboardcontroller.cpp
    src/configstore.h
    src/configstore.cpp
    src/lightmodes.h
    src/presetstore.h
    src/presetstore.cpp
    src/theme.h
    src/theme.cpp
    src/widgets.h
    src/widgets.cpp
    src/pages.h
    src/pages.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    resources.qrc
)

target_link_libraries(gs3108t-pro PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Svg
)

target_compile_options(gs3108t-pro PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

install(TARGETS gs3108t-pro
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)
