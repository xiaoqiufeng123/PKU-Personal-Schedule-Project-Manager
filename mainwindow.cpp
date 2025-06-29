#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidgetItem>
#include <QDebug>
#include "CourseScheduleWindow.h"
#include "StudySessionDialog.h"
#include "StatisticsWindow.h"
#include "DatabaseManager.h"
#include <QLabel>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWindow)
    , courseWindow(nullptr)
    , studyDialog(nullptr)
    , statsWindow(nullptr)
{
    ui->setupUi(this);
    setWindowTitle("个人学习助手");

    setupUiLooks();

    if (!DatabaseManager::instance().init())
    {
        qDebug() << "数据库初始化失败";
    }

    currentSelectedDate = QDate::currentDate();
    updateCalendarHighlights();
    onDateSelected(currentSelectedDate);
    calendarWidget->setSelectedDate(currentSelectedDate);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupUiLooks()
{
    // --- 1. 创建控件 (代码与之前相同) ---
    courseScheduleButton = new QPushButton(" 课表与教室");
    studySessionButton = new QPushButton(" 开始自习");
    statisticsButton = new QPushButton(" 学习统计");
    reminderButton = new QPushButton(" 日程提醒");
    addDailyTaskButton = new QPushButton("添加 / 修改本日日程");
    calendarWidget = new QCalendarWidget();
    detailTextEdit = new QTextEdit();
    detailTextEdit->setReadOnly(true);

    QLabel *studyMotivationLabel = new QLabel();
    studyMotivationLabel->setAlignment(Qt::AlignCenter);
    studyMotivationLabel->setText("<span style='color:red; font-size: 20px; font-family: \"Comic Sans MS\", cursive;'>你</span><br>"
                                  "<span style='color:green; font-size: 20px; font-family: \"Comic Sans MS\", cursive;'>今</span><br>"
                                  "<span style='color:blue; font-size: 20px; font-family: \"Comic Sans MS\", cursive;'>天</span><br>"
                                  "<span style='color:purple; font-size: 20px; font-family: \"Comic Sans MS\", cursive;'>学</span><br>"
                                  "<span style='color:orange; font-size: 20px; font-family: \"Comic Sans MS\", cursive;'>习</span><br>"
                                  "<span style='color:#008080; font-size: 20px; font-family: \"Comic Sans MS\", cursive;'>了</span><br>"
                                  "<span style='color:#FF69B4; font-size: 20px; font-family: \"Comic Sans MS\", cursive;'>吗</span><span style='color:#800080; font-size: 20px; font-family: \"Comic Sans MS\", cursive;'>？</span>");

    // --- 2. 【关键修正】通过C++代码设置日历格式 ---
    // 禁用默认的垂直表头（周数）
    calendarWidget->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);

    // 创建一个黑色字体的文本格式
    QTextCharFormat blackTextFormat;
    blackTextFormat.setForeground(Qt::black); // 设置颜色为黑色

    // 将周一到周日的所有文字格式都强制设置为黑色，覆盖掉默认的红色周末
    for (int day = 1; day <= 7; ++day) {
        calendarWidget->setWeekdayTextFormat(static_cast<Qt::DayOfWeek>(day), blackTextFormat);
    }


    // --- 3. 布局重构 (代码与之前相同) ---
    QWidget *leftPane = new QWidget();
    leftPane->setFixedWidth(200);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPane);
    leftLayout->setSpacing(20);
    leftLayout->setContentsMargins(15, 25, 15, 25);
    leftLayout->addWidget(courseScheduleButton);
    leftLayout->addWidget(studySessionButton);
    leftLayout->addWidget(statisticsButton);
    leftLayout->addWidget(reminderButton);
    leftLayout->addStretch();

    QWidget *rightPane = new QWidget();
    QHBoxLayout *rightTopLayout = new QHBoxLayout();
    rightTopLayout->addWidget(studyMotivationLabel);
    rightTopLayout->addWidget(calendarWidget);

    QVBoxLayout *rightLayout = new QVBoxLayout(rightPane);
    rightLayout->addLayout(rightTopLayout, 5);
    rightLayout->addWidget(addDailyTaskButton);
    rightLayout->addWidget(detailTextEdit, 4);
    rightLayout->setSpacing(10);
    rightLayout->setContentsMargins(10, 10, 10, 10);

    splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(leftPane);
    splitter->addWidget(rightPane);
    splitter->setStretchFactor(1, 1);
    splitter->setHandleWidth(2);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->addWidget(splitter);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    setLayout(mainLayout);

    // --- 4. 连接信号和槽 (代码与之前相同) ---
    connect(courseScheduleButton, &QPushButton::clicked, this, &MainWindow::onCourseScheduleButtonClicked);
    connect(studySessionButton, &QPushButton::clicked, this, &MainWindow::onStudySessionButtonClicked);
    connect(statisticsButton, &QPushButton::clicked, this, &MainWindow::onStatisticsButtonClicked);
    connect(reminderButton, &QPushButton::clicked, this, &MainWindow::onReminderButtonClicked);
    connect(calendarWidget, &QCalendarWidget::clicked, this, &MainWindow::onDateSelected);
    connect(addDailyTaskButton, &QPushButton::clicked, this, &MainWindow::onAddDailyTaskClicked);

    // --- 5.【关键修正】应用更精确的QSS样式表 ---
    QString styleSheet = R"(
        /* 全局字体和背景 */
        QWidget {
            font-family: 'Microsoft YaHei UI', 'Segoe UI', Arial, sans-serif;
            font-size: 15px;
            background-color: #F8F9FA;
        }
        QWidget > QWidget:first-child {
            background-color: #E9ECEF;
        }
        QPushButton {
            background-color: #6C757D;
            color: white;
            border: none;
            padding: 12px;
            border-radius: 8px;
            text-align: left;
        }
        QPushButton:hover {
            background-color: #5A6268;
        }
        /* 日历控件样式 */
        QCalendarWidget QTableView {
            /* 移除 alternate-background-color，确保所有日期背景统一 */
            background-color: #FFFFFF;
        }
        /* 【新增】将不属于本月的日期文字颜色设置为淡灰色 */
        QCalendarWidget QTableView::item:disabled {
            color: #d3d3d3;
        }
        /* 当前选中日期的样式 */
        QCalendarWidget QTableView::item:selected {
            background-color: #007BFF;
            color: white;
        }
        /* 日历导航栏 */
        QCalendarWidget QToolButton {
            color: #212529;
            font-size: 16px;
            background-color: transparent;
            border: none;
            margin: 5px;
        }
        QCalendarWidget QToolButton:hover {
            background-color: #E9ECEF;
            border-radius: 4px;
        }
        QCalendarWidget #qt_calendar_navigationbar {
            background-color: #F8F9FA;
        }
        /* 其他控件样式保持不变 */
        #addDailyTaskButton { background-color: #007BFF; text-align: center; }
        #addDailyTaskButton:hover { background-color: #0069D9; }
        #reminderButton { background-color: #17A2B8; }
        #reminderButton:hover { background-color: #138496; }
        QTextEdit {
            background-color: #FFFFFF;
            border: 1px solid #CED4DA;
            border-radius: 5px;
            padding: 8px;
            font-size: 16px;
            color: #495057;
        }
    )";
    this->setStyleSheet(styleSheet);
    addDailyTaskButton->setObjectName("addDailyTaskButton");
    reminderButton->setObjectName("reminderButton");
}


