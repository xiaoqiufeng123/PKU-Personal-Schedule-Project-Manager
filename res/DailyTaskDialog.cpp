#include "DailyTaskDialog.h"    // 引入对话框头文件
#include "DatabaseManager.h"    // 引入数据库管理器，用于数据操作
#include <QVBoxLayout>          // 垂直布局
#include <QHBoxLayout>          // 水平布局
#include <QPushButton>          // 按钮
#include <QMessageBox>          // 消息框，用于警告/提示
#include <QLabel>               // 标签
#include <QLineEdit>            // 单行文本输入
#include <QTimeEdit>            // 时间选择
#include <QTextEdit>            // 多行文本输入
#include <QDialogButtonBox>     // 对话框标准按钮盒（这里未使用，但作为常见组件保留）
#include <QGridLayout>          // 网格布局，用于更好的对齐

// 构造函数
DailyTaskDialog::DailyTaskDialog(const QDate &date, QWidget *parent, const QList<DailyTask> &existingTasks)
    : QDialog(parent), currentDate(date),  taskList(existingTasks) // 初始化父类、当前日期和现有任务列表
{
    setWindowTitle("添加/修改日程"); // 设置窗口标题
    setMinimumSize(700, 500); // 设置最小尺寸，使界面更美观

    /* STEP 1: 设置页面布局 */

    // 创建左侧任务列表部件
    taskListWidget = new QListWidget(this);
    refreshTaskList(); // 初次加载时，填充左侧任务列表

    // 创建右侧的输入框和按钮
    titleEdit = new QLineEdit(this);
    startTimeEdit = new QTimeEdit(QTime::currentTime(), this); // 默认显示当前时间
    endTimeEdit = new QTimeEdit(QTime::currentTime().addSecs(3600), this); // 默认显示当前时间加一小时
    noteEdit = new QTextEdit(this);

    saveButton = new QPushButton("保存", this);
    cancelButton = new QPushButton("取消", this);
    newTaskButton = new QPushButton("新建日程", this); // 新建日程按钮
    deleteTaskButton = new QPushButton("删除日程", this); // 删除日程按钮

    // 连接信号和槽
    connect(saveButton, &QPushButton::clicked, this, &DailyTaskDialog::onSaveClicked);
    connect(cancelButton, &QPushButton::clicked, this, &DailyTaskDialog::reject); // 点击取消按钮关闭对话框
    connect(newTaskButton, &QPushButton::clicked, this, &DailyTaskDialog::onNewTaskClicked); // 连接新建按钮
    connect(deleteTaskButton, &QPushButton::clicked, this, &DailyTaskDialog::onDeleteTaskClicked); // 连接删除按钮
    connect(taskListWidget, &QListWidget::currentRowChanged, this, &DailyTaskDialog::onTaskSelected); // 列表选中项改变时触发

    // 使用 QGridLayout 布置输入字段，实现更好的对齐
    QGridLayout *inputLayout = new QGridLayout;
    inputLayout->addWidget(new QLabel("标题:"), 0, 0);
    inputLayout->addWidget(titleEdit, 0, 1);
    inputLayout->addWidget(new QLabel("开始时间:"), 1, 0);
    inputLayout->addWidget(startTimeEdit, 1, 1);
    inputLayout->addWidget(new QLabel("结束时间:"), 2, 0);
    inputLayout->addWidget(endTimeEdit, 2, 1);
    inputLayout->addWidget(new QLabel("备注:"), 3, 0, 1, 2); // 备注标签跨两列
    inputLayout->addWidget(noteEdit, 4, 0, 1, 2); // 备注输入框跨两列

    // 动作按钮布局 (新建、删除)
    QHBoxLayout *actionButtonLayout = new QHBoxLayout;
    actionButtonLayout->addWidget(newTaskButton);
    actionButtonLayout->addWidget(deleteTaskButton);
    actionButtonLayout->addStretch(); // 将按钮推向左侧

    // 对话框底部标准按钮布局 (保存、取消)
    QHBoxLayout *dialogButtonLayout = new QHBoxLayout;
    dialogButtonLayout->addStretch(); // 将按钮推向右侧
    dialogButtonLayout->addWidget(saveButton);
    dialogButtonLayout->addWidget(cancelButton);

    // 组合右侧布局：动作按钮 -> 输入区域 -> 底部对话框按钮
    QVBoxLayout *rightLayout = new QVBoxLayout;
    rightLayout->addLayout(actionButtonLayout); // 顶部添加动作按钮
    rightLayout->addLayout(inputLayout);       // 添加输入字段布局
    rightLayout->addStretch();                 // 填充剩余空间，将内容推向顶部
    rightLayout->addLayout(dialogButtonLayout); // 底部添加保存/取消按钮

    // 主布局：左侧列表和右侧输入/按钮区域
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->addWidget(taskListWidget, 1); // 左侧列表占据1份空间
    mainLayout->addLayout(rightLayout, 2);   // 右侧输入/按钮占据2份空间
    setLayout(mainLayout); // 应用主布局

    clearInputFields(); // 构造完成后，默认进入“新建任务”模式
}

// 获取当前对话框中的任务数据
DailyTask DailyTaskDialog::getTask() const {
    return DailyTask(titleEdit->text(),
                     startTimeEdit->time(),
                     endTimeEdit->time(),
                     noteEdit->toPlainText());
}

