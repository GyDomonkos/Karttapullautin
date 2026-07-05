# KarttaGUI

A Qt6-based graphical frontend for [Karttapullautin](https://github.com/karttapullautin/karttapullautin) — the LiDAR-to-orienteering-map processing engine.

KarttaGUI wraps Karttapullautin's command-line workflow in a modern Windows desktop application, exposing its full configuration surface through a native UI and providing live map preview as tiles are generated.

**Copyright (c) 2026 Domonkos Gyorffy. All Rights Reserved.**

---

## What KarttaGUI does

1. You select a folder of LAS/LAZ point cloud files and an output folder.
2. KarttaGUI writes your settings to `pullauta.ini` and launches Karttapullautin.
3. Generated map tiles appear in the preview panel in real time as each tile completes.
4. Post-processing tools (tile merging, vegetation and cliff regeneration) are available once the batch finishes.

KarttaGUI never modifies the Karttapullautin binary. It is purely a configuration, execution, and visualisation layer.

---

## Features

### Run tab
- Folder picker for LAS/LAZ input and PNG output
- Drag-and-drop: drop a folder or a single LAS/LAZ file directly onto the window
- **Existing output detection:** if the output folder already contains tile PNGs, a dialog asks whether to clear them for a full reprocess or skip existing tiles
- Run / Cancel buttons with a live tile-count progress bar (`X / N tiles`)
- Open Output Folder button — opens the output directory in Explorer with one click
- Full process log in a scrollable console

### Settings tab
Full control over `pullauta.ini` parameters, organised into groups:

| Group | Parameters |
|---|---|
| **Contours** | Interval, form line mode, smoothing, curviness, knoll sensitivity, index contour interval |
| **Vegetation** | Undergrowth (slow run / walk), green ground, green high, yellow height/threshold, **variable green shade levels** (default: 0.8 \| 1.3 \| 2.0 for standard OMaps) |
| **Cliffs** | Passable/impassable thresholds, steep factor, flat place threshold, minimum cliff length |
| **Processing** | Parallel processes, north lines, scale factor, Z offset, DXF output, save temp files, save temp folders |
| **Optional features** | Water class, water elevation threshold, buildings class, building detection |

Settings are written to `pullauta.ini` on every run. Green shade thresholds are always written from the UI — the INI value is intentionally ignored to preserve your preferred defaults.

#### Presets
Save any combination of settings as a named preset and reload it at any time.
- **Save As…** — prompts for a name, warns before overwriting
- **Load** — applies all preset values to the form instantly
- **Delete** — removes the preset file permanently

Presets are stored as human-readable JSON in `%AppData%\KarttaGUI\presets\` and can be shared or backed up freely.

### Preview panel
- Thumbnail list of all generated PNG tiles, updated live as tiles finish
- Full zoomable map view using `QGraphicsView`
- **Physically calibrated zoom:** 100% = true 1:10,000 scale on your screen's actual DPI (Karttapullautin renders at 600 DPI for 1:10,000)
- Preset zoom buttons: **Fit · 50% (1:20,000) · 100% (1:10,000) · 200% (1:5,000) · 400% (1:2,500)**
- Mouse-wheel zoom and drag-to-pan

### Tools tab
Available after a batch run completes.

**Iterative & Partial Processing**
- Quick Re-render — re-renders PNG maps from existing temp data (useful after changing north lines or cosmetic settings)
- Regenerate Vegetation Only — re-runs green/yellow classification with updated undergrowth parameters *(requires Save temp folders)*
- Regenerate Cliffs Only — re-runs cliff detection with custom smoothing and steepness *(requires Save temp folders)*

**Batch Tile Merging**
- Merge Standard Maps — stitches all tile PNGs into `merged.png` / `merged.jpg`
- Merge Depression Maps — same for the depression-variant tiles
- Merge Vegetation Backgrounds — stitches `_vege.png` files *(requires Save temp files)*
- Merge DXF Outputs — combines per-tile DXF vector files *(requires DXF output + Save temp files)*

All merged files are automatically moved from the Karttapullautin working directory to your output folder.

---

## Project structure

```
Karttapullautin/
├── CMakeLists.txt                  (root, adds KarttaGUI subdirectory)
└── KarttaGUI/
    ├── CMakeLists.txt
    ├── src/
    │   ├── main.cpp
    │   ├── MainWindow.cpp / .h     — layout, INI writing, run/cancel logic
    │   ├── KarttaRunner.cpp / .h   — QProcess wrapper
    │   ├── PreviewPanel.cpp / .h   — live tile viewer (QGraphicsView + QFileSystemWatcher)
    │   ├── SettingsPanel.cpp / .h  — settings form with preset bar
    │   ├── PresetManager.cpp / .h  — JSON preset read/write (AppData storage)
    │   └── ToolsPanel.cpp / .h     — post-processing tool buttons
    └── resources/
        ├── icon.png
        └── resources.rc
```

The `karttapullautin/` folder (containing the `pullauta` executable and `pullauta.ini`) must sit alongside `KarttaGUI.exe` in the deployment directory.

---

## Requirements

| Component | Notes |
|---|---|
| Windows 10 or 11 | Primary supported platform |
| CMake ≥ 3.16 | |
| Qt 6.x (MinGW version) | e.g. `C:\Qt\6.10.2\mingw_64` |
| MinGW toolchain | Must match the Qt build |
| OpenCV | Optional — currently unused; hooks in place for future features |

Qt and OpenCV (if used) must be built with the same MinGW compiler version.

---

## Build instructions (Windows + MinGW)

```powershell
# 1. Create build directory
mkdir build-mingw
cd build-mingw

# 2. Configure
cmake .. -G "MinGW Makefiles" `
  -DCMAKE_PREFIX_PATH=C:/Qt/6.10.2/mingw_64 `
  -DOpenCV_DIR=C:/opencv/build/x64/mingw/install

# 3. Build
cmake --build . -j8
```

The executable is written to `build-mingw/KarttaGUI/`.

---

## Deployment (Windows)

```powershell
# Deploy Qt runtime DLLs
C:\Qt\6.10.2\mingw_64\bin\windeployqt.exe KarttaGUI.exe

# Copy Karttapullautin next to the executable
# The folder must be named exactly "karttapullautin" and contain
# pullauta.exe and pullauta.ini
```

If DLL errors occur at runtime, verify that Qt and any other dependencies were built with the same MinGW version.

---

## Printing at correct scale

Karttapullautin outputs PNGs at **600 DPI** for **1:10,000** scale. To print at the correct physical scale:

1. Open the PNG in IrfanView
2. Image → Information → set resolution to **600 × 600 DPI** → Change → Save
3. Print using **"Original Size from DPI"**

KarttaGUI's preview panel reflects this: the **100% zoom level** corresponds to the true 1:10,000 physical scale on your monitor.

---

## License

**Copyright (c) 2026 Domonkos Gyorffy. All Rights Reserved.**

This project is **Source-Available**, not Open Source.

- **Use & compilation:** You may compile and use this software for private, non-commercial purposes.
- **Redistribution:** You may not redistribute, modify, or sell this software or its source code without explicit written permission from the author.
- **Qt attribution:** This software uses the Qt framework under the LGPL. Relevant license files are included in the distribution.

---

## Disclaimer

This software is provided without warranty of any kind. Use at your own risk.