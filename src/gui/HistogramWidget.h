#pragma once

#include <QWidget>
#include <array>
#include <cstdint>

class HistogramWidget : public QWidget {
    Q_OBJECT

   public:
    explicit HistogramWidget(QWidget* parent = nullptr);

    void updateHistogram(const std::array<uint8_t, 256>& histogram);

   protected:
    void paintEvent(QPaintEvent* event) override;

   private:
    std::array<uint8_t, 256> _histogram;
    static constexpr int kMargin = 10;
    static constexpr int kMinHeight = 150;
    static constexpr int kMinWidth = 256;  // One pixel per histogram bin
};