#include "LidarDriver.h"

#include "sl_lidar.h"
#include "sl_lidar_driver.h"

#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <chrono>
#include <thread>

using namespace sl;

static void logMsg(const QString& msg) {
    QFile f("C:/Users/smith/Desktop/rplidar_log.txt");
    if (f.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream s(&f);
        s << msg << "\n";
    }
    qDebug() << msg;
}

struct LidarDriver::Impl {
    ILidarDriver* driver  = nullptr;
    IChannel*     channel = nullptr;

    ~Impl() {
        if (driver) {
            driver->stop();
            driver->setMotorSpeed(0);
            delete driver;
        }
        if (channel) {
            delete channel;
        }
    }
};

LidarDriver::LidarDriver(std::shared_ptr<ScanBuffer> buffer, QObject* parent)
    : QObject(parent)
    , m_buffer(std::move(buffer))
    , m_impl(std::make_unique<Impl>())
{}

LidarDriver::~LidarDriver() = default;

void LidarDriver::requestStop() {
    m_stopRequested.store(true);
}

void LidarDriver::setMotorSpeed(int rpm) {
    m_motorRpm.store(rpm);
    if (m_impl->driver && m_running.load())
        m_impl->driver->setMotorSpeed(static_cast<sl_u16>(rpm));
}

void LidarDriver::run() {
    m_stopRequested.store(false);
    m_running.store(true);
    logMsg("=== LidarDriver::run() started ===");
    connectToDevice();
    if (!m_stopRequested.load())
        scanLoop();
    disconnect();
    m_running.store(false);
    emit disconnected("Scan stopped.");
}

void LidarDriver::connectToDevice() {
    logMsg("Creating lidar driver instance...");
    m_impl->driver = *createLidarDriver();
    if (!m_impl->driver) {
        QString err = "Failed to create lidar driver instance.";
        logMsg(err); emit disconnected(err);
        m_stopRequested.store(true); return;
    }
    logMsg("Driver instance OK.");

    std::string portStr = m_port.isEmpty() ? "COM3" : m_port.toStdString();

    // RPLidar C1 uses 460800 baud
    sl_u32 baudrate = 460800;
    logMsg(QString("Opening %1 at %2 baud...").arg(m_port).arg(baudrate));

    m_impl->channel = *createSerialPortChannel(portStr, baudrate);
    if (!m_impl->channel) {
        QString err = QString("Cannot open %1").arg(m_port);
        logMsg(err); emit disconnected(err);
        m_stopRequested.store(true); return;
    }
    logMsg("Channel OK.");

    auto result = m_impl->driver->connect(m_impl->channel);
    if (SL_IS_FAIL(result)) {
        QString err = QString("connect() failed 0x%1").arg(result, 8, 16, QChar('0'));
        logMsg(err); emit disconnected(err);
        m_stopRequested.store(true); return;
    }
    logMsg("connect() OK.");

    // Reset the device and wait for it to boot
    logMsg("Sending reset...");
    m_impl->driver->reset();
    logMsg("Waiting 2000ms for device to boot after reset...");
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    // Try getHealth up to 3 times
    sl_lidar_response_device_health_t health{};
    bool healthOk = false;
    for (int attempt = 1; attempt <= 3; ++attempt) {
        logMsg(QString("getHealth() attempt %1...").arg(attempt));
        result = m_impl->driver->getHealth(health);
        if (!SL_IS_FAIL(result)) {
            healthOk = true;
            logMsg(QString("getHealth() OK, status=%1").arg(health.status));
            break;
        }
        logMsg(QString("getHealth() failed 0x%1, retrying...").arg(result, 8, 16, QChar('0')));
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    if (!healthOk) {
        QString err = "getHealth() failed after 3 attempts. Device not responding.";
        logMsg(err); emit disconnected(err);
        m_stopRequested.store(true); return;
    }

    // Try getDeviceInfo up to 3 times
    sl_lidar_response_device_info_t devInfo{};
    bool infoOk = false;
    for (int attempt = 1; attempt <= 3; ++attempt) {
        logMsg(QString("getDeviceInfo() attempt %1...").arg(attempt));
        result = m_impl->driver->getDeviceInfo(devInfo);
        if (!SL_IS_FAIL(result)) {
            infoOk = true;
            break;
        }
        logMsg(QString("getDeviceInfo() failed 0x%1, retrying...").arg(result, 8, 16, QChar('0')));
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    if (!infoOk) {
        QString err = "getDeviceInfo() failed after 3 attempts.";
        logMsg(err); emit disconnected(err);
        m_stopRequested.store(true); return;
    }

    int fwMajor = devInfo.firmware_version >> 8;
    int fwMinor = devInfo.firmware_version & 0xFF;
    logMsg(QString("Device info OK. FW=%1.%2").arg(fwMajor).arg(fwMinor));
    emit connected(m_port, fwMajor, fwMinor);

    logMsg("Starting motor...");
    m_impl->driver->setMotorSpeed(static_cast<sl_u16>(m_motorRpm.load()));
    logMsg("Motor command sent.");
}

void LidarDriver::scanLoop() {
    logMsg("Starting scan...");
    auto result = m_impl->driver->startScan(false, true);
    if (SL_IS_FAIL(result)) {
        QString err = QString("startScan() failed 0x%1").arg(result, 8, 16, QChar('0'));
        logMsg(err); emit disconnected(err); return;
    }
    logMsg("Scan running.");

    sl_lidar_response_measurement_node_hq_t nodes[8192];
    auto tStart = std::chrono::steady_clock::now();
    int frameCount = 0;

    while (!m_stopRequested.load()) {
        std::size_t count = sizeof(nodes) / sizeof(nodes[0]);
        result = m_impl->driver->grabScanDataHq(nodes, count);
        if (SL_IS_FAIL(result)) {
            if (result == SL_RESULT_OPERATION_TIMEOUT) continue;
            QString err = QString("grabScanDataHq 0x%1").arg(result, 8, 16, QChar('0'));
            logMsg(err); emit disconnected(err); return;
        }

        m_impl->driver->ascendScanData(nodes, count);
        ScanFrame frame;
        frame.reserve(count);
        float minDist = 1e9f, maxDist = -1e9f;

        for (std::size_t i = 0; i < count; ++i) {
            const auto& n = nodes[i];
            ScanPoint pt;
            pt.angle_deg   = (n.angle_z_q14 >> 1) / 64.0f;
            pt.distance_mm = n.dist_mm_q2 / 4.0f;
            pt.quality     = static_cast<uint8_t>(n.quality >> 2);
            if (pt.isValid()) {
                minDist = std::min(minDist, pt.distance_mm);
                maxDist = std::max(maxDist, pt.distance_mm);
            }
            frame.push_back(pt);
        }

        m_buffer->push(std::move(frame));
        ++frameCount;

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration<float>(now - tStart).count();
        if (elapsed >= 1.0f) {
            emit statusUpdate(frameCount / elapsed);
            emit scanReady(static_cast<int>(count), minDist, maxDist);
            frameCount = 0;
            tStart = now;
        }
    }
    m_impl->driver->stop();
}

void LidarDriver::disconnect() {
    if (m_impl->driver) {
        m_impl->driver->stop();
        m_impl->driver->setMotorSpeed(0);
    }
}
