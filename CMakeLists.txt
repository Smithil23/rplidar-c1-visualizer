cmake_minimum_required(VERSION 3.20)
project(RPLidarGUI VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ── Qt6 ──────────────────────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets SerialPort)

# ── RPLidar SDK ──────────────────────────────────────────────────────────────
# The Slamtec SDK lives in third_party/rplidar_sdk/
# Clone it with:
#   git clone https://github.com/Slamtec/rplidar_sdk.git third_party/rplidar_sdk
set(RPLIDAR_SDK_DIR "${CMAKE_SOURCE_DIR}/third_party/rplidar_sdk")
set(RPLIDAR_SDK_SRC "${RPLIDAR_SDK_DIR}/sdk/src")
set(RPLIDAR_SDK_INC "${RPLIDAR_SDK_DIR}/sdk/include")

# Collect all SDK platform-agnostic sources
file(GLOB_RECURSE RPLIDAR_SRCS
    "${RPLIDAR_SDK_SRC}/*.cpp"
    "${RPLIDAR_SDK_SRC}/*.c"
)

# On Windows exclude the arch/ unix folder and vice versa
if(WIN32)
    list(FILTER RPLIDAR_SRCS EXCLUDE REGEX ".*arch/linux.*")
    list(FILTER RPLIDAR_SRCS EXCLUDE REGEX ".*arch/macOS.*")
elseif(APPLE)
    list(FILTER RPLIDAR_SRCS EXCLUDE REGEX ".*arch/win32.*")
    list(FILTER RPLIDAR_SRCS EXCLUDE REGEX ".*arch/linux.*")
else()
    list(FILTER RPLIDAR_SRCS EXCLUDE REGEX ".*arch/win32.*")
    list(FILTER RPLIDAR_SRCS EXCLUDE REGEX ".*arch/macOS.*")
endif()

add_library(rplidar_sdk STATIC ${RPLIDAR_SRCS})
target_include_directories(rplidar_sdk PUBLIC
    "${RPLIDAR_SDK_INC}"
    "${RPLIDAR_SDK_SRC}"
)
if(WIN32)
    target_compile_definitions(rplidar_sdk PUBLIC _WIN32)
endif()

# ── nlohmann/json (single-header, bundled in third_party/) ───────────────────
# Download with:
#   curl -L https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp \
#        -o third_party/json.hpp
set(JSON_INC "${CMAKE_SOURCE_DIR}/third_party")

# ── Application sources ───────────────────────────────────────────────────────
set(APP_SOURCES
    src/main.cpp

    # Core (non-GUI) layer
    src/core/ScanPoint.h
    src/core/ScanBuffer.h
    src/core/ScanBuffer.cpp
    src/core/LidarDriver.h
    src/core/LidarDriver.cpp
    src/core/DataExporter.h
    src/core/DataExporter.cpp

    # GUI layer
    src/gui/MainWindow.h
    src/gui/MainWindow.cpp
    src/gui/ScanWidget.h
    src/gui/ScanWidget.cpp
    src/gui/ControlPanel.h
    src/gui/ControlPanel.cpp
    src/gui/ExportDialog.h
    src/gui/ExportDialog.cpp
    src/gui/StatsPanel.h
    src/gui/StatsPanel.cpp
)

add_executable(RPLidarGUI ${APP_SOURCES})

target_include_directories(RPLidarGUI PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${RPLIDAR_SDK_INC}"
    "${JSON_INC}"
)

target_link_libraries(RPLidarGUI PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::SerialPort
    rplidar_sdk
)

# Windows: copy Qt DLLs next to the .exe automatically
if(WIN32)
    add_custom_command(TARGET RPLidarGUI POST_BUILD
        COMMAND "${Qt6_DIR}/../../../bin/windeployqt.exe"
                "$<TARGET_FILE:RPLidarGUI>"
        COMMENT "Running windeployqt..."
    )
endif()
