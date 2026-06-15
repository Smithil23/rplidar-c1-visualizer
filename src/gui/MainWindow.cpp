#include "MainWindow.h"
#include "ScanWidget.h"
#include "ControlPanel.h"
#include "StatsPanel.h"
#include "ExportDialog.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <QStatusBar>
#include <QCloseEvent>
#include <QMessageBox>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QDebug>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_buffer(std::make_shared<ScanBuffer>(128))
{
    setWindowTitle("RPLidar C1 Research Visualizer");
    resize(1200, 750);
    setupUi();
    setupStatusBar();

    m_renderTimer = new QTimer(this);
    m_renderTimer->setInterval(33);
    connect(m_renderTimer, &QTimer::timeout, this, &MainWindow::onRenderTimer);
    m_renderTimer->start();
}

MainWindow::~MainWindow() { stopDriver(); }

void MainWindow::setupUi() {
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* layout = new QHBoxLayout(central);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);

    m_controlPanel = new ControlPanel(this);
    m_scanWidget   = new ScanWidget(this);
    m_statsPanel   = new StatsPanel(this);

    auto makeLine = [](QWidget* p) {
        auto* f = new QFrame(p);
        f->setFrameShape(QFrame::VLine);
        f->setFrameShadow(QFrame::Plain);
        f->setStyleSheet("color:#1a2a3a");
        return f;
    };

    layout->addWidget(m_controlPanel);
    layout->addWidget(makeLine(central));
    layout->addWidget(m_scanWidget, 1);
    layout->addWidget(makeLine(central));
    layout->addWidget(m_statsPanel);

    connect(m_controlPanel, &ControlPanel::connectRequested,    this, &MainWindow::onConnectRequested);
    connect(m_controlPanel, &ControlPanel::disconnectRequested, this, &MainWindow::onDisconnectRequested);
    connect(m_controlPanel, &ControlPanel::motorSpeedChanged, this, [this](int rpm){
        if (m_driver) m_driver->setMotorSpeed(rpm);
    });
    connect(m_controlPanel, &ControlPanel::minQualityChanged, m_scanWidget, &ScanWidget::setMinQuality);
    connect(m_controlPanel, &ControlPanel::maxRangeChanged,   m_scanWidget, &ScanWidget::setMaxRange_mm);
    connect(m_controlPanel, &ControlPanel::minRangeChanged,   m_scanWidget, &ScanWidget::setMinRange_mm);
    connect(m_controlPanel, &ControlPanel::showGridChanged,    m_scanWidget, &ScanWidget::setShowGrid);
    connect(m_controlPanel, &ControlPanel::showInvalidChanged, m_scanWidget, &ScanWidget::setShowInvalid);
    connect(m_controlPanel, &ControlPanel::showTrailsChanged,  m_scanWidget, &ScanWidget::setShowTrails);
    connect(m_controlPanel, &ControlPanel::showNearestChanged, m_scanWidget, &ScanWidget::setShowNearest);
    connect(m_controlPanel, &ControlPanel::accumulateChanged,  m_scanWidget, &ScanWidget::setAccumulate);
    connect(m_controlPanel, &ControlPanel::colorMapChanged,    m_scanWidget, &ScanWidget::setColorMap);
    connect(m_controlPanel, &ControlPanel::viewModeChanged, this, [this](int m){
        m_scanWidget->setViewMode(static_cast<ScanWidget::ViewMode>(m));
    });
    connect(m_controlPanel, &ControlPanel::recordingToggled,   this, &MainWindow::onRecordingToggled);
    connect(m_controlPanel, &ControlPanel::snapshotRequested,  this, &MainWindow::onSnapshotRequested);

    connect(m_statsPanel, &StatsPanel::exportRequested, this, &MainWindow::onExportRequested);
    connect(m_scanWidget, &ScanWidget::nearestChanged,  this, &MainWindow::onNearestChanged);
}

void MainWindow::setupStatusBar() {
    m_sbPort   = new QLabel("  Not connected  ");
    m_sbStatus = new QLabel("  Ready  ");
    m_sbRate   = new QLabel("  — Hz  ");
    statusBar()->addPermanentWidget(m_sbPort);
    statusBar()->addPermanentWidget(m_sbStatus, 1);
    statusBar()->addPermanentWidget(m_sbRate);
    statusBar()->showMessage("Connect the RPLidar C1 and press Connect.  |  Double-click canvas to reset view.");
}

void MainWindow::onConnectRequested(const QString& port) {
    if (m_driver) return;
    m_buffer->clear();
    m_recordedFrames.clear();

    m_driver       = new LidarDriver(m_buffer);
    m_driverThread = new QThread(this);
    m_driver->setPort(port);
    m_driver->moveToThread(m_driverThread);

    connect(m_driverThread, &QThread::started,   m_driver, &LidarDriver::run);
    connect(m_driver, &LidarDriver::connected,    this, &MainWindow::onDriverConnected);
    connect(m_driver, &LidarDriver::disconnected, this, &MainWindow::onDriverDisconnected);
    connect(m_driver, &LidarDriver::scanReady,    this, &MainWindow::onScanReady);
    connect(m_driver, &LidarDriver::statusUpdate, this, &MainWindow::onStatusUpdate);

    m_driverThread->start();
    statusBar()->showMessage(QString("Connecting to %1…").arg(port));
}

