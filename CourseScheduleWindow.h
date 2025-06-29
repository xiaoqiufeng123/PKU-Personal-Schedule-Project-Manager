#ifndef COURSESCHEDULEWINDOW_H
#define COURSESCHEDULEWINDOW_H

#include <QWidget>
#include <QProcess>
#include <QHBoxLayout>
#include <QPushButton>
#include <QInputDialog> // 新增
#include <QMessageBox>  // 新增
#include "smartroomwidget.h"

QT_BEGIN_NAMESPACE
class QTableWidget;
class QLabel;
class QDragEnterEvent;
class QDropEvent;
QT_END_NAMESPACE

class CourseScheduleWindow : public QWidget
{
    Q_OBJECT

public:
    explicit CourseScheduleWindow(QWidget *parent = nullptr);
    ~CourseScheduleWindow();

protected:
    // 重写这两个事件处理函数来响应拖拽
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void onScraperFinished(int exitCode, QProcess::ExitStatus exitStatus);
    // void onFreeRoomButtonClicked(); // 不再需要
    void onTableCellClicked(int row, int column); // 新增：处理单元格点击
    void onFreeRoomQueryFinished();               // 新增：处理教室查询脚本结束

private:
    void setupUi();
    void startScraperWithFile(const QString& filePath);
    void populateTable(const QByteArray& jsonData);

    // --- 新增的函数 ---
    void saveSchedule(const QByteArray& jsonData);
    void loadSchedule();
    // -----------------
    QHBoxLayout* topLayout;
    QLabel* m_infoLabel; // 用于提示用户拖拽文件
    // QPushButton* m_freeRoomButton; // 不再需要
    SmartRoomWidget* m_freeRoomWindow = nullptr;
    QTableWidget* m_scheduleTable;
    QProcess* m_scraperProcess;

    // --- 新增成员 ---
    QProcess* m_freeRoomQueryProcess; // 用于查询空闲教室的进程
    int m_lastQueriedPeriod = -1;     // 记录上次查询的节次
    QString m_lastQueriedBuilding;    // 记录上次查询的教学楼
};

#endif // COURSESCHEDULEWINDOW_H