// ==============【整合】添加 onReminderButtonClicked 函数的实现 ==============
void MainWindow::onReminderButtonClicked()
{
    QList<QDate> datesWithTasks = DatabaseManager::instance().getAllDatesWithTasks();
    TaskReminderDialog dialog(datesWithTasks, this);
    dialog.exec();
}


// ============== 其余函数保持不变 ==============

void MainWindow::updateCalendarHighlights()
{
    for (const QDate &date : m_highlightedDates) {
        calendarWidget->setDateTextFormat(date, QTextCharFormat());
    }
    m_highlightedDates.clear();
    QList<QDate> datesWithTasks = DatabaseManager::instance().getAllDatesWithTasks();
    const QDate today = QDate::currentDate();
    for (const QDate &taskDate : datesWithTasks) {
        if (taskDate < today) continue;
        qint64 daysUntil = today.daysTo(taskDate);
        QTextCharFormat format;
        format.setFontWeight(QFont::Bold);
        format.setForeground(Qt::black);
        if (daysUntil >= 15) {
            format.setBackground(QColor("#C8E6C9"));
        } else if (daysUntil >= 7) {
            format.setBackground(QColor("#FFF9C4"));
        } else {
            format.setBackground(QColor("#FFCDD2"));
        }
        calendarWidget->setDateTextFormat(taskDate, format);
        m_highlightedDates.insert(taskDate);
    }
}

void MainWindow::onCourseScheduleButtonClicked()
{
    if (!courseWindow) {
        courseWindow = new CourseScheduleWindow(nullptr);
    }
    courseWindow->show();
    courseWindow->raise();
}

void MainWindow::onStudySessionButtonClicked()
{
    StudySessionDialog dialog(this);
    dialog.startSession();
    dialog.exec();
}

void MainWindow::onStatisticsButtonClicked()
{
    if(statsWindow) {
        statsWindow->close();
    }
    statsWindow = new StatisticsWindow(nullptr);
    statsWindow->show();
    statsWindow->raise();
}

void MainWindow::onDateSelected(const QDate &date) {
    currentSelectedDate = date;
    detailTextEdit->clear();
    QList<DailyTask> tasks = DatabaseManager::instance().getTasksForDate(date);
    if (tasks.isEmpty()) {
        detailTextEdit->setHtml(QString("<h3>%1</h3><p>这一天还没有任务。</p>").arg(date.toString("yyyy年MM月dd日")));
    } else {
        QString html = QString("<h3>%1 的日程：</h3>").arg(date.toString("yyyy年MM月dd日"));
        for (const DailyTask &task : tasks) {
            html += QString(
                        "<hr>"
                        "<p><b>标题:</b> %1<br>"
                        "<b>时间:</b> %2 - %3<br>"
                        "<b>备注:</b> %4</p>")
                        .arg(task.getTitle())
                        .arg(task.getStartTime().toString("HH:mm"))
                        // 【关键修正】将 posture_correction_data.getEndTime() 修改为 task.getEndTime()
                        .arg(task.getEndTime().toString("HH:mm"))
                        .arg(task.getNote().isEmpty() ? "无" : task.getNote());
        }
        detailTextEdit->setHtml(html);
    }
}

void MainWindow::onAddDailyTaskClicked()
{
    QList<DailyTask> tasks = DatabaseManager::instance().getTasksForDate(currentSelectedDate);
    DailyTaskDialog dialog(currentSelectedDate, this, tasks);
    dialog.exec();
    onDateSelected(currentSelectedDate);
    updateCalendarHighlights();
}
