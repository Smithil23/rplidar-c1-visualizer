#pragma once
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <deque>

// Right-side panel: live metric cards, mini history chart, export buttons.
class StatsPanel : public QWidget {
    Q_OBJECT
public:
    explicit StatsPanel(QWidget* parent = nullptr);

    void updateStats(int pts, float minD, float maxD,
                     float rate, float nearD, float nearA);

signals:
    void exportRequested(int format); // 0=CSV 1=PCD 2=PLY 3=JSON 4=ROS

protected:
    void paintEvent(QPaintEvent*) override;

private:
    // Metric cards
    QLabel* m_valPts;
    QLabel* m_valRate;
    QLabel* m_valMin;
    QLabel* m_valMax;
    QLabel* m_valNearest;

    // History for mini chart (scan rate)
    std::deque<float> m_rateHistory;
    static constexpr int HISTORY = 30;

    // Export buttons
    QPushButton* m_btnCSV;
    QPushButton* m_btnPCD;
    QPushButton* m_btnPLY;
    QPushButton* m_btnJSON;
    QPushButton* m_btnROS;

    // Mini chart widget (nested)
    QWidget* m_chartWidget;
};
