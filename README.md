# KarttaGUI

A Qt6 + OpenCV based graphical frontend for Karttapullautin.

KarttaGUI provides a modern Windows desktop interface for running and managing Karttapullautin workflows using a MinGW-based toolchain.

This project is licensed under the GNU General Public License v3.0 (GPLv3).

---

## Overview

KarttaGUI is designed as a user-friendly GUI wrapper around Karttapullautin.
It integrates:

* Qt6 (Widgets)
* OpenCV (Optional/Planned for advanced image processing)
* CMake build system
* MinGW (GCC) toolchain

The goal is to simplify map processing workflows, provide real-time visual feedback, and expose powerful configuration options.

---

## Features

### User Interface & Layout
* **Modern Split-Pane Layout:** Utilizes a resizable `QSplitter` interface to cleanly divide configuration controls and execution panels on the left from the live map preview area on the right.
* **Live Map Preview System:** Actively monitors the processing output folder using `QFileSystemWatcher` to discover new map tiles in real time, instantly loading them into a dedicated thumbnail list.
* **Optimized Image Viewer:** Implements an advanced thumbnail viewer via `QStackedWidget` that toggles smoothly from a placeholder view to an interactive, scale-aware image panel utilizing `QImageReader::setScaledSize()` for lightweight memory and rendering overhead.

### Engine Integration & Processing
* **Asynchronous Process Runner:** Executes the Karttapullautin backend (`pullauta`) out-of-process using a customized `QProcess` wrapper. The desktop client remains perfectly responsive throughout heavy rendering jobs.
* **Real-Time Log Parsing & Progress Tracking:** Tracks real-time stdout/stderr outputs to capture rendering patterns (e.g., counting `"All done!"` and `"exists already..."` outputs), driving an accurate `QProgressBar` with live feedback.
* **Format-Preserving INI Parsing:** Features a bespoke `writeIniValues()` engine that applies configuration updates line-by-line, dynamically injecting new properties while seamlessly preserving user comments, spacing, and formatting.
* **Dynamic Settings Panel:** Exposes comprehensive generation options (Contours, Vegetation, Cliffs, and Processing configurations) through a unified, native scroll area configuration grid.

### Build & Architecture
* **Native Windows Support:** Tailored and validated specifically for Windows 10/11 platforms utilizing a modern MinGW (GCC) toolchain.
* **Clean CMake Build System:** Standardized orchestration of compiler settings, standard library links, and deployment configurations.
* **Modern, Modular C++ Archetype:** Built on modular architectural practices using C++17 paradigms for robust scope handling and asynchronous event handling.
* **Extensible Image Pipeline:** Architected with structural hooks allowing seamless integration of OpenCV dependencies for future image processing workflows.

---

## Project Structure

```
Karttapullautin/
│
├── CMakeLists.txt
├── KarttaGUI/
│   ├── CMakeLists.txt
│   ├── src/
│   │   ├── main.cpp
│   │   ├── MainWindow.cpp / .h      # Main UI layout, INI parsing, and execution glue
│   │   ├── KarttaRunner.cpp / .h    # QProcess wrapper for asynchronous execution
│   │   ├── PreviewPanel.cpp / .h    # QFileSystemWatcher and QStackedWidget image viewer
│   │   └── SettingsPanel.cpp / .h   # QScrollArea UI for pullauta.ini configuration
│   └── resources/
│
└── build-mingw/
```

---

## Requirements

* Windows 10 or 11
* CMake ≥ 3.16
* Qt 6.x (MinGW version)
* OpenCV built with MinGW
* MinGW matching the Qt toolchain

Example installation paths:

```
C:\Qt\6.10.2\mingw_64
C:\opencv\build\x64\mingw\install
```

Important: Qt and OpenCV must be built with the same compiler (MinGW).

---

## Build Instructions (Windows + MinGW)

### 1. Create build directory

```powershell
mkdir build-mingw
cd build-mingw
```

### 2. Configure

```powershell
cmake .. -G "MinGW Makefiles" ^
 -DCMAKE_PREFIX_PATH=C:/Qt/6.10.2/mingw_64 ^
 -DOpenCV_DIR=C:/opencv/build/x64/mingw/install
```

### 3. Build

```powershell
cmake --build . -j8
```

The executable will be generated inside:

```
build-mingw/
```

---

## Deployment (Windows)

After building, deploy Qt runtime dependencies:

```powershell
C:\Qt\6.10.2\mingw_64\bin\windeployqt.exe KarttaGUI.exe
```

This copies required Qt DLLs and plugin folders.

You must also copy OpenCV runtime DLLs from:

```
C:\opencv\build\x64\mingw\install\x64\mingw\bin
```

into the same directory as `KarttaGUI.exe`.

---

## Running the Application

After deployment:

```
KarttaGUI.exe
```

If DLL errors occur, verify:

* Qt and OpenCV were built with MinGW
* All required DLLs are in the executable directory
* No MSVC builds are mixed with MinGW

---

## License

This project is licensed under the GNU General Public License v3.0.

You are free to:

* Use
* Study
* Modify
* Distribute

Under the following conditions:

* Any distributed modified version must also be licensed under GPLv3.
* Source code must be made available when distributing binaries.
* No additional restrictions may be imposed.


For more details on the license, see:
[https://www.gnu.org/licenses/gpl-3.0.html](https://www.gnu.org/licenses/gpl-3.0.html)

---

## Contributing

Contributions are welcome.

By submitting changes, you agree that your contributions will be licensed under GPLv3.

---

## Disclaimer

This project is provided without warranty of any kind.
See the GPLv3 license for full details.
