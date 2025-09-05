#pragma once

#include <QWidget>
#include <array>
#include <cstdint>
#include "constants.h"

class HistogramWidget : public QWidget {
    Q_OBJECT

   public:
    explicit HistogramWidget(QWidget* parent = nullptr);

    void updateHistogram(const std::array<uint32_t, brightroom::kHistogramBins>& histogram);

   protected:
    void paintEvent(QPaintEvent* event) override;

   private:
    static constexpr int kMargin = 10;
    static constexpr int kMinHeight = 150;
    static constexpr int kMinWidth = 256;  // One pixel per histogram bin

    std::array<uint32_t, brightroom::kHistogramBins> _histogram{0};
};