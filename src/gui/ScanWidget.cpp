#define _USE_MATH_DEFINES
#include <cmath>
#include "ScanWidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QImage>
#include <algorithm>

// ── Colour map (green→cyan→blue→purple) by distance ─────────────────────────
static QColor distColor(float t) {
    // t in [0,1]: 0=near(green), 0.33=cyan, 0.66=blue, 1=purple
    t = std::clamp(t, 0.f, 1.f);
    if (t < 0.33f) {
        float s = t / 0.33f;
        return QColor(int(29*(1-s)+55*s), int(158*(1-s)+179*s), int(117*(1-s)+208*s));
    } else if (t < 0.66f) {
        float s = (t-0.33f)/0.33f;
        return QColor(int(55*(1-s)+55*s), int(179*(1-s)+120*s), int(208*(1-s)+221*s));
    } else {
        float s = (t-0.66f)/0.34f;
        return QColor(int(55*(1-s)+139*s), int(120*(1-s)+92*s), int(221*(1-s)+246*s));
    }
}

ScanWidget::ScanWidget(QWidget* parent) : QWidget(parent) {
    setMinimumSize(300,300);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
}

// ── Public slots ──────────────────────────────────────────────────────────────
void ScanWidget::setFrame(const ScanFrame& frame) {
    if (m_accumulate) {
        // merge into trail only
    } else {
        if (m_showTrails) {
            m_trail.push_back(m_frame);
            while ((int)m_trail.size() > TRAIL_FRAMES) m_trail.pop_front();
        }
    }
    m_frame = frame;

    // auto range from 95th percentile
    if (m_maxRange_mm <= 0.f && !frame.empty()) {
        std::vector<float> dists;
        dists.reserve(frame.size());
        for (const auto& pt : frame)
            if (pt.isValid()) dists.push_back(pt.distance_mm);
        if (!dists.empty()) {
            std::sort(dists.begin(), dists.end());
            auto p95 = dists[std::size_t(dists.size()*0.95)];
            m_autoRange = std::max(500.f, p95 * 1.1f);
        }
    }
    updateNearest();
    update();
}

void ScanWidget::saveSnapshot(const QString& path) {
    QImage img(size(), QImage::Format_ARGB32);
    render(&img);
    img.save(path);
}

// ── Nearest obstacle ──────────────────────────────────────────────────────────
void ScanWidget::updateNearest() {
    m_nearest = {0,0,false};
    float minD = 1e9f;
    for (const auto& pt : m_frame) {
        if (!pt.isValid() || pt.quality < m_minQuality) continue;
        if (pt.distance_mm < m_minRange_mm) continue;
        if (pt.distance_mm < minD) {
            minD = pt.distance_mm;
            m_nearest = {pt.distance_mm, pt.angle_deg, true};
        }
    }
    if (m_nearest.valid)
        emit nearestChanged(m_nearest.dist_mm, m_nearest.angle_deg);
}

// ── Coordinate transform ──────────────────────────────────────────────────────
QPointF ScanWidget::toPixel(const ScanPoint& pt) const {
    float range  = (m_maxRange_mm > 0.f) ? m_maxRange_mm : m_autoRange;
    float radius = std::min(width(), height()) / 2.f * 0.88f;
    float scale  = (radius / range) * m_scale;
    float cx = width()  / 2.f + m_panOffset.x();
    float cy = height() / 2.f + m_panOffset.y();

    if (m_viewMode == ViewMode::Cartesian) {
        return { cx + pt.x() / range * radius * m_scale,
                 cy - pt.y() / range * radius * m_scale };
    }
    // Polar: angle 0=up, clockwise
    float rad = pt.angle_deg * float(M_PI) / 180.f;
    return { cx + pt.distance_mm * std::sin(rad) * scale,
             cy - pt.distance_mm * std::cos(rad) * scale };
}

QColor ScanWidget::pointColor(const ScanPoint& pt, float range) const {
    if (!m_colorMap) return QColor(0, 220, 120);
    float t = std::clamp(pt.distance_mm / range, 0.f, 1.f);
    return distColor(t);
}

// ── Paint ─────────────────────────────────────────────────────────────────────
void ScanWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(8, 12, 20));

    if (m_showGrid)   drawGrid(p);
    if (m_showTrails) drawTrails(p);
    drawPoints(p);
    if (m_showNearest && m_nearest.valid) drawNearest(p);
    drawColorBar(p);
    drawOverlay(p);
}

