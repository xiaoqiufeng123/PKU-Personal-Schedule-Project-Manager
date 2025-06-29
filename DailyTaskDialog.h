#ifndef DAILYTASKDIALOG_H
#define DAILYTASKDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QLineEdit>
#include <QTimeEdit>
#include <QTextEdit>
#include <QPushButton> // 引入QPushButton，因为头文件里用到了

#include "DailyTask.h" // 引入DailyTask类定义

class QLineEdit; // 前向声明QLineEdit，避免循环引用
class QTimeEdit; // 前向声明QTimeEdit
class QTextEdit; // 前向声明QTextEdit

class DailyTaskDialog : public QDialog
{
    Q_OBJECT // 宏，启用Qt的元对象系统，用于信号和槽

public:
    // 构造函数，接收日期、父部件和现有任务列表
    DailyTaskDialog(const QDate &date,
                    QWidget *parent = nullptr,
                    const QList<DailyTask> &existingTasks = {});
    // 获取当前对话框中的任务数据
    DailyTask getTask() const;
    // 判断当前是否处于编辑模式
    bool isEditMode() const;
    // 删除任务（已存在，但在cpp中会有更具体的调用逻辑）
    void deleteTask(int index);

private slots:
    // 当左侧任务列表选中项改变时触发
    void onTaskSelected(int currentRow);
    // 点击保存按钮时触发
    void onSaveClicked();
    // 点击“新建日程”按钮时触发
    void onNewTaskClicked();
    // 点击“删除日程”按钮时触发
    void onDeleteTaskClicked();

private:
    // 辅助函数：清空右侧输入框内容并重置为新建模式
    void clearInputFields();
    // 辅助函数：刷新左侧任务列表
    void refreshTaskList();

    QListWidget *taskListWidget; // 左侧任务列表部件
    QLineEdit *titleEdit;        // 任务标题输入框
    QTimeEdit *startTimeEdit;    // 开始时间选择器
    QTimeEdit *endTimeEdit;      // 结束时间选择器
    QTextEdit *noteEdit;         // 备注输入框

    QPushButton *saveButton;     // 保存按钮
    QPushButton *cancelButton;   // 取消按钮
    QPushButton *newTaskButton;  // 新建日程按钮
    QPushButton *deleteTaskButton; // 删除日程按钮

    QDate currentDate;           // 当前日期，用于添加新任务时关联

    QList<DailyTask> taskList;   // 存储当前日期的任务列表
    int editingIndex = -1;       // -1表示添加模式，其他索引表示编辑模式
};

#endif // DAILYTASKDIALOG_H
