#pragma once

#include <QSlider>
#include <QMouseEvent>

class MySlider : public QSlider {
    Q_OBJECT

public:
    explicit MySlider(Qt::Orientation orientation, QWidget* parent = nullptr);

signals:
    void doubleClicked();

protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override;
};