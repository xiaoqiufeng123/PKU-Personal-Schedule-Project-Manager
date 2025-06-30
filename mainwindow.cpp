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
#include <QMovie>
#include <QNetworkRequest>
#include <QIcon>
#include <QPair>

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
    , netManager(new QNetworkAccessManager(this))
    , iconNetManager(new QNetworkAccessManager(this)) // 初始化图标下载管理器
    , m_apiKey("4259998722ba752c0ce245ab9f00d75e") // OpenWeatherMap API Key
{
    ui->setupUi(this);
    setWindowTitle("个人学习助手");

    // 初始化UI外观
    setupUiLooks();
    setupWeatherUI(); // 初始化天气UI

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

    // 连接天气API的finished信号
    connect(netManager, &QNetworkAccessManager::finished,
            this, &MainWindow::onWeatherApiReply);
    // 连接图标下载的finished信号
    connect(iconNetManager, &QNetworkAccessManager::finished,
            this, &MainWindow::onWeatherIconReply);

    // 启动时获取默认城市的天气（例如：东京）
    fetchWeather("Tokyo"); // 您可以改为任何默认城市

    // 初始化天气描述到中文翻译的映射
    weatherTranslationMap["clear sky"] = "晴空";
    weatherTranslationMap["few clouds"] = "少云";
    weatherTranslationMap["scattered clouds"] = "多云";
    weatherTranslationMap["broken clouds"] = "碎云";
    weatherTranslationMap["overcast clouds"] = "阴天";
    weatherTranslationMap["shower rain"] = "阵雨";
    weatherTranslationMap["rain"] = "雨";
    weatherTranslationMap["light rain"] = "小雨";
    weatherTranslationMap["moderate rain"] = "中雨";
    weatherTranslationMap["heavy intensity rain"] = "大雨";
    weatherTranslationMap["thunderstorm"] = "雷暴";
    weatherTranslationMap["snow"] = "雪";
    weatherTranslationMap["light snow"] = "小雪";
    weatherTranslationMap["mist"] = "薄雾";
    weatherTranslationMap["fog"] = "雾";
    weatherTranslationMap["haze"] = "霾";
    weatherTranslationMap["sleet"] = "雨夹雪";
    weatherTranslationMap["light intensity drizzle"] = "毛毛雨";
    weatherTranslationMap["drizzle"] = "毛毛雨";
    weatherTranslationMap["heavy intensity drizzle"] = "大毛毛雨";
    weatherTranslationMap["drizzle rain"] = "毛毛雨";
    weatherTranslationMap["heavy intensity drizzle rain"] = "大毛毛雨";
    weatherTranslationMap["light intensity shower rain"] = "小阵雨";
    weatherTranslationMap["heavy intensity shower rain"] = "大阵雨";
    weatherTranslationMap["ragged shower rain"] = "零星阵雨";
    weatherTranslationMap["light snow"] = "小雪";
    weatherTranslationMap["snow"] = "雪";
    weatherTranslationMap["heavy snow"] = "大雪";
    weatherTranslationMap["sleet"] = "雨夹雪";
    weatherTranslationMap["light shower sleet"] = "小阵雨夹雪";
    weatherTranslationMap["shower sleet"] = "阵雨夹雪";
    weatherTranslationMap["light rain and snow"] = "小雨夹雪";
    weatherTranslationMap["rain and snow"] = "雨夹雪";
    weatherTranslationMap["light shower snow"] = "小阵雪";
    weatherTranslationMap["shower snow"] = "阵雪";
    weatherTranslationMap["heavy shower snow"] = "大阵雪";
    weatherTranslationMap["squalls"] = "飑";
    weatherTranslationMap["tornado"] = "龙卷风";
    weatherTranslationMap["dust whirls"] = "尘卷风";
    weatherTranslationMap["sand/dust whirls"] = "沙尘暴";
    weatherTranslationMap["volcanic ash"] = "火山灰";
    weatherTranslationMap["smoke"] = "烟";
    weatherTranslationMap["hail"] = "冰雹";
    weatherTranslationMap["freezing rain"] = "冻雨";
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
    gifMovie = new QMovie(":/images/lancer_main_window_160.gif"); // 确保您的资源文件路径正确
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
    // 创建一个包含激励标签和日历的水平布局
    QHBoxLayout *rightTopLayout = new QHBoxLayout();
    rightTopLayout->addWidget(studyMotivationLabel);
    rightTopLayout->addWidget(calendarWidget);

    // 创建右侧主垂直布局
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPane);
    rightLayout->addLayout(rightTopLayout, 5);
    rightLayout->addWidget(addDailyTaskButton);
    rightLayout->addWidget(detailTextEdit, 4);
    rightLayout->setSpacing(10);
    rightLayout->setContentsMargins(10, 10, 10, 10);
    rightPane->setLayout(rightLayout);

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
        /* 天气模块样式 */
        #weatherWidgetContainer {
            background-color: #E0F7FA;
            border-radius: 8px;
            padding: 10px;
            margin-bottom: 10px;
            border: 1px solid #B2EBF2;
        }
        #weatherLocationLabel {
            font-size: 18px;
            font-weight: bold;
            color: #00796B;
        }
        #weatherTemperatureLabel {
            font-size: 36px;
            font-weight: bold;
            color: #009688;
        }
        #weatherConditionLabel {
            font-size: 16px;
            color: #4CAF50;
        }
        #weatherIconLabel {
            background-color: transparent;
        }
        #cityLineEdit {
            padding: 8px;
            border: 1px solid #B0BEC5;
            border-radius: 5px;
            font-size: 14px;
        }
        #fetchWeatherButton {
            background-color: #4CAF50;
            color: white;
            padding: 8px 12px;
            border: none;
            border-radius: 5px;
            font-size: 14px;
        }
        #fetchWeatherButton:hover {
            background-color: #43A047;
        }
    )";
    this->setStyleSheet(styleSheet);
    addDailyTaskButton->setObjectName("addDailyTaskButton");
    reminderButton->setObjectName("reminderButton");
}

