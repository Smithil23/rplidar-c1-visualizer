#include "StatsPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QPainter>
#include <QFont>
#include <QFrame>

// ── Mini chart widget ─────────────────────────────────────────────────────────
class MiniChart : public QWidget {
public:
    std::deque<float>* data = nullptr;
    QString label;
    QColor  barColor = QColor(55,138,221);
    explicit MiniChart(QWidget* p=nullptr) : QWidget(p) { setMinimumHeight(50); }
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.fillRect(rect(), QColor(15,20,30));
        if (!data || data->empty()) return;

        float mx = *std::max_element(data->begin(), data->end());
        if (mx <= 0) mx = 1;
        int n = data->size();
        float bw = float(width()) / n;

        for (int i = 0; i < n; ++i) {
            float h = ((*data)[i] / mx) * (height() - 4);
            QColor c = barColor;
            c.setAlpha(i == n-1 ? 220 : 140);
            p.fillRect(int(i*bw)+1, int(height()-h-2), int(bw)-1, int(h), c);
        }

        p.setPen(QColor(80,120,180,120));
        p.drawRect(0,0,width()-1,height()-1);

        if (!label.isEmpty()) {
            p.setFont(QFont("Courier",8));
            p.setPen(QColor(80,120,180,160));
            p.drawText(4, 12, label);
        }
    }
};

