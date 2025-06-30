#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QSplitter>
#include <QListWidget>
#include <QStackedWidget>
#include <QPushButton>
#include <QCalendarWidget>
#include <QTextEdit>
#include <QDate>
#include <QTextCharFormat>
#include <QSet>
#include <QLabel>
#include <QMovie>
#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QStringList>
#include <QRandomGenerator>
#include <QLineEdit> // Added for QLineEdit
#include <QMap>    // Added for QMap
#include <QPair>   // Added for QPair

// 【新增】网络模块相关头文件
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QPixmap> // Added for QPixmap

#include "DailyTask.h"
#include "DailyTaskDialog.h"
#include "TaskReminderDialog.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

// 前向声明
class CourseScheduleWindow;
class StudySessionDialog;
class StatisticsWindow;

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onCourseScheduleButtonClicked();
    void onStudySessionButtonClicked();
    void onStatisticsButtonClicked();
    void onDateSelected(const QDate &date);
    void onAddDailyTaskClicked();
    void onReminderButtonClicked();

    void onTypewriterTimeout();

    // 天气模块的槽函数
    void onFetchWeatherButtonClicked(); // 搜索天气按钮点击槽函数
    void onWeatherApiReply(QNetworkReply *reply); // 天气API响应槽函数
    void onWeatherIconReply(QNetworkReply *reply); // 新增槽函数处理图标下载

private:
    void setupUiLooks();
    void updateCalendarHighlights();
    void setupWeatherUI(); // 设置天气UI的函数

    // 打字机相关函数
    void startTypewriter(int intervalMs = 120);
    void selectNextPhrase();

    // 天气模块相关函数
    void fetchWeather(const QString &city); // 获取天气数据
    // 修改：增加一个参数用于传递原始的英文天气描述和图标代码，以便进行翻译和图标加载
    void updateWeatherUI(const QString &city, double temperature, const QString &englishCondition, const QString &iconCode); // 更新天气UI显示

    // 辅助函数：根据英文天气描述获取中文描述
    QString getChineseWeatherCondition(const QString &englishCondition);


    Ui::MainWindow *ui;
    QSplitter *splitter;
    QPushButton *courseScheduleButton;
    QPushButton *studySessionButton;
    QPushButton *statisticsButton;
    QPushButton *addDailyTaskButton;
    QPushButton *reminderButton;

    QMap<QDate, QList<DailyTask>> dateInfoMap;
    QCalendarWidget *calendarWidget;
    QTextEdit *detailTextEdit;
    QDate currentSelectedDate;

    QSet<QDate> m_highlightedDates;

    CourseScheduleWindow *courseWindow;
    StudySessionDialog *studyDialog;
    StatisticsWindow *statsWindow;

    // 新增 GIF 播放控件
    QLabel *gifLabel;
    QMovie *gifMovie;

    // 顶部打字机
    QLabel      *headerLabel;
    QTimer      *typewriterTimer;
    QStringList  phraseList;      // 语句库
    QString      currentPhrase;   // 当前正在打/删的句子
    int          typewriterPos;   // 在 currentPhrase 中的位置
    bool         deletingPhase;   // false=打字，true=删除

    // 天气模块成员变量
    QNetworkAccessManager *netManager; // 网络请求管理器 (用于天气数据)
    QNetworkAccessManager *iconNetManager; // 新增：用于下载图标的网络管理器
    QLabel *weatherLocationLabel;      // 显示城市名
    QLabel *weatherTemperatureLabel;   // 显示温度
    QLabel *weatherConditionLabel;     // 显示天气状况（中文）
    QLabel *weatherIconLabel;          // 天气图标标签
    QLineEdit *cityLineEdit;           // 城市输入框
    QPushButton *fetchWeatherButton;   // 获取天气按钮

    QString m_apiKey; // OpenWeatherMap API Key
    const QString OPENWEATHER_API_BASE_URL = "http://api.openweathermap.org/data/2.5/weather?"; // OpenWeatherMap API基础URL

    // 天气描述到中文翻译的映射
    QMap<QString, QString> weatherTranslationMap;
};

#endif // MAINWINDOW_H
