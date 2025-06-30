#include "TaskReminderDialog.h"
#include <QVBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QDate>
#include <algorithm> // 用于排序

TaskReminderDialog::TaskReminderDialog(const QList<QDate> &taskDates, QWidget *parent)
    : QDialog(parent)
{
    setupUiAndLogic(taskDates);
}

void TaskReminderDialog::setupUiAndLogic(const QList<QDate> &taskDates)
{
    setWindowTitle("日程提醒");
    setMinimumSize(400, 500);

    // --- 创建控件 ---
    reminderTextEdit = new QTextEdit(this);
    reminderTextEdit->setReadOnly(true);

    closeButton = new QPushButton("关闭", this);
    connect(closeButton, &QPushButton::clicked,
            this, &TaskReminderDialog::accept);

    // --- 布局 ---
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(reminderTextEdit);
    mainLayout->addWidget(closeButton, 0, Qt::AlignRight);
    setLayout(mainLayout);

    // --- 逻辑处理与内容生成 ---
    if (taskDates.isEmpty()) {
        reminderTextEdit->setHtml("<h2>太棒了！</h2><p>未来没有任何已安排的日程。</p>");
        return;
    }

    // 1. 过滤并排序日期
    QList<QDate> futureDates;
    const QDate today = QDate::currentDate();
    for (const QDate &date : taskDates) {
        if (date >= today) {
            futureDates.append(date);
        }
    }
    std::sort(futureDates.begin(), futureDates.end());

    // 2. 生成HTML内容
    QString htmlContent = "<h1>未来日程一览</h1><hr>";

    if (futureDates.isEmpty()) {
        htmlContent += "<p>未来没有任何已安排的日程。</p>";
    } else {
        htmlContent += "<p>您在以下日期安排了任务：</p><ul>";
        for (const QDate& date : futureDates) {
            qint64 daysUntil = today.daysTo(date);
            QString style;
            QString distanceText;

            if (daysUntil == 0) {
                style = "color:#D32F2F; font-weight:bold;"; // 红色加粗
                distanceText = " (今天)";
            } else if (daysUntil <= 6) {
                style = "color:#D32F2F;"; // 红色
                distanceText = QString(" (%1天后)").arg(daysUntil);
            } else if (daysUntil <= 14) {
                style = "color:#FBC02D;"; // 黄色
                distanceText = QString(" (%1天后)").arg(daysUntil);
            } else {
                style = "color:#388E3C;"; // 绿色
                distanceText = QString(" (%1天后)").arg(daysUntil);
            }

            htmlContent += QString("<li><span style='%1'>%2</span>%3</li>")
                               .arg(style)
                               .arg(date.toString("yyyy年MM月dd日 (dddd)"))
                               .arg(distanceText);
        }
        htmlContent += "</ul>";
    }

    reminderTextEdit->setHtml(htmlContent);

    // --- 设置样式 ---
    QString styleSheet = R"(
        QDialog { background-color: #F8F9FA; }
        QTextEdit {
            background-color: white;
            border: 1px solid #CED4DA;
            border-radius: 8px;
            font-size: 16px;
            padding: 15px;
        }
        QPushButton {
            background-color: #6C757D;
            color: white;
            border: none;
            padding: 10px 25px;
            border-radius: 5px;
        }
        QPushButton:hover { background-color: #5A6268; }
    )";
    this->setStyleSheet(styleSheet);
}
