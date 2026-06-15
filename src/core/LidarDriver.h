#pragma once
#include "ScanPoint.h"
#include "ScanBuffer.h"

#include <QObject>
#include <QString>
#include <QThread>
#include <atomic>
#include <memory>

// LidarDriver wraps the Slamtec RPLidar C++ SDK.
// It lives in its own QThread so that serial I/O never blocks the GUI.
//
// Typical usage:
//   auto* driver = new LidarDriver(buffer);
//   auto* thread = new QThread;
//   driver->moveToThread(thread);
//   connect(thread, &QThread::started, driver, &LidarDriver::run);
//   thread->start();
//   ...
//   driver->requestStop();   // graceful shutdown
//   thread->quit(); thread->wait();
class LidarDriver : public QObject {
    Q_OBJECT

public:
    explicit LidarDriver(std::shared_ptr<ScanBuffer> buffer,
                         QObject* parent = nullptr);
    ~LidarDriver() override;

public slots:
    // Entry point for the worker thread.
    void run();

    // Request a graceful stop from any thread.
    void requestStop();

    // Change the scan motor speed (RPM) while running.
    void setMotorSpeed(int rpm);

signals:
    // Emitted when the lidar connects successfully.
    void connected(const QString& portName, int firmwareMajor, int firmwareFirmMinor);

    // Emitted when the lidar disconnects or an error occurs.
    void disconnected(const QString& reason);

    // Emitted after each complete 360° sweep with basic stats.
    void scanReady(int pointCount, float minDist_mm, float maxDist_mm);

    // Emitted periodically with the current scan rate.
    void statusUpdate(float scansPerSec);

public:
    // Serial port to use (e.g. "COM3" on Windows, "/dev/ttyUSB0" on Linux).
    // Set before calling run(). Defaults to auto-detect.
    void setPort(const QString& port) { m_port = port; }
    QString port() const              { return m_port; }

    bool isRunning() const { return m_running.load(); }

private:
    void connectToDevice();
    void scanLoop();
    void disconnect();

    std::shared_ptr<ScanBuffer> m_buffer;
    QString                     m_port;
    std::atomic<bool>           m_running{false};
    std::atomic<bool>           m_stopRequested{false};
    std::atomic<int>            m_motorRpm{660};   // C1 default

    // Opaque pointer to the SDK driver — avoids pulling SDK headers
    // into every TU that includes LidarDriver.h.
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
