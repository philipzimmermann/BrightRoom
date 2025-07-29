#include "my_slider.h"
#include <QMouseEvent>

MySlider::MySlider(Qt::Orientation orientation, QWidget* parent)
    : QSlider(orientation, parent) {
}

void MySlider::mouseDoubleClickEvent(QMouseEvent* event) {
    QSlider::mouseDoubleClickEvent(event);
    emit doubleClicked();
}