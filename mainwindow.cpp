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
#include <QMovie> // 用于加载GIF动画

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWindow)
    , courseWindow(nullptr)
    , studyDialog(nullptr)
    , statsWindow(nullptr)
    , headerLabel(nullptr)
    , typewriterTimer(nullptr)
    , typewriterPos(0)
    , deletingPhase(false)
{
    ui->setupUi(this);
    setWindowTitle("个人学习助手");

    // 初始化UI外观
    setupUiLooks();

    // 初始化打字机效果的语句库
    phraseList = {
        "Keep your determination!",
        "You are filled with the power of determination.",
        "It's a beautiful day outside...",
        "Geeetttt dunked on~",
        "Beware of the man who speaks in hands.",
        "Debugging is fun!"
    };

    // 创建并启动打字机效果定时器
    typewriterTimer = new QTimer(this);
    connect(typewriterTimer, &QTimer::timeout,
            this,           &MainWindow::onTypewriterTimeout);

    // 初始化数据库
    if (!DatabaseManager::instance().init()) {
        qDebug() << "数据库初始化失败";
    }

    // 设置当前日期并更新UI
    currentSelectedDate = QDate::currentDate();
    updateCalendarHighlights();
    onDateSelected(currentSelectedDate);
    calendarWidget->setSelectedDate(currentSelectedDate);

    // 启动打字机效果
    selectNextPhrase();
    startTypewriter(120);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupUiLooks()
{
    // 创建主界面控件
    courseScheduleButton = new QPushButton(" 课表与教室");
    studySessionButton = new QPushButton(" 开始自习");
    statisticsButton = new QPushButton(" 学习统计");
    reminderButton = new QPushButton(" 日程提醒");
    addDailyTaskButton = new QPushButton("添加 / 修改本日日程");
    calendarWidget = new QCalendarWidget();
    detailTextEdit = new QTextEdit();
    detailTextEdit->setReadOnly(true);

    // 创建学习激励标签
    QLabel *studyMotivationLabel = new QLabel();
    studyMotivationLabel->setAlignment(Qt::AlignCenter);
    studyMotivationLabel->setText(
        "<span style='color:red; font-size: 20px; font-family: \"Comic Sans MS\", cursive;'>你</span><br>"
        "<span style='color:green; font-size: 20px; font-family: \"Comic Sans MS\", cursive;'>今</span><br>"
        "<span style='color:blue; font-size: 20px; font-family: \"Comic Sans MS\", cursive;'>天</span><br>"
        "<span style='color:purple; font-size: 20px; font-family: \"Comic Sans MS\", cursive;'>学</span><br>"
        "<span style='color:orange; font-size: 20px; font-family: \"Comic Sans MS\", cursive;'>习</span><br>"
        "<span style='color:#008080; font-size: 20px; font-family: \"Comic Sans MS\", cursive;'>了</span><br>"
        "<span style='color:#FF69B4; font-size: 20px; font-family: \"Comic Sans MS\", cursive;'>吗</span>"
        "<span style='color:#800080; font-size: 20px; font-family: \"Comic Sans MS\", cursive;'>？</span>"
        );

    // 配置日历控件
    calendarWidget->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    QTextCharFormat blackTextFormat;
    blackTextFormat.setForeground(Qt::black);
    for (int day = 1; day <= 7; ++day) {
        calendarWidget->setWeekdayTextFormat(
            static_cast<Qt::DayOfWeek>(day),
            blackTextFormat
            );
    }

    // 创建左侧面板布局
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

    // 创建并配置GIF标签
    gifLabel = new QLabel;
    gifLabel->setAlignment(Qt::AlignCenter);
    gifLabel->setFixedSize(160, 160);
    gifMovie = new QMovie(":/images/lancer_main_window_160.gif");
    if (gifMovie->isValid()) {
        gifLabel->setMovie(gifMovie);
        gifMovie->setCacheMode(QMovie::CacheAll);
        gifMovie->start();
    } else {
        qDebug() << "错误：无法加载 GIF 文件";
    }
    leftLayout->addWidget(gifLabel);

    // 创建右侧面板布局
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

    // 创建主分割器
    splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(leftPane);
    splitter->addWidget(rightPane);
    splitter->setStretchFactor(1, 1);
    splitter->setHandleWidth(2);

    // 创建顶部标题标签
    headerLabel = new QLabel(this);
    headerLabel->setAlignment(Qt::AlignCenter);
    headerLabel->setFixedHeight(50);
    headerLabel->setStyleSheet("font-size:20px;");

    // 设置主布局
    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    rootLayout->addWidget(headerLabel);
    rootLayout->addWidget(splitter, 1);
    setLayout(rootLayout);

    // 连接信号和槽
    connect(courseScheduleButton, &QPushButton::clicked,
            this, &MainWindow::onCourseScheduleButtonClicked);
    connect(studySessionButton, &QPushButton::clicked,
            this, &MainWindow::onStudySessionButtonClicked);
    connect(statisticsButton, &QPushButton::clicked,
            this, &MainWindow::onStatisticsButtonClicked);
    connect(reminderButton, &QPushButton::clicked,
            this, &MainWindow::onReminderButtonClicked);
    connect(calendarWidget, &QCalendarWidget::clicked,
            this, &MainWindow::onDateSelected);
    connect(addDailyTaskButton, &QPushButton::clicked,
            this, &MainWindow::onAddDailyTaskClicked);

    // 应用样式表
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
            background-color: #FFFFFF;
        }
        /* 不属于本月的日期文字样式 */
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
        /* 特定按钮样式 */
        #addDailyTaskButton {
            background-color: #007BFF;
            text-align: center;
        }
        #addDailyTaskButton:hover {
            background-color: #0069D9;
        }
        #reminderButton {
            background-color: #17A2B8;
        }
        #reminderButton:hover {
            background-color: #138496;
        }
        /* 文本编辑框样式 */
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

