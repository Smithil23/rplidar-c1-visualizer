#pragma once
#include "core/ScanBuffer.h"
#include "core/LidarDriver.h"
#include "core/DataExporter.h"
#include <QMainWindow>
#include <QThread>
#include <QTimer>
#include <QLabel>
#include <memory>

class ScanWidget;
class ControlPanel;
class StatsPanel;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent*) override;

private slots:
    void onConnectRequested(const QString& port);
    void onDisconnectRequested();
    void onDriverConnected(const QString& port, int fwMaj, int fwMin);
    void onDriverDisconnected(const QString& reason);
    void onScanReady(int pts, float minD, float maxD);
    void onStatusUpdate(float scansPerSec);
    void onRenderTimer();
    void onRecordingToggled(bool recording);
    void onSnapshotRequested();
    void onExportRequested(int format);
    void onNearestChanged(float dist, float angle);

private:
    void setupUi();
    void setupStatusBar();
    void stopDriver();

    std::shared_ptr<ScanBuffer> m_buffer;
    LidarDriver*   m_driver       = nullptr;
    QThread*       m_driverThread = nullptr;
    DataExporter   m_exporter;

    bool                   m_recording = false;
    std::vector<ScanFrame> m_recordedFrames;

    // Live stats carried across signals
    int   m_lastPts     = 0;
    float m_lastMinDist = 0, m_lastMaxDist = 0;
    float m_lastRate    = 0;
    float m_lastNearD   = 0, m_lastNearA = 0;

    ScanWidget*   m_scanWidget   = nullptr;
    ControlPanel* m_controlPanel = nullptr;
    StatsPanel*   m_statsPanel   = nullptr;
    QTimer*       m_renderTimer  = nullptr;

    QLabel* m_sbPort   = nullptr;
    QLabel* m_sbStatus = nullptr;
    QLabel* m_sbRate   = nullptr;
};
