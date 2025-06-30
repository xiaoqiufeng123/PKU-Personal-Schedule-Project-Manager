#include "StudySessionWindow.h"
#include <QVBoxLayout>
#include <QLabel>

StudySessionWindow::StudySessionWindow(QWidget *parent) : QWidget(parent) {
    setWindowTitle("开始自习");
    setFixedSize(400, 300);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("这里是开始自习窗口", this));
    setLayout(layout);
}