void MainWindow::setupWeatherUI()
{
    // 创建天气显示相关的控件
    weatherLocationLabel = new QLabel("正在获取天气...", this);
    weatherLocationLabel->setAlignment(Qt::AlignCenter);
    weatherLocationLabel->setObjectName("weatherLocationLabel");

    weatherTemperatureLabel = new QLabel("", this);
    weatherTemperatureLabel->setAlignment(Qt::AlignCenter);
    weatherTemperatureLabel->setObjectName("weatherTemperatureLabel");

    weatherConditionLabel = new QLabel("", this);
    weatherConditionLabel->setAlignment(Qt::AlignCenter);
    weatherConditionLabel->setObjectName("weatherConditionLabel");

    // 初始化天气图标标签
    weatherIconLabel = new QLabel(this);
    weatherIconLabel->setAlignment(Qt::AlignCenter);
    weatherIconLabel->setFixedSize(64, 64); // 设置图标大小，OpenWeatherMap的@2x图标通常是50x50或更大
    weatherIconLabel->setObjectName("weatherIconLabel");

    cityLineEdit = new QLineEdit(this);
    cityLineEdit->setPlaceholderText("输入城市名称 (例如: Beijing)");
    cityLineEdit->setClearButtonEnabled(true);
    cityLineEdit->setFixedHeight(35);
    cityLineEdit->setObjectName("cityLineEdit");

    fetchWeatherButton = new QPushButton("获取天气", this);
    fetchWeatherButton->setCursor(Qt::PointingHandCursor);
    fetchWeatherButton->setObjectName("fetchWeatherButton");

    // 连接城市输入框的回车键信号
    connect(cityLineEdit, &QLineEdit::returnPressed, this, &MainWindow::onFetchWeatherButtonClicked);
    // 连接获取天气按钮的点击信号
    connect(fetchWeatherButton, &QPushButton::clicked, this, &MainWindow::onFetchWeatherButtonClicked);

    // 创建天气信息的主水平布局，包含图标和文本信息
    QHBoxLayout *weatherDisplayLayout = new QHBoxLayout();
    weatherDisplayLayout->addWidget(weatherIconLabel); // 添加图标
    QVBoxLayout *weatherTextLayout = new QVBoxLayout(); // 文本信息的垂直布局
    weatherTextLayout->addWidget(weatherLocationLabel);
    weatherTextLayout->addWidget(weatherTemperatureLabel);
    weatherTextLayout->addWidget(weatherConditionLabel);
    weatherDisplayLayout->addLayout(weatherTextLayout);

    // 创建城市输入和按钮的水平布局
    QHBoxLayout *cityInputLayout = new QHBoxLayout();
    cityInputLayout->addWidget(cityLineEdit);
    cityInputLayout->addWidget(fetchWeatherButton);

    // 创建一个总的天气垂直布局，包含信息和输入/按钮
    QVBoxLayout *weatherLayout = new QVBoxLayout();
    weatherLayout->addLayout(weatherDisplayLayout); // 使用新的天气显示布局
    weatherLayout->addLayout(cityInputLayout);
    weatherLayout->addStretch();

    // 将天气布局放入一个QWidget容器中，方便管理和应用样式
    QWidget *weatherWidgetContainer = new QWidget();
    weatherWidgetContainer->setLayout(weatherLayout);
    weatherWidgetContainer->setObjectName("weatherWidgetContainer");

    // 将天气容器添加到右侧面板的布局中
    QVBoxLayout *currentRightLayout = qobject_cast<QVBoxLayout*>(splitter->widget(1)->layout());

    if (currentRightLayout) {
        currentRightLayout->insertWidget(1, weatherWidgetContainer); // 在索引1处插入天气容器
    } else {
        qDebug() << "错误：未找到 rightPane 的布局，无法添加天气控件。";
    }
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

// 天气模块相关实现

void MainWindow::onFetchWeatherButtonClicked()
{
    QString city = cityLineEdit->text().trimmed(); // 获取输入框中的城市名并去除空白
    if (!city.isEmpty()) {
        fetchWeather(city); // 如果不为空，则获取该城市天气
    } else {
        QMessageBox::warning(this, "输入错误", "请输入一个城市名称。");
    }
}

void MainWindow::fetchWeather(const QString &city)
{
    if (m_apiKey.isEmpty() || m_apiKey == "YOUR_OPENWEATHERMAP_API_KEY") {
        qDebug() << "错误：API Key 未设置或未替换。请在 mainwindow.cpp 中设置您的 OpenWeatherMap API key。";
        weatherLocationLabel->setText("API Key 未设置！");
        weatherTemperatureLabel->clear();
        weatherConditionLabel->clear();
        weatherIconLabel->clear(); // 清除图标
        return;
    }

    // 构建 OpenWeatherMap API 请求URL
    // q=城市名, units=metric表示摄氏度, appid=您的API Key, lang=en 获取英文描述
    QString url = QString("%1q=%2&units=metric&appid=%3&lang=en")
                      .arg(OPENWEATHER_API_BASE_URL)
                      .arg(city)
                      .arg(m_apiKey);

    QNetworkRequest request;
    request.setUrl(QUrl(url));
    netManager->get(request); // 发送 GET 请求

    // 更新UI显示为正在获取状态
    weatherLocationLabel->setText(QString("正在获取 %1 的天气...").arg(city));
    weatherTemperatureLabel->clear();
    weatherConditionLabel->clear();
    weatherIconLabel->clear(); // 清除图标
}

void MainWindow::onWeatherApiReply(QNetworkReply *reply)
{
    // 检查网络请求是否成功
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray responseData = reply->readAll(); // 读取所有响应数据
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData); // 将数据解析为JSON文档
        QJsonObject jsonObj = jsonDoc.object(); // 获取JSON根对象

        // 检查JSON响应中是否包含必要的字段
        if (jsonObj.contains("main") && jsonObj.contains("weather") && jsonObj.contains("name")) {
            QJsonObject mainObject = jsonObj["main"].toObject(); // 获取“main”对象，包含温度等信息
            QJsonArray weatherArray = jsonObj["weather"].toArray(); // 获取“weather”数组，包含天气状况

            QString cityName = jsonObj["name"].toString(); // 城市名
            double temperature = mainObject["temp"].toDouble(); // 温度
            QString englishCondition = weatherArray[0].toObject()["description"].toString(); // 天气描述（英文）
            QString iconCode = weatherArray[0].toObject()["icon"].toString(); // 天气图标代码

            // 更新UI显示
            updateWeatherUI(cityName, temperature, englishCondition, iconCode);
        } else {
            qDebug() << "无效的 JSON 响应：" << responseData;
            updateWeatherUI("错误", 0.0, "无法解析天气数据。", ""); // 传递空图标代码
        }
    } else {
        qDebug() << "网络错误：" << reply->errorString();
        updateWeatherUI("错误", 0.0, QString("无法获取天气：%1").arg(reply->errorString()), ""); // 传递空图标代码
    }
    reply->deleteLater(); // 释放QNetworkReply对象，避免内存泄漏
}