// 辅助函数：清空输入字段并重置为新建模式
void DailyTaskDialog::clearInputFields() {
    titleEdit->clear(); // 清空标题
    startTimeEdit->setTime(QTime::currentTime()); // 设置开始时间为当前时间
    endTimeEdit->setTime(QTime::currentTime().addSecs(3600)); // 设置结束时间为当前时间加一小时
    noteEdit->clear(); // 清空备注
    editingIndex = -1; // 设置为新建模式
    taskListWidget->clearSelection(); // 取消左侧列表的选中状态
}

// 辅助函数：刷新任务列表部件
void DailyTaskDialog::refreshTaskList() {
    taskListWidget->clear(); // 清空现有列表项
    for (const DailyTask &task : taskList) {
        taskListWidget->addItem(task.getTitle()); // 为每个任务添加标题到列表
    }
}

// 点击左侧任务列表项时触发
void DailyTaskDialog::onTaskSelected(int currentRow) {
    // 如果选中行无效（例如，清除了选中或超出范围），则切换到新建任务模式
    if (currentRow < 0 || currentRow >= taskList.size()) {
        clearInputFields();
        return;
    }

    const DailyTask &task = taskList[currentRow]; // 获取选中的任务对象

    // 将任务数据填充到右侧输入框
    titleEdit->setText(task.getTitle());
    startTimeEdit->setTime(task.getStartTime());
    endTimeEdit->setTime(task.getEndTime());
    noteEdit->setPlainText(task.getNote());

    editingIndex = currentRow; // 标记当前正在编辑的任务索引
}

// 点击保存按钮时触发
void DailyTaskDialog::onSaveClicked()
{
    QString title = titleEdit->text();
    QTime startTime = startTimeEdit->time();
    QTime endTime = endTimeEdit->time();
    QString note = noteEdit->toPlainText();

    // 标题不能为空检查
    if (title.isEmpty()) {
        QMessageBox::warning(this, "警告", "标题不能为空！");
        return;
    }

    DailyTask taskToSave(title, startTime, endTime, note); // 创建或更新的任务对象

    if (editingIndex >= 0 && editingIndex < taskList.size()) {
        // 处于编辑模式
        int oldId = taskList[editingIndex].getId(); // 获取原始任务的ID
        taskToSave.setId(oldId); // 将原始ID赋给新任务对象，以保留ID

        // 更新数据库中的任务
        if (DatabaseManager::instance().updateTaskById(oldId, taskToSave)) {
            taskList[editingIndex] = taskToSave; // 更新本地任务列表中的数据
            taskListWidget->item(editingIndex)->setText(taskToSave.getTitle()); // 更新左侧列表项的显示文本
            QMessageBox::information(this, "成功", "任务更新成功！");
        } else {
            QMessageBox::critical(this, "错误", "更新任务失败！");
            return;
        }
    } else {
        // 处于新建模式
        // 添加任务到数据库，并获取新生成的ID
        int newId = DatabaseManager::instance().addDailyTask(currentDate, taskToSave);
        if (newId != -1) {
            taskToSave.setId(newId); // 设置任务对象的ID为数据库生成的ID
            taskList.append(taskToSave); // 将新任务添加到本地任务列表
            taskListWidget->addItem(taskToSave.getTitle()); // 将新任务标题添加到左侧列表部件
            QMessageBox::information(this, "成功", "任务添加成功！");
        } else {
            QMessageBox::critical(this, "错误", "添加任务失败！");
            return;
        }
    }
    clearInputFields(); // 保存后，清空输入框并切换回“新建任务”模式
    // 如果希望保存后关闭对话框，取消注释下一行：
    // accept();
}

// “新建日程”按钮的槽函数
void DailyTaskDialog::onNewTaskClicked()
{
    clearInputFields(); // 调用辅助函数清空输入框，进入新建模式
}

// “删除日程”按钮的槽函数
void DailyTaskDialog::onDeleteTaskClicked()
{
    if (editingIndex >= 0 && editingIndex < taskList.size()) { // 确保有选中的任务
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "确认删除", "确定要删除此日程吗？",
                                      QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            int taskIdToDelete = taskList[editingIndex].getId(); // 获取要删除任务的ID
            if (DatabaseManager::instance().deleteTaskById(taskIdToDelete)) { // 从数据库中删除
                deleteTask(editingIndex); // 调用现有的deleteTask方法从本地列表和UI中删除
                QMessageBox::information(this, "成功", "任务删除成功！");
                clearInputFields(); // 删除后清空输入框
                // 重新刷新整个列表，确保索引正确，因为删除后原有索引可能失效
                // 或者在deleteTask内部直接处理好刷新
                // 暂时这里不调用refreshTaskList()，因为deleteTask内部已经处理了QListWidget的移除
            } else {
                QMessageBox::critical(this, "错误", "删除任务失败！");
            }
        }
    } else {
        QMessageBox::warning(this, "警告", "请选择一个要删除的日程！");
    }
}

// 删除任务的逻辑（从本地列表和UI中移除）
void DailyTaskDialog::deleteTask(int index) {
    if (index >= 0 && index < taskList.size()) {
        taskList.removeAt(index); // 从本地QList中移除任务
        QListWidgetItem *item = taskListWidget->takeItem(index); // 从QListWidget中移除项
        delete item; // 删除QListWidgetItem对象以释放内存
        editingIndex = -1; // 重置编辑索引，回到新建模式
        // 注意：这里删除了一个项，后续项的索引会发生变化。
        // 如果在外部调用此函数后有其他操作依赖索引，可能需要重新刷新或处理。
        // 在onDeleteTaskClicked中，我们删除后直接调用了clearInputFields，这会取消选中。
    }
}

// 判断是否处于编辑模式
bool DailyTaskDialog::isEditMode() const
{
    return editingIndex >= 0;
}
