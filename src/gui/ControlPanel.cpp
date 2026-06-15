#include "ControlPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QSerialPortInfo>
#include <QFont>
#include <QFrame>

ControlPanel::ControlPanel(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setSpacing(6);
    root->setContentsMargins(6,6,6,6);
    setFixedWidth(210);

    QFont mono("Courier", 9);
    QFont small;
    small.setPointSize(9);

    // ── Connection ─────────────────────────────────────────────────────────
    m_connGroup = new QGroupBox("Connection");
    auto* cL = new QVBoxLayout(m_connGroup);
    cL->setSpacing(4);

    auto* portRow = new QHBoxLayout;
    m_portCombo  = new QComboBox; m_portCombo->setFont(small);
    m_refreshBtn = new QPushButton("↻"); m_refreshBtn->setFixedWidth(26);
    portRow->addWidget(m_portCombo,1); portRow->addWidget(m_refreshBtn);

    m_connectBtn = new QPushButton("Connect");
    cL->addLayout(portRow);
    cL->addWidget(m_connectBtn);
    root->addWidget(m_connGroup);

    // ── View mode ──────────────────────────────────────────────────────────
    m_viewGroup = new QGroupBox("View mode");
    auto* vL = new QHBoxLayout(m_viewGroup);
    vL->setSpacing(3);
    m_btnPolar     = new QPushButton("Polar");
    m_btnCartesian = new QPushButton("XY");
    m_btnHeatmap   = new QPushButton("Heat");
    for (auto* b : {m_btnPolar, m_btnCartesian, m_btnHeatmap}) {
        b->setCheckable(true); b->setFont(small); vL->addWidget(b);
    }
    m_btnPolar->setChecked(true);
    root->addWidget(m_viewGroup);

    // ── Scan settings ──────────────────────────────────────────────────────
    m_scanGroup = new QGroupBox("Scan settings");
    auto* sF = new QFormLayout(m_scanGroup);
    sF->setSpacing(4);

    m_rpmSpin = new QSpinBox;
    m_rpmSpin->setRange(100,1023); m_rpmSpin->setValue(660);
    m_rpmSpin->setSuffix(" RPM"); m_rpmSpin->setFont(small);
    sF->addRow("Motor:", m_rpmSpin);

    auto makeSliderRow = [&](QSlider*& sl, QLabel*& lbl, int min, int max, int val) {
        sl = new QSlider(Qt::Horizontal);
        sl->setRange(min,max); sl->setValue(val);
        lbl = new QLabel(QString::number(val));
        lbl->setFont(mono); lbl->setMinimumWidth(36);
        auto* row = new QHBoxLayout;
        row->addWidget(sl,1); row->addWidget(lbl);
        return row;
    };

    auto* qRow = makeSliderRow(m_qualitySlider, m_qualityLabel, 0, 100, 10);
    sF->addRow("Quality:", qRow);

    auto* rRow = makeSliderRow(m_rangeSlider, m_rangeLabel, 0, 20, 0);
    m_rangeLabel->setText("Auto");
    sF->addRow("Max range:", rRow);

    auto* mrRow = makeSliderRow(m_minRangeSlider, m_minRangeLabel, 0, 500, 50);
    m_minRangeLabel->setText("50mm");
    sF->addRow("Min range:", mrRow);

    root->addWidget(m_scanGroup);

    // ── Display ────────────────────────────────────────────────────────────
    m_displayGroup = new QGroupBox("Display");
    auto* dL = new QVBoxLayout(m_displayGroup);
    dL->setSpacing(2);

    auto makeCheck = [&](const QString& label, bool checked) {
        auto* cb = new QCheckBox(label);
        cb->setChecked(checked); cb->setFont(small);
        dL->addWidget(cb); return cb;
    };
    m_gridCheck      = makeCheck("Range grid",          true);
    m_colorMapCheck  = makeCheck("Distance colour map", true);
    m_trailCheck     = makeCheck("Point trails",        true);
    m_nearestCheck   = makeCheck("Nearest obstacle",    true);
    m_accumulateCheck= makeCheck("Accumulate frames",   false);
    m_invalidCheck   = makeCheck("Show invalid points", false);

    root->addWidget(m_displayGroup);

    // ── Recording ──────────────────────────────────────────────────────────
    m_recordGroup = new QGroupBox("Recording");
    auto* rL = new QVBoxLayout(m_recordGroup);
    rL->setSpacing(4);
    m_recordBtn   = new QPushButton("● Start recording");
    m_snapshotBtn = new QPushButton("📷  Save snapshot");
    m_recordBtn->setFont(small); m_snapshotBtn->setFont(small);
    rL->addWidget(m_recordBtn);
    rL->addWidget(m_snapshotBtn);
    root->addWidget(m_recordGroup);

    // ── Live stats ─────────────────────────────────────────────────────────
    m_statsGroup = new QGroupBox("Live stats");
    auto* stF = new QFormLayout(m_statsGroup);
    stF->setSpacing(2);
    m_statPoints  = new QLabel("—"); m_statPoints->setFont(mono);
    m_statMin     = new QLabel("—"); m_statMin->setFont(mono);
    m_statMax     = new QLabel("—"); m_statMax->setFont(mono);
    m_statRate    = new QLabel("—"); m_statRate->setFont(mono);
    m_statNearest = new QLabel("—"); m_statNearest->setFont(mono);
    m_statNearest->setStyleSheet("color:#e06060");
    stF->addRow("Points:",   m_statPoints);
    stF->addRow("Min dist:", m_statMin);
    stF->addRow("Max dist:", m_statMax);
    stF->addRow("Scan rate:",m_statRate);
    stF->addRow("Nearest:",  m_statNearest);
    root->addWidget(m_statsGroup);

    root->addStretch();

    // ── Initial state ──────────────────────────────────────────────────────
    setConnected(false);
    refreshPorts();

    // ── Signal connections ─────────────────────────────────────────────────
    connect(m_refreshBtn, &QPushButton::clicked, this, &ControlPanel::refreshPorts);
    connect(m_connectBtn, &QPushButton::clicked, this, &ControlPanel::onConnectClicked);

    connect(m_rpmSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int v){ emit motorSpeedChanged(v); });

    connect(m_qualitySlider, &QSlider::valueChanged, this, [this](int v){
        m_qualityLabel->setText(QString::number(v));
        emit minQualityChanged(v);
    });

    connect(m_rangeSlider, &QSlider::valueChanged, this, [this](int v){
        if (v==0) { m_rangeLabel->setText("Auto"); emit maxRangeChanged(0.f); }
        else      { m_rangeLabel->setText(QString("%1m").arg(v)); emit maxRangeChanged(v*1000.f); }
    });

    connect(m_minRangeSlider, &QSlider::valueChanged, this, [this](int v){
        m_minRangeLabel->setText(QString("%1mm").arg(v));
        emit minRangeChanged(float(v));
    });

    connect(m_gridCheck,       &QCheckBox::toggled, this, &ControlPanel::showGridChanged);
    connect(m_invalidCheck,    &QCheckBox::toggled, this, &ControlPanel::showInvalidChanged);
    connect(m_trailCheck,      &QCheckBox::toggled, this, &ControlPanel::showTrailsChanged);
    connect(m_nearestCheck,    &QCheckBox::toggled, this, &ControlPanel::showNearestChanged);
    connect(m_accumulateCheck, &QCheckBox::toggled, this, &ControlPanel::accumulateChanged);
    connect(m_colorMapCheck,   &QCheckBox::toggled, this, &ControlPanel::colorMapChanged);

    // View mode buttons
    connect(m_btnPolar, &QPushButton::clicked, this, [this]{
        m_btnPolar->setChecked(true); m_btnCartesian->setChecked(false); m_btnHeatmap->setChecked(false);
        emit viewModeChanged(0);
    });
    connect(m_btnCartesian, &QPushButton::clicked, this, [this]{
        m_btnPolar->setChecked(false); m_btnCartesian->setChecked(true); m_btnHeatmap->setChecked(false);
        emit viewModeChanged(1);
    });
    connect(m_btnHeatmap, &QPushButton::clicked, this, [this]{
        m_btnPolar->setChecked(false); m_btnCartesian->setChecked(false); m_btnHeatmap->setChecked(true);
        emit viewModeChanged(2);
    });

    connect(m_recordBtn, &QPushButton::clicked, this, [this]{
        m_isRecording = !m_isRecording;
        m_recordBtn->setText(m_isRecording ? "■ Stop recording" : "● Start recording");
        emit recordingToggled(m_isRecording);
    });
    connect(m_snapshotBtn, &QPushButton::clicked, this, &ControlPanel::snapshotRequested);
}