// 辅助函数：根据英文天气描述获取中文描述
QString MainWindow::getChineseWeatherCondition(const QString &englishCondition)
{
    QString lowerCaseCondition = englishCondition.toLower();
    if (weatherTranslationMap.contains(lowerCaseCondition)) {
        return weatherTranslationMap.value(lowerCaseCondition);
    } else {
        qDebug() << "未找到天气描述的中文翻译：" << englishCondition;
        return englishCondition; // 如果没有找到翻译，返回原始英文描述
    }
}

void MainWindow::updateWeatherUI(const QString &city, double temperature, const QString &englishCondition, const QString &iconCode)
{
    weatherLocationLabel->setText(city);
    // 格式化温度，保留一位小数，并加上摄氏度符号
    weatherTemperatureLabel->setText(QString("%1°C").arg(temperature, 0, 'f', 1));

    // 获取中文天气描述
    QString chineseCondition = getChineseWeatherCondition(englishCondition);
    weatherConditionLabel->setText(chineseCondition);

    // 从OpenWeatherMap下载图标
    if (!iconCode.isEmpty()) {
        QString iconUrl = QString("https://openweathermap.org/img/wn/%1@2x.png").arg(iconCode); // 获取@2x大小的图标
        iconNetManager->get(QNetworkRequest(QUrl(iconUrl)));
    } else {
        weatherIconLabel->clear(); // 没有图标代码，清除图标
    }
}

// 处理图标下载完成的槽函数
void MainWindow::onWeatherIconReply(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QPixmap pixmap;
        if (pixmap.loadFromData(reply->readAll())) {
            // 根据 QLabel 的大小等比例缩放图标
            weatherIconLabel->setPixmap(pixmap.scaled(weatherIconLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            qDebug() << "无法从数据加载图标。";
            weatherIconLabel->clear();
        }
    } else {
        qDebug() << "图标下载错误：" << reply->errorString();
        weatherIconLabel->clear();
    }
    reply->deleteLater();
}
