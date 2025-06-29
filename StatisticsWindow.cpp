#include "StatisticsWindow.h"
#include "DatabaseManager.h"
#include <QtCharts> // 包含所有Qt Charts组件
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QInputDialog>
#include <QMessageBox>
#include <QDebug>
#include <QDate>
#include <algorithm>

StatisticsWindow::StatisticsWindow(QWidget *parent)
    : QWidget(parent)
{
    // 初始化UI
    setupUi();

    // 连接信号和槽
    connect(barChartButton, &QPushButton::clicked,
            this, &StatisticsWindow::onChartTypeChanged);
    connect(lineChartButton, &QPushButton::clicked,
            this, &StatisticsWindow::onChartTypeChanged);
    connect(unitComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StatisticsWindow::onUnitChanged);
    connect(deleteButton, &QPushButton::clicked,
            this, &StatisticsWindow::onDeleteRecordsClicked);

    // 初始绘图
    drawChart();
}

void StatisticsWindow::setupUi()
{
    setWindowTitle("自习统计分析");
    resize(900, 600);

    // --- 创建顶部工具栏控件 ---
    barChartButton = new QPushButton("条形图");
    lineChartButton = new QPushButton("折线图");
    unitComboBox = new QComboBox();
    deleteButton = new QPushButton("删除所有记录");

    // 配置单位选择框
    unitComboBox->addItems({"分钟", "秒", "小时"});
    unitComboBox->setCurrentIndex(0); // 默认选择"分钟"

    // 设置删除按钮样式
    deleteButton->setStyleSheet("background-color: #E74C3C; color: white;");

    // 创建顶部布局
    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->addWidget(new QLabel("图表类型:"));
    topLayout->addWidget(barChartButton);
    topLayout->addWidget(lineChartButton);
    topLayout->addSpacing(30);
    topLayout->addWidget(new QLabel("时间单位:"));
    topLayout->addWidget(unitComboBox);
    topLayout->addStretch(); // 添加弹簧
    topLayout->addWidget(deleteButton);

    // --- 创建堆叠窗口控件 ---
    stackedWidget = new QStackedWidget(this);

    // 页面一：图表视图
    chartView = new QChartView();
    chartView->setRenderHint(QPainter::Antialiasing); // 启用抗锯齿

    // 页面二：无数据提示
    noDataWidget = new QWidget(this);
    QLabel *noDataLabel = new QLabel("暂无自习数据，快去自习吧！", noDataWidget);
    noDataLabel->setAlignment(Qt::AlignCenter);
    noDataLabel->setStyleSheet("font-size: 20px; color: grey;");
    QVBoxLayout* noDataLayout = new QVBoxLayout(noDataWidget);
    noDataLayout->addWidget(noDataLabel);
    noDataWidget->setLayout(noDataLayout);

    // 将两个页面添加到堆叠窗口
    stackedWidget->addWidget(chartView);    // 索引 0: 图表视图
    stackedWidget->addWidget(noDataWidget); // 索引 1: 无数据提示

    // --- 设置主布局 ---
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(stackedWidget);
    setLayout(mainLayout);
}

void StatisticsWindow::drawChart()
{
    // 从数据库获取每日学习时长数据
    QMap<QDate, int> dailyDurations = DatabaseManager::instance().getDailyStudyDurations();

    // 检查数据是否为空
    if (dailyDurations.isEmpty()) {
        stackedWidget->setCurrentIndex(1); // 显示"无数据"页面
        return;
    }
    stackedWidget->setCurrentIndex(0); // 显示图表页面

    // 设置单位转换因子
    double conversionFactor = 1.0;
    QString unitName;
    switch (currentTimeUnit) {
    case Seconds:
        conversionFactor = 1.0;
        unitName = "秒";
        break;
    case Minutes:
        conversionFactor = 60.0;
        unitName = "分钟";
        break;
    case Hours:
        conversionFactor = 3600.0;
        unitName = "小时";
        break;
    }
    QString yAxisTitle = QString("时长 (%1)").arg(unitName);

    // 准备数据：按日期排序
    QList<QDate> sortedDates = dailyDurations.keys();
    std::sort(sortedDates.begin(), sortedDates.end());

    // 准备类别标签和最大值
    QStringList categories;
    double maxValue = 0;
    QAbstractSeries *series = nullptr;

    // 创建条形图系列
    if (currentChartType == Bar) {
        QBarSeries *barSeries = new QBarSeries();
        QBarSet *set = new QBarSet(yAxisTitle);

        for (const QDate& date : sortedDates) {
            double value = static_cast<double>(dailyDurations[date]) / conversionFactor;
            *set << value;
            maxValue = std::max(maxValue, value);
            categories << date.toString("MM-dd");
        }

        barSeries->append(set);
        series = barSeries;
    }
    // 创建折线图系列
    else {
        QLineSeries *lineSeries = new QLineSeries();
        lineSeries->setName(yAxisTitle);
        int index = 0;

        for (const QDate& date : sortedDates) {
            double value = static_cast<double>(dailyDurations[date]) / conversionFactor;
            lineSeries->append(index++, value);
            maxValue = std::max(maxValue, value);
            categories << date.toString("MM-dd");
        }

        series = lineSeries;
    }

    // 创建图表对象
    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle(QString("每日自习时长统计 (%1)")
                        .arg(currentChartType == Bar ? "条形图" : "折线图"));
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    // 配置X轴
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    // 配置Y轴
    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText(yAxisTitle);
    axisY->setLabelFormat("%.1f");
    axisY->setMin(0);
    // 设置Y轴最大值，留10%的余量
    axisY->setMax(maxValue * 1.1 + (maxValue > 0 ? 1.0 : 5.0));
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    // 将图表设置到视图
    chartView->setChart(chart);
}

void StatisticsWindow::onChartTypeChanged()
{
    // 确定当前选择的图表类型
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if (button == barChartButton) {
        currentChartType = Bar;
    } else if (button == lineChartButton) {
        currentChartType = Line;
    }

    // 重绘图表
    drawChart();
}

void StatisticsWindow::onUnitChanged()
{
    // 更新时间单位
    switch(unitComboBox->currentIndex()) {
    case 0: currentTimeUnit = Minutes; break;
    case 1: currentTimeUnit = Seconds; break;
    case 2: currentTimeUnit = Hours;   break;
    }

    // 重绘图表
    drawChart();
}

void StatisticsWindow::onDeleteRecordsClicked()
{
    // 显示确认对话框
    bool confirmed;
    QString input = QInputDialog::getText(
        this,
        "确认删除",
        "这是一个不可逆操作，将永久删除所有自习记录。\n"
        "请输入 'yes' 以确认删除：",
        QLineEdit::Normal,
        "",
        &confirmed
        );

    if (!confirmed) return; // 用户取消操作

    if (input == "yes") {
        // 执行删除操作
        if (DatabaseManager::instance().deleteAllStudySessions()) {
            QMessageBox::information(
                this,
                "成功",
                "所有自习记录已成功删除。"
                );
            drawChart(); // 刷新图表
        } else {
            QMessageBox::critical(
                this,
                "错误",
                "删除记录时发生错误。"
                );
        }
    } else {
        QMessageBox::warning(
            this,
            "操作取消",
            "输入不正确，删除操作已取消。"
            );
    }
}
