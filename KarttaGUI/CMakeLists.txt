cmake_minimum_required(VERSION 3.16)

project(KarttaGUI)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(OpenCV REQUIRED)

add_executable(KarttaGUI
    src/main.cpp
    src/MainWindow.cpp
    src/KarttaRunner.cpp
)

target_include_directories(KarttaGUI PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(KarttaGUI PRIVATE
    Qt6::Widgets
    ${OpenCV_LIBS}
)
