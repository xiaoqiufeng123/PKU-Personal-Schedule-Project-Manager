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
#include <QLabel> // 确保包含
#include <QMovie> // 引入 QMovie
#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QStringList>
#include <QRandomGenerator>

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
    void onReminderButtonClicked(); // 【整合】确保提醒按钮的槽函数声明存在

    void onTypewriterTimeout();

private:
    void setupUiLooks();
    void updateCalendarHighlights();

    // 打字机相关函数
    void startTypewriter(int intervalMs = 120);
    void selectNextPhrase();

    Ui::MainWindow *ui;
    QSplitter *splitter;
    QPushButton *courseScheduleButton;
    QPushButton *studySessionButton;
    QPushButton *statisticsButton;
    QPushButton *addDailyTaskButton;
    QPushButton *reminderButton; // 【整合】确保提醒按钮的成员变量声明存在

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

};

#endif // MAINWINDOW_H