// ── StatsPanel ────────────────────────────────────────────────────────────────
StatsPanel::StatsPanel(QWidget* parent) : QWidget(parent) {
    setFixedWidth(190);
    auto* root = new QVBoxLayout(this);
    root->setSpacing(6);
    root->setContentsMargins(6,6,6,6);

    QFont mono("Courier", 10);
    QFont small; small.setPointSize(9);
    QFont tiny;  tiny.setPointSize(8);

    // ── Metric cards ──────────────────────────────────────────────────────
    auto* metricsGroup = new QGroupBox("Live metrics");
    auto* mGrid = new QGridLayout(metricsGroup);
    mGrid->setSpacing(4);

    auto makeCard = [&](const QString& label) -> QLabel* {
        auto* card = new QWidget;
        card->setStyleSheet("background:#0d1520;border:1px solid #1a2a40;border-radius:4px");
        auto* vl = new QVBoxLayout(card);
        vl->setContentsMargins(6,4,6,4); vl->setSpacing(1);
        auto* lbl = new QLabel(label); lbl->setFont(tiny);
        lbl->setStyleSheet("color:#4a7aaa;border:none");
        auto* val = new QLabel("—"); val->setFont(mono);
        val->setStyleSheet("color:#7ec8e3;border:none");
        vl->addWidget(lbl); vl->addWidget(val);
        return val;
    };

    // We need to add cards to layout — let's do it differently
    struct Card { QString lbl; QLabel** val; };
    // Create cards manually
    auto makeCardWidget = [&](const QString& lbl, QLabel*& valOut) -> QWidget* {
        auto* card = new QWidget;
        card->setStyleSheet("background:#0d1520;border:1px solid #1a2a40;border-radius:4px");
        auto* vl = new QVBoxLayout(card);
        vl->setContentsMargins(5,3,5,3); vl->setSpacing(1);
        auto* l = new QLabel(lbl); l->setFont(tiny);
        l->setStyleSheet("color:#4a7aaa;border:none;background:transparent");
        valOut = new QLabel("—"); valOut->setFont(mono);
        valOut->setStyleSheet("color:#7ec8e3;border:none;background:transparent");
        vl->addWidget(l); vl->addWidget(valOut);
        return card;
    };

    mGrid->addWidget(makeCardWidget("POINTS", m_valPts),    0,0);
    mGrid->addWidget(makeCardWidget("SCAN Hz", m_valRate),  0,1);
    mGrid->addWidget(makeCardWidget("MIN mm", m_valMin),    1,0);
    mGrid->addWidget(makeCardWidget("MAX mm", m_valMax),    1,1);

    // Nearest obstacle full width
    auto* nearCard = new QWidget;
    nearCard->setStyleSheet("background:#1a0a0a;border:1px solid #402020;border-radius:4px");
    auto* nL = new QVBoxLayout(nearCard);
    nL->setContentsMargins(5,3,5,3); nL->setSpacing(1);
    auto* nLbl = new QLabel("NEAREST OBSTACLE"); nLbl->setFont(tiny);
    nLbl->setStyleSheet("color:#aa4444;border:none;background:transparent");
    m_valNearest = new QLabel("—"); m_valNearest->setFont(mono);
    m_valNearest->setStyleSheet("color:#e06060;border:none;background:transparent");
    nL->addWidget(nLbl); nL->addWidget(m_valNearest);
    mGrid->addWidget(nearCard, 2, 0, 1, 2);

    root->addWidget(metricsGroup);

    // ── Scan rate history chart ────────────────────────────────────────────
    auto* chartGroup = new QGroupBox("Scan rate history");
    auto* chartL = new QVBoxLayout(chartGroup);
    chartL->setContentsMargins(4,4,4,4);
    m_chartWidget = new MiniChart;
    static_cast<MiniChart*>(m_chartWidget)->data = &m_rateHistory;
    static_cast<MiniChart*>(m_chartWidget)->label = "Hz";
    chartL->addWidget(m_chartWidget);
    root->addWidget(chartGroup);

    // ── Export buttons ─────────────────────────────────────────────────────
    auto* exportGroup = new QGroupBox("Export data");
    auto* eL = new QVBoxLayout(exportGroup);
    eL->setSpacing(3);

    auto makeExportBtn = [&](const QString& label, const QString& color) -> QPushButton* {
        auto* btn = new QPushButton(label);
        btn->setFont(small);
        btn->setStyleSheet(QString("QPushButton{border:1px solid %1;color:%1;"
            "border-radius:4px;padding:3px;background:transparent}"
            "QPushButton:hover{background:%1;color:#0a0f14}").arg(color));
        eL->addWidget(btn);
        return btn;
    };

    m_btnCSV  = makeExportBtn("CSV  — Excel / MATLAB", "#1d9e75");
    m_btnPCD  = makeExportBtn("PCD  — PCL / ROS",      "#378add");
    m_btnPLY  = makeExportBtn("PLY  — MeshLab",        "#8b5cf6");
    m_btnJSON = makeExportBtn("JSON — Web / REST",      "#ba7517");
    m_btnROS  = makeExportBtn("MCAP — ROS bag",         "#993556");

    root->addWidget(exportGroup);
    root->addStretch();

    // ── Connections ────────────────────────────────────────────────────────
    connect(m_btnCSV,  &QPushButton::clicked, this, [this]{ emit exportRequested(0); });
    connect(m_btnPCD,  &QPushButton::clicked, this, [this]{ emit exportRequested(1); });
    connect(m_btnPLY,  &QPushButton::clicked, this, [this]{ emit exportRequested(2); });
    connect(m_btnJSON, &QPushButton::clicked, this, [this]{ emit exportRequested(3); });
    connect(m_btnROS,  &QPushButton::clicked, this, [this]{ emit exportRequested(4); });
}

void StatsPanel::updateStats(int pts, float minD, float maxD,
                              float rate, float nearD, float nearA) {
    m_valPts->setText(QString::number(pts));
    m_valRate->setText(QString("%1").arg(rate,0,'f',1));
    m_valMin->setText(QString::number(int(minD)));
    m_valMax->setText(QString::number(int(maxD)));
    if (nearD > 0)
        m_valNearest->setText(QString("%1mm @ %2°").arg(int(nearD)).arg(nearA,0,'f',0));

    m_rateHistory.push_back(rate);
    while ((int)m_rateHistory.size() > 30) m_rateHistory.pop_front();
    m_chartWidget->update();
}

void StatsPanel::paintEvent(QPaintEvent* e) {
    QWidget::paintEvent(e);
}