void ScanWidget::drawGrid(QPainter& p) const {
    float range  = (m_maxRange_mm > 0.f) ? m_maxRange_mm : m_autoRange;
    float radius = std::min(width(), height()) / 2.f * 0.88f * m_scale;
    float cx = width()  / 2.f + m_panOffset.x();
    float cy = height() / 2.f + m_panOffset.y();

    // Range rings
    float step = 1000.f;
    if (range < 3000) step = 500.f;
    if (range < 1500) step = 250.f;

    for (float r = step; r <= range * 1.05f; r += step) {
        float px = r / range * radius;
        p.setPen(QPen(QColor(50, 80, 120, 80), 0.5f, Qt::DashLine));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QPointF(cx,cy), px, px);

        p.setPen(QColor(80, 120, 180, 120));
        p.setFont(QFont("Courier", 9));
        QString lbl = r >= 1000 ? QString("%1m").arg(r/1000.f,0,'f',1)
                                : QString("%1mm").arg(int(r));
        p.drawText(QPointF(cx + px + 4, cy - 4), lbl);
    }

    // Axes
    p.setPen(QPen(QColor(60, 100, 160, 100), 0.5f));
    p.drawLine(QPointF(cx, cy - radius*1.1f), QPointF(cx, cy + radius*1.1f));
    p.drawLine(QPointF(cx - radius*1.1f, cy), QPointF(cx + radius*1.1f, cy));

    // Cardinal labels
    p.setPen(QColor(80, 130, 200, 150));
    p.setFont(QFont("Courier", 10));
    p.drawText(QPointF(cx - 5, cy - radius*1.05f), "N");
    p.drawText(QPointF(cx + radius*1.02f, cy + 5), "E");
    p.drawText(QPointF(cx - 5, cy + radius*1.1f), "S");
    p.drawText(QPointF(cx - radius*1.1f, cy + 5), "W");

    // Origin
    p.setBrush(QColor(220, 60, 60));
    p.setPen(Qt::NoPen);
    p.drawEllipse(QPointF(cx,cy), 5.0, 5.0);
}

void ScanWidget::drawTrails(QPainter& p) const {
    if (m_trail.empty()) return;
    float range = (m_maxRange_mm > 0.f) ? m_maxRange_mm : m_autoRange;

    for (int fi = 0; fi < (int)m_trail.size(); ++fi) {
        float alpha = float(fi+1) / float(m_trail.size() + 1) * 0.4f;
        for (const auto& pt : m_trail[fi]) {
            if (!pt.isValid() || pt.quality < m_minQuality) continue;
            if (pt.distance_mm < m_minRange_mm) continue;
            QColor c = pointColor(pt, range);
            c.setAlphaF(alpha);
            p.setPen(Qt::NoPen);
            p.setBrush(c);
            p.drawEllipse(toPixel(pt), 1.5, 1.5);
        }
    }
}

void ScanWidget::drawPoints(QPainter& p) const {
    if (m_frame.empty()) return;
    float range = (m_maxRange_mm > 0.f) ? m_maxRange_mm : m_autoRange;

    p.setPen(Qt::NoPen);
    for (const auto& pt : m_frame) {
        bool valid = pt.isValid() && pt.quality >= m_minQuality
                     && pt.distance_mm >= m_minRange_mm;
        if (!valid) {
            if (!m_showInvalid) continue;
            p.setBrush(QColor(60, 60, 80, 80));
            p.drawEllipse(toPixel(pt), 1.5, 1.5);
            continue;
        }
        QColor c = pointColor(pt, range);
        c.setAlpha(200);
        p.setBrush(c);
        // size: closer = slightly bigger
        float sz = 2.5f - std::clamp(pt.distance_mm/range, 0.f, 1.f) * 1.f;
        p.drawEllipse(toPixel(pt), sz, sz);
    }
}

