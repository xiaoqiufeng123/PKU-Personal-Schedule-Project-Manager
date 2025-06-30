#ifndef STATISTICSWINDOW_H
#define STATISTICSWINDOW_H

#include <QWidget>

// 前向声明
QT_BEGIN_NAMESPACE
class QChartView;
class QPushButton;
class QComboBox;
class QStackedWidget; // 【新增】
QT_END_NAMESPACE

class StatisticsWindow : public QWidget {
    Q_OBJECT
public:
    StatisticsWindow(QWidget *parent = nullptr);

private slots:
    void onChartTypeChanged();
    void onUnitChanged();
    void onDeleteRecordsClicked();

private:
    enum ChartType { Bar, Line };
    enum TimeUnit { Seconds, Minutes, Hours };

    void setupUi(); // 【新增】将UI创建和逻辑分离
    void drawChart();

    // UI 控件
    QStackedWidget *stackedWidget; // 【核心修正】使用堆叠窗口
    QChartView *chartView;
    QWidget *noDataWidget;         // 【新增】用于显示“无数据”的独立窗口
    QPushButton *barChartButton;
    QPushButton *lineChartButton;
    QComboBox *unitComboBox;
    QPushButton *deleteButton;

    // 状态变量
    ChartType currentChartType = Bar;
    TimeUnit currentTimeUnit = Minutes;
};

#endif // STATISTICSWINDOW_H
