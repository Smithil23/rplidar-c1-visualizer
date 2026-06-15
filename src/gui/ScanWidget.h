#pragma once
#include "core/ScanPoint.h"
#include <QWidget>
#include <QTimer>
#include <QColor>
#include <deque>
#include <optional>

class ScanWidget : public QWidget {
    Q_OBJECT
public:
    enum class ViewMode { Polar, Cartesian, Heatmap };

    explicit ScanWidget(QWidget* parent = nullptr);
    QSize sizeHint() const override { return {700, 600}; }

    // Nearest obstacle info (for status display)
    struct NearestInfo { float dist_mm; float angle_deg; bool valid; };
    NearestInfo nearestObstacle() const { return m_nearest; }

public slots:
    void setFrame(const ScanFrame& frame);
    void setViewMode(ViewMode m)       { m_viewMode = m; update(); }
    void setMaxRange_mm(float mm)      { m_maxRange_mm = mm; update(); }
    void setMinRange_mm(float mm)      { m_minRange_mm = mm; update(); }
    void setMinQuality(int q)          { m_minQuality = q; update(); }
    void setShowGrid(bool on)          { m_showGrid = on; update(); }
    void setShowInvalid(bool on)       { m_showInvalid = on; update(); }
    void setShowTrails(bool on)        { m_showTrails = on; update(); }
    void setShowNearest(bool on)       { m_showNearest = on; update(); }
    void setAccumulate(bool on)        { m_accumulate = on; if(!on) m_trail.clear(); update(); }
    void setColorMap(bool on)          { m_colorMap = on; update(); }
    void saveSnapshot(const QString& path);
    void resetView()                   { m_scale=1.f; m_panOffset={0,0}; update(); }

signals:
    void nearestChanged(float dist_mm, float angle_deg);
    void statsChanged(int points, float minD, float maxD);

protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;

private:
    QPointF toPixel(const ScanPoint& pt) const;
    QColor  pointColor(const ScanPoint& pt, float range) const;
    void drawGrid(QPainter& p) const;
    void drawTrails(QPainter& p) const;
    void drawPoints(QPainter& p) const;
    void drawNearest(QPainter& p) const;
    void drawColorBar(QPainter& p) const;
    void drawOverlay(QPainter& p) const;
    void updateNearest();

    ScanFrame   m_frame;
    std::deque<ScanFrame> m_trail;      // last N frames for trails
    static constexpr int TRAIL_FRAMES = 5;

    ViewMode m_viewMode   = ViewMode::Polar;
    float    m_maxRange_mm= 0.f;
    float    m_minRange_mm= 50.f;
    float    m_autoRange  = 5000.f;
    int      m_minQuality = 10;
    bool     m_showGrid   = true;
    bool     m_showInvalid= false;
    bool     m_showTrails = true;
    bool     m_showNearest= true;
    bool     m_accumulate = false;
    bool     m_colorMap   = true;

    // Pan / zoom
    float    m_scale      = 1.f;
    QPointF  m_panOffset  = {0,0};
    QPoint   m_lastMouse;
    bool     m_dragging   = false;

    NearestInfo m_nearest = {0,0,false};
};
