cmake_minimum_required(VERSION 3.16)

project(KarttaGUI)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

# OpenCV is optional for now — required once image processing features land
find_package(OpenCV QUIET)

add_executable(KarttaGUI
    src/main.cpp
    src/MainWindow.cpp
    src/KarttaRunner.cpp
    src/PreviewPanel.cpp
)

target_include_directories(KarttaGUI PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(KarttaGUI PRIVATE
    Qt6::Widgets
)

if(OpenCV_FOUND)
    target_link_libraries(KarttaGUI PRIVATE ${OpenCV_LIBS})
    target_compile_definitions(KarttaGUI PRIVATE HAVE_OPENCV)
    message(STATUS "OpenCV found — image processing features enabled")
else()
    message(STATUS "OpenCV not found — image processing features disabled")
endif()