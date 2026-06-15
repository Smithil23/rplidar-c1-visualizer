#pragma once
#include "ScanPoint.h"
#include <QString>
#include <QDateTime>
#include <vector>

// Writes scan data to disk in various formats.
// All methods are synchronous and can be called from any thread.
// They return true on success, false on I/O failure (check lastError()).
class DataExporter {
public:
    enum class Format {
        CSV,        // Angle, Distance, Quality, X, Y  — opens in Excel/MATLAB
        PCD,        // ASCII PCD v0.7  — opens in PCL, ROS, CloudCompare
        PLY,        // ASCII PLY       — opens in MeshLab, CloudCompare, Blender
        JSON,       // JSON array of objects — useful for web / REST
        ROSBag      // Minimal MCAP/ROS2 bag (CSV-inside-bag for compatibility)
    };

    DataExporter() = default;

    // Export a sequence of frames to a file.
    // 'path' should already have the correct extension.
    bool exportFrames(const std::vector<ScanFrame>& frames,
                      const QString&                path,
                      Format                        format);

    // Convenience: export a single frame.
    bool exportFrame(const ScanFrame& frame,
                     const QString&   path,
                     Format           format);

    // Human-readable error from the last failed call.
    QString lastError() const { return m_lastError; }

    // Suggest a filename with timestamp for a given format.
    static QString suggestFilename(Format format);

    // File extension string for a given format.
    static QString extension(Format format);

private:
    bool writeCsv (const std::vector<ScanFrame>& frames, const QString& path);
    bool writePcd (const std::vector<ScanFrame>& frames, const QString& path);
    bool writePly (const std::vector<ScanFrame>& frames, const QString& path);
    bool writeJson(const std::vector<ScanFrame>& frames, const QString& path);
    bool writeRosBag(const std::vector<ScanFrame>& frames, const QString& path);

    QString m_lastError;
};