void MainWindow::onDisconnectRequested() { stopDriver(); }

void MainWindow::stopDriver() {
    if (!m_driver) return;
    m_driver->requestStop();
    m_driverThread->quit();
    m_driverThread->wait(3000);
    delete m_driver;       m_driver = nullptr;
    delete m_driverThread; m_driverThread = nullptr;
    m_controlPanel->setConnected(false);
    m_sbPort->setText("  Not connected  ");
}

void MainWindow::onDriverConnected(const QString& port, int fwMaj, int fwMin) {
    m_sbPort->setText(QString("  %1  ").arg(port));
    m_sbStatus->setText(QString("  FW %1.%2  ").arg(fwMaj).arg(fwMin));
    m_controlPanel->setConnected(true);
    statusBar()->showMessage(QString("Connected on %1 · FW %2.%3  |  Scroll=zoom · Drag=pan · Double-click=reset")
        .arg(port).arg(fwMaj).arg(fwMin));
}

void MainWindow::onDriverDisconnected(const QString& reason) {
    statusBar()->showMessage(reason);
    m_controlPanel->setConnected(false);
    m_sbPort->setText("  Not connected  ");
    if (m_driverThread && !m_driverThread->isRunning()) {
        delete m_driver;       m_driver = nullptr;
        delete m_driverThread; m_driverThread = nullptr;
    }
}

void MainWindow::onScanReady(int pts, float minD, float maxD) {
    m_lastPts = pts; m_lastMinDist = minD; m_lastMaxDist = maxD;
    m_controlPanel->updateStats(pts, minD, maxD, m_lastRate, m_lastNearD, m_lastNearA);
    m_statsPanel->updateStats(pts, minD, maxD, m_lastRate, m_lastNearD, m_lastNearA);
}

void MainWindow::onStatusUpdate(float rate) {
    m_lastRate = rate;
    m_sbRate->setText(QString("  %1 Hz  ").arg(rate,0,'f',1));
}

void MainWindow::onNearestChanged(float dist, float angle) {
    m_lastNearD = dist; m_lastNearA = angle;
}

void MainWindow::onRenderTimer() {
    auto frame = m_buffer->latest();
    if (!frame) return;
    m_scanWidget->setFrame(*frame);
    if (m_recording) m_recordedFrames.push_back(*frame);
}

void MainWindow::onRecordingToggled(bool recording) {
    m_recording = recording;
    if (recording) {
        m_recordedFrames.clear();
        statusBar()->showMessage("Recording…  press Stop to finish.");
    } else {
        statusBar()->showMessage(QString("Recording stopped — %1 frames captured. Click an export button to save.")
            .arg(m_recordedFrames.size()));
    }
}

void MainWindow::onSnapshotRequested() {
    QString f = QDir::homePath() + "/rplidar_snap_" +
        QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".png";
    m_scanWidget->saveSnapshot(f);
    statusBar()->showMessage("Snapshot saved: " + f);
}

void MainWindow::onExportRequested(int fmt) {
    // Get frames — recorded first, otherwise grab current buffer
    std::vector<ScanFrame> frames = m_recordedFrames;
    if (frames.empty()) {
        frames = m_buffer->drainAll();
    }
    if (frames.empty()) {
        QMessageBox::information(this, "No data",
            "No scan frames to export.\n\n"
            "1. Make sure the lidar is connected and scanning.\n"
            "2. Click 'Start recording', wait 5 seconds.\n"
            "3. Click 'Stop recording'.\n"
            "4. Then click an export button.");
        return;
    }

    auto format = static_cast<DataExporter::Format>(fmt);
    QString ext = DataExporter::extension(format);
    QString suggested = QDir::homePath() + "/" + DataExporter::suggestFilename(format);

    // Show save dialog
    QString filter = QString("%1 files (*.%2);;All files (*)").arg(ext.toUpper(), ext);
    QString path = QFileDialog::getSaveFileName(this,
        QString("Export %1 frames as %2").arg(frames.size()).arg(ext.toUpper()),
        suggested, filter);

    if (path.isEmpty()) return; // user cancelled

    statusBar()->showMessage(QString("Exporting %1 frames to %2…").arg(frames.size()).arg(path));

    bool ok = m_exporter.exportFrames(frames, path, format);
    if (ok) {
        statusBar()->showMessage(QString("Exported %1 frames to: %2").arg(frames.size()).arg(path));
        QMessageBox::information(this, "Export complete",
            QString("Successfully exported %1 scan frames to:\n%2").arg(frames.size()).arg(path));
    } else {
        QMessageBox::critical(this, "Export failed", m_exporter.lastError());
    }
}

void MainWindow::closeEvent(QCloseEvent* e) { stopDriver(); e->accept(); }
