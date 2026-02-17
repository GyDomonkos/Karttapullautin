cmake_minimum_required(VERSION 3.16)
project(KarttaGUI)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(OpenCV REQUIRED)

add_executable(KarttaGUI
    src/main.cpp
    src/MainWindow.cpp
    src/KarttaRunner.cpp
)

target_include_directories(KarttaGUI PRIVATE
    src
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(KarttaGUI PRIVATE
    Qt6::Widgets
    ${OpenCV_LIBS}
)
