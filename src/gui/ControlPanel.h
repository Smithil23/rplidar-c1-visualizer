#pragma once
#include <QWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QGroupBox>
#include <QButtonGroup>

class ControlPanel : public QWidget {
    Q_OBJECT
public:
    explicit ControlPanel(QWidget* parent = nullptr);
    void setConnected(bool connected);
    void updateStats(int pointCount, float minDist, float maxDist,
                     float scansPerSec, float nearestDist, float nearestAngle);

signals:
    void connectRequested(const QString& port);
    void disconnectRequested();
    void motorSpeedChanged(int rpm);
    void minQualityChanged(int q);
    void maxRangeChanged(float mm);
    void minRangeChanged(float mm);
    void showGridChanged(bool on);
    void showInvalidChanged(bool on);
    void showTrailsChanged(bool on);
    void showNearestChanged(bool on);
    void accumulateChanged(bool on);
    void colorMapChanged(bool on);
    void viewModeChanged(int mode);   // 0=Polar 1=Cartesian 2=Heatmap
    void recordingToggled(bool recording);
    void snapshotRequested();
    void exportRequested(int format); // 0=CSV 1=PCD 2=PLY 3=JSON 4=ROS
    void resetViewRequested();

private slots:
    void onConnectClicked();
    void refreshPorts();

private:
    // Connection
    QGroupBox*   m_connGroup;
    QComboBox*   m_portCombo;
    QPushButton* m_refreshBtn;
    QPushButton* m_connectBtn;

    // View mode
    QGroupBox*   m_viewGroup;
    QPushButton* m_btnPolar;
    QPushButton* m_btnCartesian;
    QPushButton* m_btnHeatmap;

    // Scan settings
    QGroupBox*   m_scanGroup;
    QSpinBox*    m_rpmSpin;
    QSlider*     m_qualitySlider;
    QLabel*      m_qualityLabel;
    QSlider*     m_rangeSlider;
    QLabel*      m_rangeLabel;
    QSlider*     m_minRangeSlider;
    QLabel*      m_minRangeLabel;

    // Display
    QGroupBox*   m_displayGroup;
    QCheckBox*   m_gridCheck;
    QCheckBox*   m_trailCheck;
    QCheckBox*   m_colorMapCheck;
    QCheckBox*   m_nearestCheck;
    QCheckBox*   m_accumulateCheck;
    QCheckBox*   m_invalidCheck;

    // Recording / export
    QGroupBox*   m_recordGroup;
    QPushButton* m_recordBtn;
    QPushButton* m_snapshotBtn;

    // Stats
    QGroupBox*   m_statsGroup;
    QLabel*      m_statPoints;
    QLabel*      m_statMin;
    QLabel*      m_statMax;
    QLabel*      m_statRate;
    QLabel*      m_statNearest;

    bool m_isConnected = false;
    bool m_isRecording = false;
};
