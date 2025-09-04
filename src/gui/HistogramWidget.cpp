#include "HistogramWidget.h"
#include <qpainterpath.h>
#include <QPainter>
#include <algorithm>

HistogramWidget::HistogramWidget(QWidget* parent) : QWidget(parent) {
    setMinimumSize(kMinWidth + 2 * kMargin, kMinHeight + 2 * kMargin);
}

void HistogramWidget::updateHistogram(const std::array<uint8_t, 256>& histogram) {
    _histogram = histogram;
    update();  // Request a repaint
}

void HistogramWidget::paintEvent(QPaintEvent*) {
    if (_histogram.empty()) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Find the maximum value for scaling
    float max_value = *std::max_element(_histogram.begin(), _histogram.end());
    if (max_value <= 0) {
        return;
    }

    // Calculate drawing area
    int draw_width = width() - 2 * kMargin;
    int draw_height = height() - 2 * kMargin;

    // Draw background
    painter.fillRect(rect(), QColor(240, 240, 240));

    // Draw histogram
    QPainterPath path;
    path.moveTo(kMargin, height() - kMargin);

    float x_scale = static_cast<float>(draw_width) / (_histogram.size() - 1);
    float y_scale = draw_height / max_value;

    for (size_t i = 0; i < _histogram.size(); ++i) {
        float x = kMargin + i * x_scale;
        float y = height() - kMargin - (_histogram[i] * y_scale);
        path.lineTo(x, y);
    }

    path.lineTo(width() - kMargin, height() - kMargin);
    path.closeSubpath();

    // Fill histogram with semi-transparent gray
    painter.fillPath(path, QColor(100, 100, 100, 128));

    // Draw outline
    painter.setPen(QPen(Qt::black, 1));
    painter.drawPath(path);
}