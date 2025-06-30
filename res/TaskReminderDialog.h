#ifndef TASKREMINDERDIALOG_H
#define TASKREMINDERDIALOG_H

#include <QDialog>
#include <QList>
#include <QDate>

// 前向声明，避免包含完整头文件
class QTextEdit;
class QPushButton;

class TaskReminderDialog : public QDialog
{
    Q_OBJECT

public:
    // 构造函数，接收一个包含任务的日期列表
    explicit TaskReminderDialog(const QList<QDate> &taskDates, QWidget *parent = nullptr);

private:
    void setupUiAndLogic(const QList<QDate> &taskDates); // 设置UI和处理逻辑的函数

    QTextEdit *reminderTextEdit; // 用于显示提醒内容的文本框
    QPushButton *closeButton;    // 关闭按钮
};

#endif // TASKREMINDERDIALOG_H
