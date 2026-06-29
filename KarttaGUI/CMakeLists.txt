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
    src/SettingsPanel.cpp
    resources/resources.rc
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



# ==============================================================================
# DEPLOYMENT AUTOMATION
# ==============================================================================

# 1. Install the main executable
install(TARGETS KarttaGUI RUNTIME DESTINATION bin)

# 2. Copy the external karttapullautin folder (executable + ini)
# Assumes 'karttapullautin' is in the project root (.. relative to KarttaGUI)
# Get the absolute path to where CMake thinks the folder is
get_filename_component(KARTTA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../karttapullautin" ABSOLUTE)

# Print it to the console during generation so you can verify it
message(STATUS "Looking for karttapullautin folder at: ${KARTTA_DIR}")

# Copy it (if it exists)
if(EXISTS "${KARTTA_DIR}")
    install(DIRECTORY "${KARTTA_DIR}" DESTINATION bin)
    message(STATUS "SUCCESS: Will copy karttapullautin folder!")
else()
    message(WARNING "ERROR: Could not find karttapullautin folder at ${KARTTA_DIR} - please check the path!")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources")
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources" DESTINATION bin)
    message(STATUS "SUCCESS: Will copy resources folder to deployment folder!")
else()
    message(WARNING "WARNING: Could not find resources folder at ${CMAKE_CURRENT_SOURCE_DIR}/resources")
endif()

# 3. Copy OpenCV DLLs automatically
# We look for the DLLs relative to where CMake found OpenCV
if(OpenCV_FOUND)
    # Try to find the bin directory based on the include directory
    get_filename_component(OPENCV_ROOT "${OpenCV_DIR}/../../.." ABSOLUTE)
    install(DIRECTORY "${OPENCV_ROOT}/bin/" 
            DESTINATION bin 
            FILES_MATCHING PATTERN "*.dll")
endif()

# 4. Automate windeployqt (Post-Install step)
# Note: Ensure CMAKE_PREFIX_PATH is set (as in your build.sh)
set(QT_BIN_DIR "${CMAKE_PREFIX_PATH}/bin") 

install(CODE "
    message(STATUS \"Running windeployqt to collect Qt dependencies...\")
    execute_process(
        COMMAND \"${QT_BIN_DIR}/windeployqt.exe\" \"\${CMAKE_INSTALL_PREFIX}/bin/KarttaGUI.exe\"
        WORKING_DIRECTORY \"\${CMAKE_INSTALL_PREFIX}/bin\"
    )
")
