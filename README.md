# RPLidar C1 GUI Visualizer

A C++17 / Qt6 desktop application for real-time visualization and data recording
from a **Slamtec RPLidar C1** connected via USB.

---

## Features

| Feature | Details |
|---|---|
| Live 2D polar scan view | Custom QPainter canvas with zoom (scroll wheel) and pan (drag) |
| Colour-by-distance | Near = bright green → Far = blue |
| Range ring grid | Auto-scaled or fixed range |
| Quality filter | Slider to hide low-confidence points |
| Motor speed control | Adjustable RPM via SDK |
| Frame recording | Buffer N frames with one button click |
| Snapshot | Save the current canvas as PNG |
| Export | CSV · PCD · PLY · JSON · MCAP (ROS bag) |

---

## Prerequisites

| Tool | Version |
|---|---|
| CMake | ≥ 3.20 |
| Qt6 | 6.x (Core, Gui, Widgets, SerialPort) |
| C++ compiler | GCC 11+ / Clang 14+ / MSVC 2022 |
| Slamtec RPLidar SDK | Latest from GitHub |
| nlohmann/json | v3.11+ (single header) |

---

## Setup

### 1 — Get the RPLidar SDK

```bash
git clone https://github.com/Slamtec/rplidar_sdk.git third_party/rplidar_sdk
```

### 2 — Get nlohmann/json

```bash
curl -L https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp \
     -o third_party/json.hpp
```

### 3 — Install Qt6

**Ubuntu / Debian:**
```bash
sudo apt install qt6-base-dev qt6-serialport-dev
```

**Windows** — use the [Qt Online Installer](https://www.qt.io/download) and select:
- Qt 6.x → MSVC 2022 64-bit
- Qt Serial Port

**macOS:**
```bash
brew install qt6
```

### 4 — Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

On Windows with MSVC:
```cmd
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

---

## Serial port access (Linux)

Add your user to the `dialout` group so the app can open `/dev/ttyUSB0`:

```bash
sudo usermod -aG dialout $USER
# log out and back in
```

---

## Running

```bash
./RPLidarGUI          # Linux / macOS
RPLidarGUI.exe        # Windows
```

1. Plug in the RPLidar C1 via USB.
2. Select the correct COM / ttyUSB port in the left panel.
3. Click **Connect**.
4. The scan appears in the dark canvas immediately.
5. Use the **scroll wheel** to zoom and **drag** to pan.
6. Click **● Start recording** to buffer frames.
7. Click **💾 Export data…** to save to your chosen format.

---

## Export formats

| Format | File | Best for |
|---|---|---|
| CSV | `.csv` | Excel, MATLAB, pandas |
| PCD | `.pcd` | PCL, ROS, CloudCompare |
| PLY | `.ply` | MeshLab, Blender, CloudCompare |
| JSON | `.json` | Web apps, REST APIs |
| MCAP | `.mcap` | ROS 2 bag (CSV-inside for compatibility) |

---

## Project structure

```
rplidar-gui/
├── CMakeLists.txt
├── third_party/
│   ├── rplidar_sdk/          ← Slamtec SDK (git clone)
│   └── json.hpp              ← nlohmann/json single header
└── src/
    ├── main.cpp
    ├── core/
    │   ├── ScanPoint.h       ← data type (angle, distance, quality)
    │   ├── ScanBuffer.h/.cpp ← thread-safe ring buffer
    │   ├── LidarDriver.h/.cpp← SDK wrapper, runs in QThread
    │   └── DataExporter.h/.cpp← all 5 export formats
    └── gui/
        ├── MainWindow.h/.cpp ← top-level window
        ├── ScanWidget.h/.cpp ← 2D polar canvas (QPainter)
        ├── ControlPanel.h/.cpp← left sidebar
        └── ExportDialog.h/.cpp← export dialog
```

---

## Next steps (Phase 2+)

- [ ] **Heat-map mode**: accumulate multiple frames into a density map
- [ ] **Object detection overlay**: highlight clusters / nearest obstacle
- [ ] **SLAM preview**: integrate with hector_slam or cartographer output
- [ ] **Real MCAP writing**: use the official `mcap` C++ library for full ROS2 bags
- [ ] **Dark/light theme toggle**: extend the Fusion palette switcher
