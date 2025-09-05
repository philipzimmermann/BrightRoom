#include "HistogramWidget.h"
#include <qcolor.h>
#include <qnamespace.h>
#include <qpainterpath.h>
#include <QPainter>
#include <algorithm>
#include "constants.h"

HistogramWidget::HistogramWidget(QWidget* parent) : QWidget(parent) {
    setMinimumSize(kMinWidth + 2 * kMargin, kMinHeight + 2 * kMargin);
}

void HistogramWidget::updateHistogram(const std::array<uint32_t, brightroom::kHistogramBins>& histogram) {
    _histogram = histogram;
    update();  // Request a repaint
}

void HistogramWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    float max_value = *std::max_element(_histogram.begin(), _histogram.end());

    // Calculate drawing area
    int draw_width = width() - 2 * kMargin;
    int draw_height = height() - 2 * kMargin;

    // Draw background
    painter.fillRect(rect(), QColor(35, 35, 35));

    // Draw histogram
    QPainterPath path;
    path.moveTo(kMargin, height() - kMargin);

    float x_scale = static_cast<float>(draw_width) / (_histogram.size() - 1);
    float y_scale = static_cast<float>(draw_height) / max_value;

    for (size_t i = 0; i < _histogram.size(); ++i) {
        float x = kMargin + i * x_scale;
        float y = height() - kMargin - _histogram[i] * y_scale;
        path.lineTo(x, y);
    }

    path.lineTo(width() - kMargin, height() - kMargin);
    path.closeSubpath();

    // Fill histogram with semi-transparent gray
    painter.fillPath(path, QColor(100, 100, 100, 128));

    // Draw outline
    painter.setPen(QPen(QColor(200, 200, 200), 1));
    painter.drawPath(path);
}