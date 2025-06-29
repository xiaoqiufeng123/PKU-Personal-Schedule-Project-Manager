#include "StatisticsWindow.h"
#include "DatabaseManager.h"

#include <QtCharts> // 在Qt6中，可以这样方便地包含所有Charts组件
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

//using namespace QtCharts;

StatisticsWindow::StatisticsWindow(QWidget *parent)
    : QWidget(parent)
{
    setupUi(); // 调用UI创建函数

    // 连接信号和槽
    connect(barChartButton, &QPushButton::clicked, this, &StatisticsWindow::onChartTypeChanged);
    connect(lineChartButton, &QPushButton::clicked, this, &StatisticsWindow::onChartTypeChanged);
    connect(unitComboBox, &QComboBox::currentIndexChanged, this, &StatisticsWindow::onUnitChanged);
    connect(deleteButton, &QPushButton::clicked, this, &StatisticsWindow::onDeleteRecordsClicked);

    // 初始绘图
    drawChart();
}

void StatisticsWindow::setupUi()
{
    setWindowTitle("自习统计分析");
    resize(900, 600);

    // --- 创建顶部工具栏 ---
    barChartButton = new QPushButton("条形图");
    lineChartButton = new QPushButton("折线图");
    unitComboBox = new QComboBox();
    deleteButton = new QPushButton("删除所有记录");

    unitComboBox->addItems({"分钟", "秒", "小时"});
    unitComboBox->setCurrentIndex(0);
    deleteButton->setStyleSheet("background-color: #E74C3C; color: white;");

    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->addWidget(new QLabel("图表类型:"));
    topLayout->addWidget(barChartButton);
    topLayout->addWidget(lineChartButton);
    topLayout->addSpacing(30);
    topLayout->addWidget(new QLabel("时间单位:"));
    topLayout->addWidget(unitComboBox);
    topLayout->addStretch();
    topLayout->addWidget(deleteButton);

    // --- 【核心修正】创建 QStackedWidget 和它的两个页面 ---
    stackedWidget = new QStackedWidget(this);

    // 页面一：图表视图
    chartView = new QChartView();
    chartView->setRenderHint(QPainter::Antialiasing);

    // 页面二：“无数据”提示视图
    noDataWidget = new QWidget(this);
    QLabel *noDataLabel = new QLabel("暂无自习数据，快去自习吧！", noDataWidget);
    noDataLabel->setAlignment(Qt::AlignCenter);
    noDataLabel->setStyleSheet("font-size: 20px; color: grey;");
    QVBoxLayout* noDataLayout = new QVBoxLayout(noDataWidget);
    noDataLayout->addWidget(noDataLabel);
    noDataWidget->setLayout(noDataLayout);

    // 将两个页面添加到 QStackedWidget
    stackedWidget->addWidget(chartView);      // 索引 0
    stackedWidget->addWidget(noDataWidget); // 索引 1

    // --- 整体布局 ---
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(stackedWidget); // 将 stackedWidget 添加到主布局
    setLayout(mainLayout);
}


void StatisticsWindow::drawChart()
{
    // 1. 从数据库获取数据
    QMap<QDate, int> dailyDurations = DatabaseManager::instance().getDailyStudyDurations();

    // 2. 【核心修正】根据是否有数据，切换 QStackedWidget 的页面
    if (dailyDurations.isEmpty()) {
        stackedWidget->setCurrentIndex(1); // 切换到“无数据”页面
        return;
    }
    stackedWidget->setCurrentIndex(0); // 切换到图表页面

    // --- 后续的绘图逻辑与之前相同，但现在是安全的 ---

    // 3. 准备单位换算
    double conversionFactor = 1.0;
    QString unitName;
    switch (currentTimeUnit) {
    case Seconds: conversionFactor = 1.0; unitName = "秒"; break;
    case Minutes: conversionFactor = 60.0; unitName = "分钟"; break;
    case Hours:   conversionFactor = 3600.0; unitName = "小时"; break;
    }
    QString yAxisTitle = QString("时长 (%1)").arg(unitName);

    // 4. 准备图表数据
    QStringList categories;
    QList<QDate> sortedDates = dailyDurations.keys();
    std::sort(sortedDates.begin(), sortedDates.end());
    double maxValue = 0;
    QAbstractSeries *series = nullptr;

    if (currentChartType == Bar) {
        QBarSeries *barSeries = new QBarSeries();
        QBarSet *set = new QBarSet(yAxisTitle);
        for (const QDate& date : sortedDates) {
            double value = static_cast<double>(dailyDurations.value(date)) / conversionFactor;
            *set << value;
            maxValue = std::max(maxValue, value);
        }
        barSeries->append(set);
        series = barSeries;
    } else { // Line Chart
        QLineSeries *lineSeries = new QLineSeries();
        lineSeries->setName(yAxisTitle);
        int i = 0;
        for (const QDate& date : sortedDates) {
            double value = static_cast<double>(dailyDurations.value(date)) / conversionFactor;
            lineSeries->append(i++, value);
            maxValue = std::max(maxValue, value);
        }
        series = lineSeries;
    }

    for (const QDate& date : sortedDates) {
        categories << date.toString("MM-dd");
    }

    // 5. 创建和配置 Chart
    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle(QString("每日自习时长统计 (%1)").arg(currentChartType == Bar ? "条形图" : "折线图"));
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    // 6. 配置坐标轴
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText(yAxisTitle);
    axisY->setLabelFormat("%.1f");
    axisY->setMin(0);
    axisY->setMax(maxValue * 1.1 + (maxValue > 0 ? 1.0 : 5.0) );

    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    // 7. 将图表设置到 QChartView
    chartView->setChart(chart);
}


void StatisticsWindow::onChartTypeChanged()
{
    auto* button = qobject_cast<QPushButton*>(sender());
    if (button == barChartButton) {
        currentChartType = Bar;
    } else if (button == lineChartButton) {
        currentChartType = Line;
    }
    drawChart();
}

void StatisticsWindow::onUnitChanged()
{
    switch(unitComboBox->currentIndex()){
    case 0: currentTimeUnit = Minutes; break;
    case 1: currentTimeUnit = Seconds; break;
    case 2: currentTimeUnit = Hours; break;
    }
    drawChart();
}

void StatisticsWindow::onDeleteRecordsClicked()
{
    bool ok;
    QString text = QInputDialog::getText(this, "确认删除",
                                         "这是一个不可逆操作，将永久删除所有自习记录。\n"
                                         "请输入 'yes' 以确认删除：",
                                         QLineEdit::Normal, "", &ok);
    if (ok && text == "yes") {
        if (DatabaseManager::instance().deleteAllStudySessions()) {
            QMessageBox::information(this, "成功", "所有自习记录已成功删除。");
            drawChart();
        } else {
            QMessageBox::critical(this, "错误", "删除记录时发生错误。");
        }
    } else if (ok) {
        QMessageBox::warning(this, "操作取消", "输入不正确，删除操作已取消。");
    }
}