void ControlPanel::setConnected(bool connected) {
    m_isConnected = connected;
    m_connectBtn->setText(connected ? "Disconnect" : "Connect");
    m_portCombo->setEnabled(!connected);
    m_refreshBtn->setEnabled(!connected);
    m_rpmSpin->setEnabled(connected);
    m_recordBtn->setEnabled(connected);
    m_snapshotBtn->setEnabled(connected);
}

void ControlPanel::updateStats(int pts, float minD, float maxD,
                                float rate, float nearD, float nearA) {
    m_statPoints->setText(QString::number(pts));
    m_statMin->setText(QString("%1 mm").arg(int(minD)));
    m_statMax->setText(QString("%1 mm").arg(int(maxD)));
    m_statRate->setText(QString("%1 Hz").arg(rate,0,'f',1));
    if (nearD > 0)
        m_statNearest->setText(QString("%1mm @ %2°").arg(int(nearD)).arg(nearA,0,'f',0));
}

void ControlPanel::refreshPorts() {
    m_portCombo->clear();
    for (const auto& info : QSerialPortInfo::availablePorts())
        m_portCombo->addItem(info.portName() + " — " + info.description());
    if (m_portCombo->count() == 0)
        m_portCombo->addItem("(no ports found)");
}

void ControlPanel::onConnectClicked() {
    if (m_isConnected)
        emit disconnectRequested();
    else {
        // Extract just the port name before " — "
        QString full = m_portCombo->currentText();
        QString port = full.split(" — ").first().trimmed();
        emit connectRequested(port);
    }
}
