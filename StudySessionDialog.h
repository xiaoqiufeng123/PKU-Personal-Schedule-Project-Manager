#ifndef STUDYSESSIONDIALOG_H
#define STUDYSESSIONDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QMovie> // 引入 QMovie 以支持 GIF
#include <QTimer>
#include <QTime>
// 不再需要 QPixmap，因为图片现在由 QMovie 处理
// #include <QPixmap>

class StudySessionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StudySessionDialog(QWidget *parent = nullptr);
    void startSession();  // 每次开始自习调用

private slots:
    void updateTimer();
    void onEndSessionClicked();

private:
    QLabel *timerLabel;
    QLabel *gifLabel;
    QPushButton *endButton;
    QMovie *animation; // 现在直接用于 GIF
    QTimer *timer;
    QTime startTime;

    void saveStudySession(const QDateTime &start, const QDateTime &end);
protected:
    // eventFilter 对于简单的 GIF 显示不再是严格必需的，
    // 但如果还有其他事件过滤需求可以保留。
    bool eventFilter(QObject *obj, QEvent *event) override;
};

#endif // STUDYSESSIONDIALOG_H
