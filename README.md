# KarttaGUI

A Qt6 + OpenCV based graphical frontend for Karttapullautin.

KarttaGUI provides a modern Windows desktop interface for running and managing Karttapullautin workflows using a MinGW-based toolchain.

This project is licensed under the GNU General Public License v3.0 (GPLv3).

---

## Overview

KarttaGUI is designed as a user-friendly GUI wrapper around Karttapullautin.
It integrates:

* Qt6 (Widgets)
* OpenCV
* CMake build system
* MinGW (GCC) toolchain

The goal is to simplify map processing workflows while keeping the project fully open source.

---

## Features

* Qt6 Widgets-based graphical interface
* OpenCV integration
* Clean CMake-based build system
* Windows (MinGW) support
* Modular C++17 codebase

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
│   │   ├── MainWindow.cpp
│   │   ├── MainWindow.h
│   │   ├── KarttaRunner.cpp
│   │   └── KarttaRunner.h
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

Just tell me your preference.
