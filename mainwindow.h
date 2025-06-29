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

private:
    void setupUiLooks();
    void updateCalendarHighlights();

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
};

#endif // MAINWINDOW_H