void ScanWidget::drawNearest(QPainter& p) const {
    if (!m_nearest.valid) return;

    ScanPoint proxy;
    proxy.angle_deg   = m_nearest.angle_deg;
    proxy.distance_mm = m_nearest.dist_mm;
    proxy.quality     = 255;
    QPointF px = toPixel(proxy);

    float cx = width()  / 2.f + m_panOffset.x();
    float cy = height() / 2.f + m_panOffset.y();

    // Line from origin to nearest point
    p.setPen(QPen(QColor(220, 60, 60, 120), 0.5f, Qt::DashLine));
    p.drawLine(QPointF(cx, cy), px);

    // Circle around nearest
    p.setPen(QPen(QColor(220, 60, 60), 1.f));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(px, 6.0, 6.0);
    p.setBrush(QColor(220, 60, 60));
    p.setPen(Qt::NoPen);
    p.drawEllipse(px, 3.0, 3.0);

    // Label
    p.setPen(QColor(220, 60, 60));
    p.setFont(QFont("Courier", 9));
    QString lbl = QString("%1mm @ %2°")
        .arg(int(m_nearest.dist_mm))
        .arg(m_nearest.angle_deg, 0, 'f', 1);
    p.drawText(px + QPointF(8, -4), lbl);
}

void ScanWidget::drawColorBar(QPainter& p) const {
    float range = (m_maxRange_mm > 0.f) ? m_maxRange_mm : m_autoRange;
    int bw = 120, bh = 8;
    int bx = width()/2 - bw/2;
    int by = height() - 22;

    // Gradient bar
    for (int i = 0; i < bw; ++i) {
        QColor c = distColor(float(i)/bw);
        p.fillRect(bx+i, by, 1, bh, c);
    }
    p.setPen(QColor(80,120,180,120));
    p.drawRect(bx, by, bw, bh);

    p.setFont(QFont("Courier", 9));
    p.setPen(QColor(120,170,230,180));
    p.drawText(bx - 24, by + 8, "0m");
    p.drawText(bx + bw + 4, by + 8, QString("%1m").arg(range/1000.f,0,'f',1));
}

void ScanWidget::drawOverlay(QPainter& p) const {
    // Count valid points
    int valid = 0; float minD = 1e9f, maxD = 0;
    for (const auto& pt : m_frame) {
        if (pt.isValid() && pt.quality >= m_minQuality && pt.distance_mm >= m_minRange_mm) {
            ++valid; minD = std::min(minD,pt.distance_mm); maxD = std::max(maxD,pt.distance_mm);
        }
    }

    QFont mono("Courier", 9);
    p.setFont(mono);

    QStringList lines = {
        QString("Points  %1").arg(valid),
        QString("Min     %1 mm").arg(valid ? int(minD) : 0),
        QString("Max     %1 mm").arg(valid ? int(maxD) : 0),
        QString("Zoom    %1x").arg(m_scale,0,'f',1),
    };
    if (m_nearest.valid)
        lines << QString("Nearest %1mm@%2°").arg(int(m_nearest.dist_mm)).arg(m_nearest.angle_deg,0,'f',0);

    int lh = 16, pad = 8;
    int bw = 155, bh = lines.size() * lh + pad*2;
    p.fillRect(8, 8, bw, bh, QColor(8,12,20,180));
    p.setPen(QColor(50,80,120,120));
    p.drawRect(8, 8, bw, bh);

    for (int i = 0; i < lines.size(); ++i) {
        p.setPen(QColor(100,160,220,200));
        p.drawText(16, 8 + pad + (i+1)*lh - 3, lines[i]);
    }
}

// ── Interaction ───────────────────────────────────────────────────────────────
void ScanWidget::resizeEvent(QResizeEvent* e) { QWidget::resizeEvent(e); update(); }

void ScanWidget::wheelEvent(QWheelEvent* e) {
    float delta = e->angleDelta().y() / 120.f;
    m_scale = std::clamp(m_scale * std::pow(1.15f, delta), 0.2f, 30.f);
    update();
}

void ScanWidget::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        m_dragging = true; m_lastMouse = e->pos();
        setCursor(Qt::ClosedHandCursor);
    }
}

void ScanWidget::mouseMoveEvent(QMouseEvent* e) {
    if (m_dragging) {
        QPoint d = e->pos() - m_lastMouse;
        m_panOffset += QPointF(d); m_lastMouse = e->pos(); update();
    }
}

void ScanWidget::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) { m_dragging=false; setCursor(Qt::CrossCursor); }
}

void ScanWidget::mouseDoubleClickEvent(QMouseEvent*) {
    resetView();
}