void MainWindow::onReminderButtonClicked()
{
    QList<QDate> datesWithTasks = DatabaseManager::instance().getAllDatesWithTasks();
    TaskReminderDialog dialog(datesWithTasks, this);
    dialog.exec();
}

void MainWindow::updateCalendarHighlights()
{
    // 清除旧的高亮
    for (const QDate &date : m_highlightedDates) {
        calendarWidget->setDateTextFormat(date, QTextCharFormat());
    }
    m_highlightedDates.clear();

    // 获取有任务的日期并设置新样式
    QList<QDate> datesWithTasks = DatabaseManager::instance().getAllDatesWithTasks();
    const QDate today = QDate::currentDate();

    for (const QDate &taskDate : datesWithTasks) {
        if (taskDate < today) continue;

        qint64 daysUntil = today.daysTo(taskDate);
        QTextCharFormat format;
        format.setFontWeight(QFont::Bold);
        format.setForeground(Qt::black);

        // 根据剩余天数设置不同背景色
        if (daysUntil >= 15) {
            format.setBackground(QColor("#C8E6C9")); // 绿色
        } else if (daysUntil >= 7) {
            format.setBackground(QColor("#FFF9C4")); // 黄色
        } else {
            format.setBackground(QColor("#FFCDD2")); // 红色
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

void MainWindow::onDateSelected(const QDate &date)
{
    currentSelectedDate = date;
    detailTextEdit->clear();

    // 获取选定日期的任务
    QList<DailyTask> tasks = DatabaseManager::instance().getTasksForDate(date);

    if (tasks.isEmpty()) {
        detailTextEdit->setHtml(
            QString("<h3>%1</h3><p>这一天还没有任务。</p>")
                .arg(date.toString("yyyy年MM月dd日"))
            );
    } else {
        QString html = QString("<h3>%1 的日程：</h3>")
                           .arg(date.toString("yyyy年MM月dd日"));

        // 格式化每个任务的信息
        for (const DailyTask &task : tasks) {
            html += QString(
                        "<hr>"
                        "<p><b>标题:</b> %1<br>"
                        "<b>时间:</b> %2 - %3<br>"
                        "<b>备注:</b> %4</p>"
                        )
                        .arg(task.getTitle())
                        .arg(task.getStartTime().toString("HH:mm"))
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

    // 更新UI显示
    onDateSelected(currentSelectedDate);
    updateCalendarHighlights();
}

void MainWindow::selectNextPhrase()
{
    // 随机选择一条语句
    int idx = QRandomGenerator::global()->bounded(phraseList.size());
    currentPhrase  = phraseList.at(idx);
    typewriterPos  = 0;
    deletingPhase  = false;
    headerLabel->clear();
}

void MainWindow::startTypewriter(int intervalMs)
{
    if (typewriterTimer->isActive()) {
        typewriterTimer->stop();
    }
    typewriterTimer->start(intervalMs);
}

void MainWindow::onTypewriterTimeout()
{
    // 打字阶段
    if (!deletingPhase) {
        if (typewriterPos < currentPhrase.length()) {
            headerLabel->setText(
                headerLabel->text() + currentPhrase.at(typewriterPos)
                );
            ++typewriterPos;
        } else {
            // 完成打字后暂停1秒进入删除阶段
            typewriterTimer->stop();
            QTimer::singleShot(1000, this, [this]() {
                deletingPhase = true;
                startTypewriter(30);
            });
        }
    }
    // 删除阶段
    else {
        QString txt = headerLabel->text();
        if (!txt.isEmpty()) {
            txt.chop(1);
            headerLabel->setText(txt);
        } else {
            // 完成删除后暂停0.5秒选择新语句
            typewriterTimer->stop();
            QTimer::singleShot(500, this, [this]() {
                selectNextPhrase();
                startTypewriter(90);
            });
        }
    }
}
